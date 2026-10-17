#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/Carts/GbMbc1.h"
#include "Gameboy/Carts/GbMbc2.h"
#include "Gameboy/Carts/GbMbc3.h"
#include "Gameboy/Carts/GbMbc5.h"
#include "Gameboy/Carts/GbMbc6.h"
#include "Gameboy/Carts/GbMbc7.h"
#include "Gameboy/Carts/GbMmm01.h"
#include "Gameboy/Carts/GbM161.h"
#include "Gameboy/Carts/GbHuc1.h"
#include "Gameboy/Carts/GbWisdomTree.h"
#include "Gameboy/GbxFooter.h"

/// <summary>
/// Factory for creating Game Boy cartridge mapper instances.
/// </summary>
/// <remarks>
/// **Mapper Detection Strategy:**
/// 1. If GBX footer present, use explicit mapper ID
/// 2. Otherwise, detect from cartridge header ($0147)
/// 3. Apply game-specific exceptions for mislabeled carts
///
/// **Supported Mappers:**
/// | Code | Type | Features |
/// |------|------|----------|
/// | $00 | ROM only | No banking |
/// | $01-$03 | MBC1 | 2MB ROM, 32KB RAM |
/// | $05-$06 | MBC2 | 256KB ROM, 512×4-bit RAM |
/// | $0B-$0D | MMM01 | Multi-cart |
/// | $0F-$13 | MBC3 | 2MB ROM, RTC |
/// | $19-$1E | MBC5 | 8MB ROM, rumble |
/// | $20 | MBC6 | Flash ROM |
/// | $22 | MBC7 | Accelerometer, EEPROM |
/// | $FE | HuC3 | RTC (not implemented) |
/// | $FF | HuC1 | IR communication |
///
/// **Special Detections:**
/// - MBC1M: Multi-cart variant detected by logo at $40104
/// - MBC30: Extended MBC3 with 64KB+ RAM
/// - M161: "TETRIS SET" game mislabeled as MBC3
/// </remarks>
class GbCartFactory {
private:
	/// <summary>
	/// Creates mapper from GBX footer information.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="footer">Parsed GBX footer.</param>
	/// <param name="prgRom">ROM data.</param>
	/// <returns>Mapper instance, or nullptr if unknown.</returns>
	static GbCart* LoadFromGbxFooter(Emulator* emu, GbxFooter& footer, vector<uint8_t>& prgRom) {
		string mapperId = footer.GetMapperId();

		// Log GBX cart info
		MessageManager::Log("[GBX] Mapper: " + mapperId);
		MessageManager::Log(string("[GBX] ROM Size: ") + std::to_string(footer.GetRomSize() / 1024) + " KB");
		MessageManager::Log(string("[GBX] RAM Size: ") + std::to_string(footer.GetRamSize() / 1024) + " KB");
		MessageManager::Log("[GBX] Battery: " + string(footer.HasBattery() ? "Yes" : "No"));
		MessageManager::Log("[GBX] RTC: " + string(footer.HasRtc() ? "Yes" : "No"));
		MessageManager::Log("[GBX] Rumble: " + string(footer.HasRumble() ? "Yes" : "No"));

		// Create mapper by explicit ID
		if (mapperId == "ROM") {
			return new GbCart();
		} else if (mapperId == "MBC1") {
			return new GbMbc1(false);
		} else if (mapperId == "MBC2") {
			return new GbMbc2();
		} else if (mapperId == "MBC3") {
			bool isMbc30 = footer.GetRamSize() >= 0x10000;  // 64KB+ RAM = MBC30
			return new GbMbc3(emu, footer.HasRtc(), isMbc30);
		} else if (mapperId == "MBC5") {
			return new GbMbc5(footer.HasRumble());
		} else if (mapperId == "MBC6") {
			return new GbMbc6();
		} else if (mapperId == "MBC7") {
			return new GbMbc7();
		} else if (mapperId == "MB1M") {
			return new GbMbc1(true);  // MBC1 multi-cart wiring
		} else if (mapperId == "HUC1") {
			return new GbHuc1();
		} else if (mapperId == "WISD") {
			return new GbWisdomTree();
		} else if (mapperId == "MMM1") {
			return new GbMmm01();
		} else if (mapperId == "M161") {
			return new GbM161();
		}

		return nullptr;
	}

