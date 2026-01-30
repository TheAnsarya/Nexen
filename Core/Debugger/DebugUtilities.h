#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Shared/MemoryType.h"
#include "Utilities/HexUtilities.h"

/// <summary>
/// Static utility functions for debugger (CPU type conversion, memory classification).
/// </summary>
/// <remarks>
/// Architecture:
/// - Pure static class (no instantiation)
/// - Compile-time constant functions (constexpr)
/// - Platform-agnostic utilities
///
/// CPU type utilities:
/// - GetCpuMemoryType(): MemoryType → CpuType
/// - ToCpuType(): MemoryType → CpuType
/// - GetProgramCounterSize(): CpuType → address width
///
/// Memory classification:
/// - IsRelativeMemory(): Is CPU-addressable memory
/// - IsPpuMemory(): Is PPU memory (VRAM/OAM/palette)
/// - IsRom(): Is read-only memory
/// - IsVolatileRam(): Is volatile (not battery-backed)
///
/// Address formatting:
/// - AddressToHex(): Format address for CPU type (16/20/24/32-bit)
///
/// Platform support:
/// - NES, SNES (+ SA-1/SPC/GSU/etc), GB, GBA, PCE, SMS, WS
/// </remarks>
class DebugUtilities {
public:
	/// <summary>
	/// Get CPU memory type for CPU.
	/// </summary>
	/// <param name="type">CPU type</param>
	/// <returns>Memory type (e.g., SnesMemory for CpuType::Snes)</returns>
	[[nodiscard]] static constexpr MemoryType GetCpuMemoryType(CpuType type) {
		switch (type) {
			case CpuType::Snes:
				return MemoryType::SnesMemory;
			case CpuType::Spc:
				return MemoryType::SpcMemory;
			case CpuType::NecDsp:
				return MemoryType::NecDspMemory;
			case CpuType::Sa1:
				return MemoryType::Sa1Memory;
			case CpuType::Gsu:
				return MemoryType::GsuMemory;
			case CpuType::Cx4:
				return MemoryType::Cx4Memory;
			case CpuType::St018:
				return MemoryType::St018Memory;
			case CpuType::Gameboy:
				return MemoryType::GameboyMemory;
			case CpuType::Nes:
				return MemoryType::NesMemory;
			case CpuType::Pce:
				return MemoryType::PceMemory;
			case CpuType::Sms:
				return MemoryType::SmsMemory;
			case CpuType::Gba:
				return MemoryType::GbaMemory;
			case CpuType::Ws:
				return MemoryType::WsMemory;
		}

		[[unlikely]] throw std::runtime_error("Invalid CPU type");
	}

	/// <summary>
	/// Get program counter display size (hex digits) for CPU.
	/// </summary>
	/// <param name="type">CPU type</param>
	/// <returns>Hex digit count (4=16-bit, 5=20-bit, 6=24-bit, 8=32-bit)</returns>
	[[nodiscard]] static constexpr int GetProgramCounterSize(CpuType type) {
		switch (type) {
			case CpuType::Snes:
				return 6;
			case CpuType::Spc:
				return 4;
			case CpuType::NecDsp:
				return 6;
			case CpuType::Sa1:
				return 6;
			case CpuType::Gsu:
				return 6;
			case CpuType::Cx4:
				return 6;
			case CpuType::St018:
				return 8;
			case CpuType::Gameboy:
				return 4;
			case CpuType::Nes:
				return 4;
			case CpuType::Pce:
				return 4;
			case CpuType::Sms:
				return 4;
			case CpuType::Gba:
				return 8;
			case CpuType::Ws:
				return 5;
		}

		[[unlikely]] throw std::runtime_error("Invalid CPU type");
	}

