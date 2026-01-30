#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "Core/Debugger/DebugTypes.h"
#include "Shared/Audio/AudioPlayerTypes.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Shared/RomInfo.h"
#include "Shared/TimingInfo.h"
#include "Shared/SaveStateCompatInfo.h"

class BaseControlManager;
class VirtualFile;
class BaseVideoFilter;
struct BaseState;
struct InternalCheatCode;
enum class ConsoleType;
enum class ConsoleRegion;
enum class CpuType : uint8_t;
enum class EmulatorShortcut;
enum class HashType;

/// <summary>
/// Result codes for ROM loading operations.
/// </summary>
enum class LoadRomResult {
	Success,    ///< ROM loaded successfully
	Failure,    ///< ROM loading failed (corrupted, unsupported)
	UnknownType ///< ROM type detection failed
};

/// <summary>
/// PPU frame information for video output and synchronization.
/// </summary>
/// <remarks>
/// Used for:
/// - Video rendering (frame buffer, dimensions)
/// - Emulation statistics (frame count, cycle count)
/// - Overscan detection (first scanline)
/// </remarks>
struct PpuFrameInfo {
	uint8_t* FrameBuffer;     ///< RGB24/ARGB32 pixel data (platform-specific format)
	uint32_t Width;           ///< Frame width in pixels
	uint32_t Height;          ///< Frame height in pixels
	uint32_t FrameBufferSize; ///< Buffer size in bytes
	uint32_t FrameCount;      ///< Total frames emulated since power-on
	uint32_t ScanlineCount;   ///< Scanlines in current frame (NTSC=262, PAL=312)
	int32_t FirstScanline;    ///< First visible scanline (for overscan handling)
	uint32_t CycleCount;      ///< CPU cycles in current frame
};

/// <summary>
/// Shortcut availability state for context-sensitive hotkey filtering.
/// </summary>
enum class ShortcutState {
	Disabled = 0, ///< Shortcut disabled in current context (e.g., no game loaded)
	Enabled = 1,  ///< Shortcut explicitly enabled
	Default = 2   ///< Use default behavior (check global settings)
};

/// <summary>
/// Abstract console interface for platform-agnostic emulation.
/// Implemented by NesConsole, SnesConsole, GameboyConsole, GbaConsole, PceConsole, SmsConsole, WsConsole.
/// </summary>
/// <remarks>
/// Architecture:
/// - Emulator owns IConsole instance (polymorphic console selection)
/// - Console owns CPU, PPU, APU, memory, cartridge components
/// - ISerializable for save states, INotificationListener for event handling
///
/// Lifecycle:
/// 1. Construction (platform-specific: new NesConsole(), etc.)
/// 2. LoadRom() - Initialize cartridge, memory, peripherals
/// 3. RunFrame() - Execute one video frame of emulation
/// 4. SaveBattery() - Persist battery-backed RAM
/// 5. Reset() - Soft reset (clear RAM, reset CPU/PPU state)
/// 6. Destruction - Save battery, release resources
///
/// Thread model:
/// - All methods called from emulation thread (Emulator)
/// - No internal threading (single-threaded determinism)
/// - Synchronization via frame callbacks (RunFrame returns after PPU frame done)
///
/// Supported systems:
/// - NES: 6502 CPU, 2A03 APU, 2C02 PPU
/// - SNES: 65816 CPU, SPC700 APU, S-PPU
/// - Game Boy: Sharp LR35902 CPU, 4-channel APU, LCD controller
/// - GBA: ARM7TDMI CPU, DMA, 2D/3D graphics
/// - PC Engine: HuC6280 CPU, PSG, HuC6270 VDC
/// - Sega Master System: Z80 CPU, PSG, VDP
/// - WonderSwan: NEC V30MZ CPU, WSC
/// </remarks>
class IConsole : public ISerializable, public INotificationListener {
public:
	virtual ~IConsole() {}

	/// <summary>
	/// Soft reset console (equivalent to reset button).
	/// </summary>
	/// <remarks>
	/// Reset behavior:
	/// - Clear work RAM (except battery-backed SRAM)
	/// - Reset CPU state (PC, stack, flags)
	/// - Reset PPU registers and state
	/// - Reset APU state and audio buffers
	/// - Do NOT reload ROM or reset cartridge state
	/// </remarks>
	virtual void Reset() = 0;

