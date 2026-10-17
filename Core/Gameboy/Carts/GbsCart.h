#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/Gameboy.h"
#include "Gameboy/GbMemoryManager.h"
#include "Shared/Audio/AudioPlayerTypes.h"
#include "Shared/Emulator.h"
#include "Shared/SystemActionManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// GBS (Game Boy Sound) file player cartridge.
/// Plays Game Boy music rips as an audio player.
/// </summary>
/// <remarks>
/// **GBS Format Overview:**
/// - Audio rips from Game Boy games
/// - Contains music driver code + data
/// - Multiple tracks per file
/// - Uses actual GB hardware for playback
///
/// **GBS Header:**
/// - Load Address: Where to load music data
/// - Init Address: Called once per track with track# in A
/// - Play Address: Called on each frame/timer tick
/// - Timer settings: For non-vblank-based music
///
/// **Playback Process:**
/// 1. Load music data to specified address
/// 2. Patch interrupt vectors for INIT/PLAY
/// 3. Set up timer or vblank IRQ for tempo
/// 4. Call INIT with track number
/// 5. IRQ handler calls PLAY repeatedly
///
/// **Memory Layout (patched):**
/// - $0000-$003F: RST vector trampolines
/// - $0040: VBlank IRQ → CALL PLAY
/// - $0048: STAT IRQ → CALL PLAY
/// - $0050: Timer IRQ → CALL PLAY
/// - $0070: Entry point → CALL INIT, infinite loop
/// - $4000+: Switchable bank (music data)
///
/// **Timing Methods:**
/// - Timer-based: TAC/TMA registers for precise BPM
/// - VBlank-based: 59.7 Hz interrupt rate
/// - CGB double-speed for some files
/// </remarks>
class GbsCart : public GbCart {
private:
	/// <summary>Current PRG bank for music data.</summary>
	uint8_t _prgBank = 1;

	/// <summary>Currently playing track (0-indexed).</summary>
	uint8_t _currentTrack = 0;

	/// <summary>Master clock at track start (for position display).</summary>
	uint64_t _startClock = 0;

	/// <summary>GBS file header with addresses and metadata.</summary>
	GbsHeader _header = {};

public:
	/// <summary>
	/// Creates a GBS player with the given header.
	/// </summary>
	/// <param name="header">Parsed GBS file header.</param>
	GbsCart(GbsHeader header) {
		_header = header;

		// Log GBS information
		MessageManager::Log("[GBS] Load: $" + HexUtilities::ToHex(header.LoadAddress[0] | (header.LoadAddress[1] << 8)));
		MessageManager::Log("[GBS] Init: $" + HexUtilities::ToHex(header.InitAddress[0] | (header.InitAddress[1] << 8)));
		MessageManager::Log("[GBS] Play: $" + HexUtilities::ToHex(header.PlayAddress[0] | (header.PlayAddress[1] << 8)));
		MessageManager::Log("[GBS] Timer Control: $" + HexUtilities::ToHex(header.TimerControl));
		MessageManager::Log("[GBS] Timer Modulo: $" + HexUtilities::ToHex(header.TimerModulo));
		MessageManager::Log("-----------------------------");

		_currentTrack = header.FirstTrack - 1;
	}