	/// <summary>
	/// Convert memory type to CPU type.
	/// </summary>
	/// <param name="type">Memory type</param>
	/// <returns>Owning CPU type</returns>
	[[nodiscard]] static constexpr CpuType ToCpuType(MemoryType type) {
		switch (type) {
			case MemoryType::SnesMemory:
			case MemoryType::SnesCgRam:
			case MemoryType::SnesPrgRom:
			case MemoryType::SnesSaveRam:
			case MemoryType::SnesSpriteRam:
			case MemoryType::SnesVideoRam:
			case MemoryType::SnesWorkRam:
			case MemoryType::BsxMemoryPack:
			case MemoryType::BsxPsRam:
			case MemoryType::SufamiTurboFirmware:
			case MemoryType::SufamiTurboSecondCart:
			case MemoryType::SufamiTurboSecondCartRam:
			case MemoryType::SnesRegister:
				return CpuType::Snes;

			case MemoryType::SpcMemory:
			case MemoryType::SpcRam:
			case MemoryType::SpcRom:
			case MemoryType::SpcDspRegisters:
				return CpuType::Spc;

			case MemoryType::GsuMemory:
			case MemoryType::GsuWorkRam:
				return CpuType::Gsu;

			case MemoryType::Sa1InternalRam:
			case MemoryType::Sa1Memory:
				return CpuType::Sa1;

			case MemoryType::NecDspMemory:
			case MemoryType::DspDataRam:
			case MemoryType::DspDataRom:
			case MemoryType::DspProgramRom:
				return CpuType::NecDsp;

			case MemoryType::Cx4DataRam:
			case MemoryType::Cx4Memory:
				return CpuType::Cx4;

			case MemoryType::St018Memory:
			case MemoryType::St018PrgRom:
			case MemoryType::St018DataRom:
			case MemoryType::St018WorkRam:
				return CpuType::St018;

			case MemoryType::GbPrgRom:
			case MemoryType::GbWorkRam:
			case MemoryType::GbCartRam:
			case MemoryType::GbHighRam:
			case MemoryType::GbBootRom:
			case MemoryType::GbVideoRam:
			case MemoryType::GbSpriteRam:
			case MemoryType::GameboyMemory:
				return CpuType::Gameboy;

			case MemoryType::NesChrRam:
			case MemoryType::NesChrRom:
			case MemoryType::NesInternalRam:
			case MemoryType::NesMemory:
			case MemoryType::NesNametableRam:
			case MemoryType::NesMapperRam:
			case MemoryType::NesPaletteRam:
			case MemoryType::NesPpuMemory:
			case MemoryType::NesPrgRom:
			case MemoryType::NesSaveRam:
			case MemoryType::NesSpriteRam:
			case MemoryType::NesSecondarySpriteRam:
			case MemoryType::NesWorkRam:
				return CpuType::Nes;

			case MemoryType::PceMemory:
			case MemoryType::PcePrgRom:
			case MemoryType::PceWorkRam:
			case MemoryType::PceSaveRam:
			case MemoryType::PceCdromRam:
			case MemoryType::PceCardRam:
			case MemoryType::PceAdpcmRam:
			case MemoryType::PceArcadeCardRam:
			case MemoryType::PceVideoRam:
			case MemoryType::PceVideoRamVdc2:
			case MemoryType::PcePaletteRam:
			case MemoryType::PceSpriteRam:
			case MemoryType::PceSpriteRamVdc2:
				return CpuType::Pce;

			case MemoryType::SmsMemory:
			case MemoryType::SmsPrgRom:
			case MemoryType::SmsWorkRam:
			case MemoryType::SmsCartRam:
			case MemoryType::SmsBootRom:
			case MemoryType::SmsVideoRam:
			case MemoryType::SmsPaletteRam:
			case MemoryType::SmsPort:
				return CpuType::Sms;

			case MemoryType::GbaMemory:
			case MemoryType::GbaPrgRom:
			case MemoryType::GbaBootRom:
			case MemoryType::GbaSaveRam:
			case MemoryType::GbaIntWorkRam:
			case MemoryType::GbaExtWorkRam:
			case MemoryType::GbaVideoRam:
			case MemoryType::GbaSpriteRam:
			case MemoryType::GbaPaletteRam:
				return CpuType::Gba;

			case MemoryType::WsMemory:
			case MemoryType::WsPrgRom:
			case MemoryType::WsWorkRam:
			case MemoryType::WsCartRam:
			case MemoryType::WsCartEeprom:
			case MemoryType::WsBootRom:
			case MemoryType::WsInternalEeprom:
			case MemoryType::WsPort:
				return CpuType::Ws;

			[[unlikely]] default:
				throw std::runtime_error("Invalid CPU type");
		}
	}

	/// <summary>
	/// Check if memory type is CPU-relative (addressable by CPU).
	/// </summary>
	[[nodiscard]] static constexpr bool IsRelativeMemory(MemoryType memType) {
		return memType <= GetLastCpuMemoryType();
	}