	/// <summary>
	/// Load ROM file and initialize console state.
	/// </summary>
	/// <param name="romFile">ROM file (supports .nes, .sfc, .gb, .gba, .pce, .sms, .ws, zip, 7z)</param>
	/// <returns>Success, Failure, or UnknownType</returns>
	/// <remarks>
	/// Loading process:
	/// 1. Detect ROM type (header, file extension, heuristics)
	/// 2. Select mapper/cartridge type
	/// 3. Initialize memory map (PRG/CHR ROM, RAM, SRAM)
	/// 4. Load battery save if exists
	/// 5. Initialize peripherals (controllers, expansion devices)
	/// 6. Reset console to initial state
	///
	/// VirtualFile supports:
	/// - Regular files (.nes, .sfc, etc.)
	/// - Zip/7z archives (automatically extracts)
	/// - Patch files (.ips, .bps, .ups auto-applied)
	/// </remarks>
	virtual LoadRomResult LoadRom(VirtualFile& romFile) = 0;

	/// <summary>
	/// Execute one video frame of emulation.
	/// </summary>
	/// <remarks>
	/// Frame execution:
	/// 1. Run CPU until PPU signals end-of-frame
	/// 2. Mix audio samples for frame duration
	/// 3. Update controller input
	/// 4. Process DMA/HDMA transfers
	/// 5. Return when frame buffer ready
	///
	/// Timing:
	/// - NTSC: 60.0988 FPS, 89342 CPU cycles/frame (NES)
	/// - PAL: 50.0070 FPS, 106392 CPU cycles/frame (NES)
	/// - SNES: 60.0988/50.0070 FPS, variable cycles (SA-1, SDD-1, etc.)
	///
	/// Synchronization:
	/// - Blocking call (returns after frame complete)
	/// - FrameLimiter handles timing outside this method
	/// </remarks>
	virtual void RunFrame() = 0;

	/// <summary>
	/// Save battery-backed RAM to disk (SRAM, EEPROM, flash).
	/// </summary>
	/// <remarks>
	/// Called when:
	/// - Console reset/power-off
	/// - Periodic auto-save (every N seconds)
	/// - Game unload
	/// - Emulator exit
	///
	/// Typical save locations:
	/// - NES: 8KB SRAM (battery-backed PRG-RAM)
	/// - SNES: 8KB-128KB SRAM (LoROM/HiROM, SA-1 BWRAM)
	/// - GB: 8KB-128KB SRAM/EEPROM
	/// - GBA: 64KB SRAM, 512B-128KB EEPROM/flash
	/// </remarks>
	virtual void SaveBattery() = 0;

	/// <summary>
	/// Check if hotkey shortcut is allowed in current console state.
	/// </summary>
	/// <param name="shortcut">Shortcut ID (save state, reset, etc.)</param>
	/// <param name="shortcutParam">Shortcut parameter (e.g., state slot number)</param>
	/// <returns>Disabled, Enabled, or Default</returns>
	/// <remarks>
	/// Context-sensitive shortcuts:
	/// - Load state: disabled if no save exists for slot
	/// - Rewind: disabled if rewinding not enabled
	/// - Movie recording: disabled during netplay
	/// </remarks>
	virtual ShortcutState IsShortcutAllowed(EmulatorShortcut shortcut, uint32_t shortcutParam) { return ShortcutState::Default; }

	/// <summary>
	/// Get controller/input manager for this console.
	/// </summary>
	/// <returns>Platform-specific control manager (NesControlManager, SnesControlManager, etc.)</returns>
	virtual BaseControlManager* GetControlManager() = 0;

	/// <summary>
	/// Get arcade DIP switch configuration (for arcade ROM sets).
	/// </summary>
	/// <returns>DIP switch info or empty struct if not applicable</returns>
	virtual DipSwitchInfo GetDipSwitchInfo() { return {}; }

	/// <summary>
	/// Get console region (NTSC, PAL, Dendy).
	/// </summary>
	/// <returns>Region enum (affects timing, color encoding)</returns>
	virtual ConsoleRegion GetRegion() = 0;

	/// <summary>
	/// Get console type (NES, SNES, GB, GBA, etc.).
	/// </summary>
	virtual ConsoleType GetConsoleType() = 0;

	/// <summary>
	/// Get list of CPU types in this console.
	/// </summary>
	/// <returns>CPU types (single for NES, multiple for SNES with SPC700)</returns>
	virtual vector<CpuType> GetCpuTypes() = 0;

	/// <summary>
	/// Get current master clock cycle count.
	/// </summary>
	/// <returns>Cycle count since power-on</returns>
	virtual uint64_t GetMasterClock() = 0;

	/// <summary>
	/// Get master clock frequency in Hz.
	/// </summary>
	/// <returns>Clock rate (1.789773 MHz for NES NTSC, 21.477272 MHz for SNES)</returns>
	virtual uint32_t GetMasterClockRate() = 0;

	/// <summary>
	/// Get target frames per second.
	/// </summary>
	/// <returns>FPS (60.0988 for NTSC, 50.0070 for PAL)</returns>
	virtual double GetFps() = 0;