	/// <summary>
	/// Initializes GBS register mappings.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x2000, 0x3FFF, RegisterAccess::Write);
	}

	/// <summary>
	/// Initializes playback for a specific track.
	/// Patches ROM with interrupt handlers and sets up CPU state.
	/// </summary>
	/// <param name="selectedTrack">Track number to play (0-indexed).</param>
	void InitPlayback(uint8_t selectedTrack) {
		_currentTrack = selectedTrack;

		uint8_t* prg = _gameboy->DebugGetMemory(MemoryType::GbPrgRom);

		// Clear high ram and cart ram
		memset(_cartRam, 0, _gameboy->DebugGetMemorySize(MemoryType::GbCartRam));
		memset(_gameboy->DebugGetMemory(MemoryType::GbHighRam), 0, _gameboy->DebugGetMemorySize(MemoryType::GbHighRam));
		memset(_gameboy->DebugGetMemory(MemoryType::GbWorkRam), 0, _gameboy->DebugGetMemorySize(MemoryType::GbWorkRam));

		// Patch ROM to call INIT and PLAY routines

		uint16_t loadAddress = _header.LoadAddress[0] | (_header.LoadAddress[1] << 8);

		// Make RST xx calls jump to [LOAD+0/$8/$10/etc]
		// This allows music drivers to use RST instructions
		for (int i = 0; i <= 0x38; i += 8) {
			// JP LoadAddress+i
			prg[i + 0] = 0xC3;
			prg[i + 1] = (loadAddress + i) & 0xFF;
			prg[i + 2] = ((loadAddress + i) >> 8) & 0xFF;
		}

		// Vertical Blank IRQ: CALL [PLAY]
		prg[0x40] = 0xCD;
		prg[0x41] = _header.PlayAddress[0];
		prg[0x42] = _header.PlayAddress[1];
		prg[0x43] = 0xD9; // RETI

		// Stat IRQ: CALL [PLAY]
		prg[0x48] = 0xCD;
		prg[0x49] = _header.PlayAddress[0];
		prg[0x4A] = _header.PlayAddress[1];
		prg[0x4B] = 0xD9; // RETI

		// Timer IRQ: CALL [PLAY]
		prg[0x50] = 0xCD;
		prg[0x51] = _header.PlayAddress[0];
		prg[0x52] = _header.PlayAddress[1];
		prg[0x53] = 0xD9; // RETI

		// Joypad/Serial IRQ - just RETI (not used)
		prg[0x58] = 0xD9;
		prg[0x60] = 0xD9;

		// Init (PC starts at 0x70): CALL [INIT], then infinite loop
		prg[0x70] = 0xCD;
		prg[0x71] = _header.InitAddress[0];
		prg[0x72] = _header.InitAddress[1];
		// Infinite loop (interrupts will call PLAY)
		prg[0x73] = 0xC3;
		prg[0x74] = 0x73;
		prg[0x75] = 0x00;

		// Disable boot ROM
		_memoryManager->WriteRegister(0xFF50, 0x01);

		// Enable timer, vblank and scanline IRQs
		// (enabling joypad IRQ breaks some GBS files - no handler defined)
		_memoryManager->WriteRegister(0xFFFF, GbIrqSource::Timer | GbIrqSource::LcdStat | GbIrqSource::VerticalBlank);

		// Enable APU and all sound channels
		_memoryManager->WriteRegister(0xFF26, 0x80);
		_memoryManager->WriteRegister(0xFF25, 0xFF);

		// Clear all IRQ requests (needed when switching tracks)
		_memoryManager->ClearIrqRequest(0xFF);

		if ((_header.TimerControl & 0x04) == 0) {
			// VBlank-based timing: enable PPU
			// Turn off PPU to restart at top of frame
			_memoryManager->WriteRegister(0xFF40, 0x00);
			// Turn on PPU for vblank interrupts
			_memoryManager->WriteRegister(0xFF40, 0x80);
		} else {
			// Timer-based timing: disable PPU
			_memoryManager->WriteRegister(0xFF40, 0x00);

			// Use timer for IRQs
			if ((_header.TimerControl & 0x80) && !_memoryManager->IsHighSpeed()) {
				// Enable CGB double-speed mode if requested
				_memoryManager->ToggleSpeed();
			}

			// Configure timer registers
			_memoryManager->WriteRegister(0xFF06, _header.TimerModulo);
			_memoryManager->WriteRegister(0xFF07, _header.TimerControl & 0x07);
		}

		// Reset CPU for fresh playback
		_gameboy->GetCpu()->PowerOn();

		// Set up CPU state for INIT call
		GbCpuState& state = _gameboy->GetCpu()->GetState();
		state = {};
		state.SP = _header.StackPointer[0] | (_header.StackPointer[1] << 8);
		state.PC = 0x70;  // Entry point
		state.A = (uint8_t)selectedTrack;  // Track number in A register
		state.IME = true; // Enable interrupts

		_startClock = _gameboy->GetMasterClock();
		_prgBank = 1;
		RefreshMappings();
	}

	/// <summary>
	/// Gets current track information for audio player UI.
	/// </summary>
	/// <returns>Track info with title, artist, position.</returns>
	AudioTrackInfo GetAudioTrackInfo() {
		AudioTrackInfo info = {};
		info.GameTitle = string(_header.Title, 32);
		info.Artist = string(_header.Author, 32);
		info.Comment = string(_header.Copyright, 32);
		info.TrackNumber = _currentTrack + 1;
		info.TrackCount = _header.TrackCount;
		info.Position = (double)(_gameboy->GetMasterClock() - _startClock) / _gameboy->GetMasterClockRate();
		info.Length = 0;
		info.FadeLength = 0;
		return info;
	}

	/// <summary>
	/// Handles audio player commands (next/prev track, etc).
	/// </summary>
	/// <param name="p">Action parameters.</param>
	void ProcessAudioPlayerAction(AudioPlayerActionParams p) {
		int selectedTrack = _currentTrack;
		switch (p.Action) {
			case AudioPlayerAction::NextTrack:
				selectedTrack++;
				break;
			case AudioPlayerAction::PrevTrack:
				// If early in track, go to previous; else restart current
				if (GetAudioTrackInfo().Position < 2) {
					selectedTrack--;
				}
				break;
			case AudioPlayerAction::SelectTrack:
				selectedTrack = (int)p.TrackNumber;
				break;
		}

		// Wrap around track list
		if (selectedTrack < 0) {
			selectedTrack = _header.TrackCount - 1;
		} else if (selectedTrack >= _header.TrackCount) {
			selectedTrack = 0;
		}

		// Asynchronously switch tracks (avoids recursion issues)
		Emulator* emu = _gameboy->GetEmulator();
		thread switchTrackTask([emu, selectedTrack]() {
			auto lock = emu->AcquireLock(false);
			((Gameboy*)emu->GetConsole().get())->InitGbsPlayback((uint8_t)selectedTrack);
		});
		switchTrackTask.detach();
	}

	/// <summary>
	/// Updates memory mappings for music data access.
	/// </summary>
	void RefreshMappings() override {
		constexpr int prgBankSize = 0x4000;
		// Fixed bank 0 at $0000-$3FFF
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, 0, true);
		// Switchable bank for music data
		Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * prgBankSize, true);
		// RAM for music driver state
		Map(0xA000, 0xFFFF, GbMemoryType::CartRam, 0, false);
	}

	/// <summary>
	/// Writes to bank select (MBC1-compatible).
	/// </summary>
	/// <param name="addr">Register address ($2000-$3FFF).</param>
	/// <param name="value">Bank number (0→1).</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		_prgBank = std::max<uint8_t>(1, value);
		RefreshMappings();
	}

	/// <summary>Serializes GBS player state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_prgBank);
		SV(_currentTrack);
		SV(_startClock);
	}
};