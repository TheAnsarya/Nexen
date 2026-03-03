#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/RomHandler.h"
#include "SNES/MemoryMappings.h"
#include "Shared/Emulator.h"
#include "Shared/BatteryManager.h"
#include "Shared/FirmwareHelper.h"
#include "Utilities/VirtualFile.h"

/// <summary>
/// Message structure for prompting user to select Sufami Turbo slot B cartridge.
/// </summary>
struct SufamiTurboFilePromptMessage {
	/// <summary>Buffer for the selected filename (path to slot B ROM).</summary>
	char Filename[5000];
};

/// <summary>
/// Sufami Turbo add-on emulation.
/// The Sufami Turbo is a Bandai mini-cartridge adapter for the SNES that allows:
/// - Two small game cartridges to be inserted simultaneously
/// - Data linking between games (e.g., combining SD Gundam units)
/// - Battery-backed save RAM on supported cartridges
/// The adapter contains its own BIOS/firmware ROM required for operation.
/// </summary>
/// <remarks>
/// The Sufami Turbo was only released in Japan and uses proprietary mini-cartridges.
/// Games include SD Ultra Battle series, Poi Poi Ninja World, and SD Gundam Generation.
/// Slot A is required (main game), Slot B is optional (for linking features).
/// </remarks>
class SufamiTurbo {
private:
	/// <summary>Pointer to main emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Name of cartridge in slot A.</summary>
	string _nameSlotA;

	/// <summary>Sufami Turbo BIOS/firmware ROM.</summary>
	uint8_t* _firmware = nullptr;

	/// <summary>Size of firmware ROM.</summary>
	uint32_t _firmwareSize = 0;

	/// <summary>Name of cartridge in slot B.</summary>
	string _cartName;

	/// <summary>Slot B cartridge ROM data.</summary>
	uint8_t* _cartRom = nullptr;

	/// <summary>Size of slot B cartridge ROM.</summary>
	uint32_t _cartRomSize = 0;

	/// <summary>Slot B cartridge save RAM.</summary>
	uint8_t* _cartRam = nullptr;

	/// <summary>Size of slot B save RAM.</summary>
	uint32_t _cartRamSize = 0;

	/// <summary>Memory handlers for firmware ROM.</summary>
	vector<unique_ptr<IMemoryHandler>> _firmwareHandlers;

	/// <summary>Memory handlers for slot B ROM.</summary>
	vector<unique_ptr<IMemoryHandler>> _cartRomHandlers;

	/// <summary>Memory handlers for slot B save RAM.</summary>
	vector<unique_ptr<IMemoryHandler>> _cartRamHandlers;

	/// <summary>Private default constructor (use Init() factory method).</summary>
	SufamiTurbo() {}

public:
	/// <summary>
	/// Factory method to create and initialize a Sufami Turbo instance.
	/// Loads the required firmware and prompts for optional slot B cartridge.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="slotA">Virtual file for slot A cartridge ROM.</param>
	/// <returns>Initialized SufamiTurbo or nullptr if firmware not found.</returns>
	static SufamiTurbo* Init(Emulator* emu, VirtualFile& slotA) {
		vector<uint8_t> firmware;
		if (!FirmwareHelper::LoadSufamiTurboFirmware(emu, firmware)) {
			return nullptr;
		}

		SufamiTurbo* st = new SufamiTurbo();
		st->_emu = emu;
		st->_nameSlotA = FolderUtilities::GetFilename(slotA.GetFileName(), false);

		st->_firmwareSize = (uint32_t)firmware.size();
		st->_firmware = new uint8_t[st->_firmwareSize];
		memcpy(st->_firmware, firmware.data(), firmware.size());
		BaseCartridge::EnsureValidPrgRomSize(st->_firmwareSize, st->_firmware);
		emu->RegisterMemory(MemoryType::SufamiTurboFirmware, st->_firmware, st->_firmwareSize);
		for (uint32_t i = 0; i < st->_firmwareSize; i += 0x1000) {
			st->_firmwareHandlers.push_back(unique_ptr<RomHandler>(new RomHandler(st->_firmware, i, st->_firmwareSize, MemoryType::SufamiTurboFirmware)));
		}

		SufamiTurboFilePromptMessage msg = {};
		emu->GetNotificationManager()->SendNotification(ConsoleNotificationType::SufamiTurboFilePrompt, &msg);

		string slot2File = string(msg.Filename, strlen(msg.Filename));
		if (slot2File.size()) {
			VirtualFile file = slot2File;
			if (file.IsValid()) {
				vector<uint8_t> cart;
				(void)file.ReadFile(cart);

				st->_cartName = FolderUtilities::GetFilename(file.GetFileName(), false);
				if (st->_nameSlotA == st->_cartName) {
					st->_cartName += "_SlotB";
				}

				st->_cartRomSize = (uint32_t)cart.size();
				st->_cartRom = new uint8_t[st->_cartRomSize];
				memcpy(st->_cartRom, cart.data(), cart.size());
				BaseCartridge::EnsureValidPrgRomSize(st->_cartRomSize, st->_cartRom);

				emu->RegisterMemory(MemoryType::SufamiTurboSecondCart, st->_cartRom, st->_cartRomSize);

				for (uint32_t i = 0; i < st->_cartRomSize; i += 0x1000) {
					st->_cartRomHandlers.push_back(unique_ptr<RomHandler>(new RomHandler(st->_cartRom, i, st->_cartRomSize, MemoryType::SufamiTurboSecondCart)));
				}

				st->_cartRamSize = GetSaveRamSize(cart);
				st->_cartRam = new uint8_t[st->_cartRamSize];
				emu->RegisterMemory(MemoryType::SufamiTurboSecondCartRam, st->_cartRam, st->_cartRamSize);
				memset(st->_cartRam, 0, st->_cartRamSize);

				emu->GetBatteryManager()->LoadBattery(st->_cartName + ".srm", std::span<uint8_t>(st->_cartRam, st->_cartRamSize));

				for (uint32_t i = 0; i < st->_cartRamSize; i += 0x1000) {
					st->_cartRamHandlers.push_back(unique_ptr<RamHandler>(new RamHandler(st->_cartRam, i, st->_cartRamSize, MemoryType::SufamiTurboSecondCartRam)));
				}
			}
		}

		return st;
	}