	/// <summary>
	/// Get comprehensive timing information for specified CPU.
	/// </summary>
	/// <param name="cpuType">CPU type (for multi-CPU systems like SNES)</param>
	/// <returns>Timing info struct with clocks, FPS, frame/cycle counts</returns>
	virtual TimingInfo GetTimingInfo(CpuType cpuType) {
		TimingInfo info = {};
		info.MasterClock = GetMasterClock();
		info.MasterClockRate = GetMasterClockRate();
		info.Fps = GetFps();

		PpuFrameInfo frameInfo = GetPpuFrame();
		info.FrameCount = frameInfo.FrameCount;
		info.CycleCount = frameInfo.CycleCount;
		info.ScanlineCount = frameInfo.ScanlineCount;
		info.FirstScanline = frameInfo.FirstScanline;
		return info;
	}

	/// <summary>
	/// Get video filter for rendering (NTSC, scanlines, etc.).
	/// </summary>
	/// <param name="getDefaultFilter">True to get default filter, false for custom</param>
	/// <returns>Video filter instance (platform-specific)</returns>
	virtual BaseVideoFilter* GetVideoFilter(bool getDefaultFilter) = 0;

	/// <summary>
	/// Get screen rotation override for rotated displays.
	/// </summary>
	/// <param name="rotation">Rotation angle in degrees (0, 90, 180, 270)</param>
	/// <remarks>
	/// Used for games with portrait orientation (e.g., some arcade ports).
	/// </remarks>
	virtual void GetScreenRotationOverride(uint32_t& rotation) {}

	/// <summary>
	/// Get current PPU frame buffer and statistics.
	/// </summary>
	/// <returns>Frame info with pixel buffer, dimensions, counts</returns>
	virtual PpuFrameInfo GetPpuFrame() = 0;

	/// <summary>
	/// Get ROM hash for identification.
	/// </summary>
	/// <param name="hashType">Hash algorithm (CRC32, MD5, SHA1, SHA256)</param>
	/// <returns>Hash string in hex format</returns>
	virtual string GetHash(HashType hashType) { return {}; }

	/// <summary>
	/// Get loaded ROM format information.
	/// </summary>
	/// <returns>Format info (iNES, FDS, NSF, etc.)</returns>
	virtual RomFormat GetRomFormat() = 0;

	/// <summary>
	/// Get audio track info for multi-track formats (NSF, SPC, etc.).
	/// </summary>
	/// <returns>Track count, current track, duration</returns>
	virtual AudioTrackInfo GetAudioTrackInfo() = 0;

	/// <summary>
	/// Process audio player action (change track, fade, etc.).
	/// </summary>
	/// <param name="p">Action parameters (track number, fade duration)</param>
	virtual void ProcessAudioPlayerAction(AudioPlayerActionParams p) = 0;

	/// <summary>
	/// Convert relative address to absolute address.
	/// </summary>
	/// <param name="relAddress">Relative address (CPU address space)</param>
	/// <returns>Absolute address (ROM file offset, SRAM offset, etc.)</returns>
	virtual AddressInfo GetAbsoluteAddress(AddressInfo& relAddress) = 0;

	/// <summary>
	/// Convert absolute address to relative address.
	/// </summary>
	/// <param name="absAddress">Absolute address (ROM offset)</param>
	/// <param name="cpuType">Target CPU type</param>
	/// <returns>Relative address (CPU address space)</returns>
	virtual AddressInfo GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) = 0;

	/// <summary>
	/// Get complete console state (for save states).
	/// </summary>
	/// <param name="state">Output state struct</param>
	/// <param name="consoleType">Console type filter</param>
	virtual void GetConsoleState(BaseState& state, ConsoleType consoleType) = 0;

	/// <summary>
	/// Validate save state compatibility with current ROM.
	/// </summary>
	/// <param name="stateConsoleType">Console type from save state</param>
	/// <returns>Compatibility info (version, warnings)</returns>
	virtual SaveStateCompatInfo ValidateSaveStateCompatibility(ConsoleType stateConsoleType) { return {}; }

	/// <summary>
	/// Process cheat code on memory write.
	/// </summary>
	/// <param name="code">Cheat code definition</param>
	/// <param name="addr">Memory address being written</param>
	/// <param name="value">Value being written (may be modified)</param>
	virtual void ProcessCheatCode(InternalCheatCode& code, uint32_t addr, uint8_t& value) {}

	/// <summary>
	/// Process console notification event.
	/// </summary>
	/// <param name="type">Notification type</param>
	/// <param name="parameter">Type-specific parameter</param>
	virtual void ProcessNotification(ConsoleNotificationType type, void* parameter) {}
};