	/// <summary>
	/// Creates mapper from standard GB cartridge header.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="header">Parsed ROM header.</param>
	/// <param name="prgRom">ROM data (for MBC1M detection).</param>
	/// <returns>Mapper instance, or nullptr if unknown.</returns>
	static GbCart* LoadFromGbHeader(Emulator* emu, GameboyHeader& header, vector<uint8_t>& prgRom) {
		// Check for mislabeled cartridges first
		GbCart* cart = ProcessExceptions(header);
		if (cart) {
			return cart;
		}

		switch (header.CartType) {
			// $00: ROM only (no mapper)
			case 0x00:
				return new GbCart();

			// $01-$03: MBC1 / MBC1+RAM / MBC1+RAM+BATTERY
			case 0x01:
			case 0x02:
			case 0x03: {
				// MBC1M detection: Multi-cart if Nintendo logo appears at $40104
				// (indicating second game starts at $40000)
				bool isMbc1m = prgRom.size() > 0x40134 && memcmp(&prgRom[0x104], &prgRom[0x40104], 0x30) == 0;
				return new GbMbc1(isMbc1m);
			}

			// $05-$06: MBC2 / MBC2+BATTERY
			case 0x05:
			case 0x06:
				return new GbMbc2();

			// $0B-$0D: MMM01 / MMM01+SRAM / MMM01+SRAM+BATTERY
			case 0x0B:
			case 0x0C:
			case 0x0D:
				return new GbMmm01();

			// $0F-$13: MBC3 variants (with/without RTC, RAM)
			case 0x0F:  // MBC3+TIMER+BATTERY
			case 0x10:  // MBC3+TIMER+RAM+BATTERY
			case 0x11:  // MBC3
			case 0x12:  // MBC3+RAM
			case 0x13: { // MBC3+RAM+BATTERY
				bool isMbc30 = header.GetCartRamSize() >= 0x10000;  // 64KB+ = MBC30
				bool hasRtc = header.CartType <= 0x10;  // Only $0F/$10 have RTC
				return new GbMbc3(emu, hasRtc, isMbc30);
			}

			// $19-$1E: MBC5 variants (with/without RAM, rumble)
			case 0x19:  // MBC5
			case 0x1A:  // MBC5+RAM
			case 0x1B:  // MBC5+RAM+BATTERY
			case 0x1C:  // MBC5+RUMBLE
			case 0x1D:  // MBC5+RUMBLE+RAM
			case 0x1E: { // MBC5+RUMBLE+RAM+BATTERY
				bool hasRumble = header.CartType >= 0x1C;
				return new GbMbc5(hasRumble);
			}

			// $20: MBC6 (flash ROM support)
			case 0x20:
				return new GbMbc6();

			// $22: MBC7 (accelerometer + EEPROM)
			case 0x22:
				return new GbMbc7();

			// $FE: HuC3 (RTC - not yet implemented)
			case 0xFE:
				break;

			// $FF: HuC1 (IR communication)
			case 0xFF:
				return new GbHuc1();
		};

		return nullptr;
	}

	/// <summary>
	/// Handles mislabeled cartridges that need special mapper assignment.
	/// </summary>
	/// <param name="header">ROM header.</param>
	/// <returns>Correct mapper if exception found, nullptr otherwise.</returns>
	static GbCart* ProcessExceptions(GameboyHeader& header) {
		// "TETRIS SET" is labeled as MBC3 ($10) but actually uses M161
		if (header.CartType == 0x10 && header.GetCartName() == "TETRIS SET") {
			MessageManager::Log("Auto-detected cart type: M161");
			return new GbM161();
		}
		return nullptr;
	}

public:
	/// <summary>
	/// Creates the appropriate cartridge mapper.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="header">Standard ROM header.</param>
	/// <param name="gbxFooter">Optional GBX footer (may be invalid).</param>
	/// <param name="prgRom">ROM data.</param>
	/// <returns>Mapper instance, or nullptr if unsupported.</returns>
	/// <remarks>
	/// GBX footer takes precedence over header if valid,
	/// as it provides explicit mapper information.
	/// </remarks>
	static GbCart* CreateCart(Emulator* emu, GameboyHeader& header, GbxFooter& gbxFooter, vector<uint8_t>& prgRom) {
		if (gbxFooter.IsValid()) {
			return LoadFromGbxFooter(emu, gbxFooter, prgRom);
		} else {
			return LoadFromGbHeader(emu, header, prgRom);
		}
	}
};