	/// <summary>
	/// Determines save RAM size based on game markers in the ROM.
	/// Different Sufami Turbo games use different save RAM sizes.
	/// </summary>
	/// <param name="cart">Cartridge ROM data.</param>
	/// <returns>Save RAM size in bytes (0, 0x800, or 0x2000).</returns>
	static uint32_t GetSaveRamSize(vector<uint8_t>& cart) {
		auto checkMarker = [&](string marker) {
			return std::search((char*)cart.data(), (char*)cart.data() + cart.size(), marker.c_str(), marker.c_str() + marker.size()) != (char*)cart.data() + cart.size();
		};

		// Poi Poi Ninja World and SD Battle games use 2KB save RAM
		if (checkMarker("POIPOI.Ver") || checkMarker("SDBATTLE ")) {
			return 0x800;
		// SD Gundam Generation uses 8KB save RAM
		} else if (checkMarker("SD \xB6\xDE\xDD\xC0\xDE\xD1 GN")) {
			// SD ｶﾞﾝﾀﾞﾑ GN
			return 0x2000;
		}

		// Other games have no save RAM
		return 0;
	}

	/// <summary>
	/// Sets up memory mappings for the Sufami Turbo adapter.
	/// Maps firmware, slot A/B ROMs, and save RAMs to SNES address space.
	/// </summary>
	/// <param name="mm">Memory mappings to configure.</param>
	/// <param name="prgRomHandlers">Slot A ROM handlers.</param>
	/// <param name="saveRamHandlers">Slot A save RAM handlers.</param>
	void InitializeMappings(MemoryMappings& mm, vector<unique_ptr<IMemoryHandler>>& prgRomHandlers, vector<unique_ptr<IMemoryHandler>>& saveRamHandlers) {
		// Slot A ROM: $20-$3F:$8000-$FFFF, $A0-$BF:$8000-$FFFF
		mm.RegisterHandler(0x20, 0x3F, 0x8000, 0xFFFF, prgRomHandlers);
		mm.RegisterHandler(0xA0, 0xBF, 0x8000, 0xFFFF, prgRomHandlers);

		// Slot A save RAM: $60-$63:$8000-$FFFF, $E0-$E3:$8000-$FFFF
		mm.RegisterHandler(0x60, 0x63, 0x8000, 0xFFFF, saveRamHandlers);
		mm.RegisterHandler(0xE0, 0xE3, 0x8000, 0xFFFF, saveRamHandlers);

		// Firmware: $00-$1F:$8000-$FFFF, $80-$9F:$8000-$FFFF
		mm.RegisterHandler(0x00, 0x1F, 0x8000, 0xFFFF, _firmwareHandlers);
		mm.RegisterHandler(0x80, 0x9F, 0x8000, 0xFFFF, _firmwareHandlers);

		// Slot B ROM: $40-$5F:$8000-$FFFF, $C0-$DF:$8000-$FFFF
		mm.RegisterHandler(0x40, 0x5F, 0x8000, 0xFFFF, _cartRomHandlers);
		mm.RegisterHandler(0xC0, 0xDF, 0x8000, 0xFFFF, _cartRomHandlers);

		// Slot B save RAM: $70-$73:$8000-$FFFF, $F0-$F3:$8000-$FFFF
		mm.RegisterHandler(0x70, 0x73, 0x8000, 0xFFFF, _cartRamHandlers);
		mm.RegisterHandler(0xF0, 0xF3, 0x8000, 0xFFFF, _cartRamHandlers);
	}

	/// <summary>Saves slot B battery-backed RAM to file.</summary>
	void SaveBattery() {
		if (_cartRam) {
			_emu->GetBatteryManager()->SaveBattery(_cartName + ".srm", std::span<const uint8_t>(_cartRam, _cartRamSize));
		}
	}

	/// <summary>Destructor - releases all allocated memory.</summary>
	~SufamiTurbo() {
		delete[] _firmware;
		delete[] _cartRom;
		delete[] _cartRam;
	}
};