	/// <summary>
	/// Get last CPU memory type enum value.
	/// </summary>
	[[nodiscard]] static constexpr MemoryType GetLastCpuMemoryType() {
		return MemoryType::WsMemory;
	}

	/// <summary>
	/// Check if memory type is PPU memory (VRAM/OAM/palette).
	/// </summary>
	[[nodiscard]] static constexpr bool IsPpuMemory(MemoryType memType) {
		switch (memType) {
			case MemoryType::SnesVideoRam:
			case MemoryType::SnesSpriteRam:
			case MemoryType::SnesCgRam:
			case MemoryType::GbVideoRam:
			case MemoryType::GbSpriteRam:

			case MemoryType::NesChrRam:
			case MemoryType::NesChrRom:
			case MemoryType::NesSpriteRam:
			case MemoryType::NesPaletteRam:
			case MemoryType::NesNametableRam:
			case MemoryType::NesSecondarySpriteRam:
			case MemoryType::NesPpuMemory:
				return true;

			case MemoryType::PceVideoRam:
			case MemoryType::PceVideoRamVdc2:
			case MemoryType::PcePaletteRam:
			case MemoryType::PceSpriteRam:
			case MemoryType::PceSpriteRamVdc2:
				return true;

			case MemoryType::SmsVideoRam:
			case MemoryType::SmsPaletteRam:
				return true;

			case MemoryType::GbaVideoRam:
			case MemoryType::GbaSpriteRam:
			case MemoryType::GbaPaletteRam:
				return true;

			default:
				return false;
		}
	}

	/// <summary>
	/// Check if memory type is ROM (read-only).
	/// </summary>
	[[nodiscard]] static constexpr bool IsRom(MemoryType memType) {
		switch (memType) {
			case MemoryType::SnesPrgRom:
			case MemoryType::GbPrgRom:
			case MemoryType::GbBootRom:
			case MemoryType::NesPrgRom:
			case MemoryType::NesChrRom:
			case MemoryType::PcePrgRom:
			case MemoryType::DspDataRom:
			case MemoryType::DspProgramRom:
			case MemoryType::St018PrgRom:
			case MemoryType::St018DataRom:
			case MemoryType::SufamiTurboFirmware:
			case MemoryType::SufamiTurboSecondCart:
			case MemoryType::SpcRom:
			case MemoryType::SmsPrgRom:
			case MemoryType::SmsBootRom:
			case MemoryType::GbaPrgRom:
			case MemoryType::GbaBootRom:
			case MemoryType::WsPrgRom:
				return true;

			default:
				return false;
		}
	}

	[[nodiscard]] static constexpr bool IsVolatileRam(MemoryType memType) {
		if (IsRom(memType)) {
			return false;
		}

		switch (memType) {
			case MemoryType::NesSaveRam:
			case MemoryType::GbCartRam:
			case MemoryType::SnesSaveRam:
			case MemoryType::SufamiTurboSecondCartRam:
			case MemoryType::PceSaveRam:
			case MemoryType::SnesRegister:
			case MemoryType::SmsCartRam:
			case MemoryType::GbaSaveRam:
			case MemoryType::WsCartRam:
				return false;

			default:
				return true;
		}
	}

	/// <summary>
	/// Get last CPU type enum value.
	/// </summary>
	[[nodiscard]] static constexpr CpuType GetLastCpuType() {
		return CpuType::Ws;
	}

	/// <summary>
	/// Format address as hexadecimal string for CPU type.
	/// </summary>
	/// <param name="cpuType">CPU type</param>
	/// <param name="address">Address value</param>
	/// <returns>Formatted hex string (e.g., "CAFE" for 16-bit, "12CAFE" for 24-bit)</returns>
	[[nodiscard]] static string AddressToHex(CpuType cpuType, int32_t address) {
		int size = GetProgramCounterSize(cpuType);
		if (size == 4) {
			return HexUtilities::ToHex((uint16_t)address);
		} else if (size == 5) {
			return HexUtilities::ToHex20(address);
		} else if (size == 6) {
			return HexUtilities::ToHex24(address);
		} else if (size == 8) {
			return HexUtilities::ToHex32(address);
		} else {
			return HexUtilities::ToHex(address);
		}
	}

	/// <summary>
	/// Get total memory type count.
	/// </summary>
	/// <returns>Number of memory type enum values</returns>
	[[nodiscard]] static constexpr int GetMemoryTypeCount() {
		return (int)MemoryType::None + 1;
	}
};
