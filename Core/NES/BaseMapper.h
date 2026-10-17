#pragma once

#include "pch.h"
#include "NES/INesMemoryHandler.h"
#include "NES/NesTypes.h"
#include "NES/RomData.h"
#include "Debugger/DebugTypes.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryOperationType.h"
#include "Utilities/ISerializable.h"

class NesConsole;
class Epsm;
enum class MemoryType;
struct MapperStateEntry;

/// <summary>
/// Base class for all NES cartridge mappers.
/// Handles memory mapping, bank switching, and cartridge-specific hardware.
/// </summary>
/// <remarks>
/// NES cartridges contain mapper chips that control how the CPU and PPU access
/// ROM and RAM. The mapper translates addresses to physical memory locations.
///
/// **Memory Spaces:**
/// - **PRG ROM** ($8000-$FFFF): Program code, typically bank-switched
/// - **PRG RAM** ($6000-$7FFF): Work RAM or battery-backed save RAM
/// - **CHR ROM/RAM** ($0000-$1FFF PPU): Pattern tables for tiles/sprites
/// - **Nametables** ($2000-$2FFF PPU): Tile arrangement for backgrounds
///
/// **Key Concepts:**
/// - **Bank Switching**: Swap different ROM sections into address windows
/// - **Mirroring**: Horizontal, vertical, or 4-screen nametable mirroring
/// - **IRQ Generation**: Scanline counters for timed effects (MMC3, VRC)
/// - **Expansion Audio**: Additional sound channels (VRC6, VRC7, Sunsoft 5B)
///
/// **Architecture:**
/// - Derived classes implement InitMapper(), WriteRegister(), ReadRegister()
/// - Memory page arrays (_prgPages[], _chrPages[]) point to ROM/RAM regions
/// - SelectPrgPage/SelectChrPage helpers manage bank switching
/// - Serialize() handles save state persistence
///
/// **Common Mapper Types:**
/// - NROM (0): No mapper, simple 32KB PRG + 8KB CHR
/// - MMC1 (1): Bank switching + WRAM + mirroring control
/// - MMC3 (4): Scanline counter + fine-grained banking
/// - VRC6 (24): Konami mapper with expansion audio
/// </remarks>
class BaseMapper : public INesMemoryHandler, public ISerializable {
private:
	unique_ptr<Epsm> _epsm;  ///< EPSM (Enhanced PSG Sound Module) for expansion audio

	MirroringType _mirroringType = {};  ///< Current nametable mirroring mode
	string _batteryFilename;             ///< Path for battery-backed save file

	// Internal page size getters (validated versions of virtual methods)
	uint16_t InternalGetPrgPageSize();
	uint16_t InternalGetSaveRamPageSize();
	uint16_t InternalGetWorkRamPageSize();
	uint16_t InternalGetChrRomPageSize();
	uint16_t InternalGetChrRamPageSize();
	bool ValidateAddressRange(uint16_t startAddr, uint16_t endAddr);

	uint8_t* _nametableRam = nullptr;   ///< Nametable RAM (CIRAM or cartridge RAM)
	uint8_t _nametableCount = 2;        ///< Number of nametables (2 or 4)
	uint32_t _ntRamSize = 0;            ///< Total nametable RAM size

	uint32_t _internalRamMask = 0x7FF;  ///< Mask for internal RAM mirroring

	bool _hasBusConflicts = false;      ///< ROM has bus conflict behavior
	bool _hasDefaultWorkRam = false;    ///< Mapper has default work RAM at $6000

	bool _hasCustomReadRam = false;     ///< Uses custom CPU ReadRam logic (FDS, Rainbow)
	bool _hasCustomReadVram = false;    ///< Uses custom VRAM read logic
	bool _hasCpuClockHook = false;      ///< Requires CPU clock notification
	bool _hasVramAddressHook = false;   ///< Requires VRAM address change notification

	bool _allowRegisterRead = false;    ///< Mapper supports register reads
	bool _isReadRegisterAddr[0x10000] = {};   ///< Address is a read register
	bool _isWriteRegisterAddr[0x10000] = {};  ///< Address is a write register

	/// PRG memory page pointers (256 x 256-byte pages = 64KB address space)
	MemoryAccessType _prgMemoryAccess[0x100] = {};
	uint8_t* _prgPages[0x100] = {};

	/// CHR memory page pointers (256 x 256-byte pages for PPU address space)
	MemoryAccessType _chrMemoryAccess[0x100] = {};
	uint8_t* _chrPages[0x100] = {};

	/// PRG memory offset tracking for debugger absolute address calculation
	int32_t _prgMemoryOffset[0x100] = {};
	PrgMemoryType _prgMemoryType[0x100] = {};

	/// CHR memory offset tracking for debugger absolute address calculation
	int32_t _chrMemoryOffset[0x100] = {};
	ChrMemoryType _chrMemoryType[0x100] = {};

	vector<uint8_t> _originalPrgRom;  ///< Original PRG ROM for diff/revert
	vector<uint8_t> _originalChrRom;  ///< Original CHR ROM for diff/revert

protected:
	NesRomInfo _romInfo = {};  ///< ROM header info (mapper, mirroring, sizes)

	NesConsole* _console = nullptr;  ///< Parent console reference
	Emulator* _emu = nullptr;        ///< Emulator instance for debugger hooks

	// ROM data pointers and sizes
	uint8_t* _prgRom = nullptr;   ///< PRG ROM data
	uint8_t* _chrRom = nullptr;   ///< CHR ROM data (pattern tables)
	uint8_t* _chrRam = nullptr;   ///< CHR RAM (when cartridge has no CHR ROM)
	uint32_t _prgSize = 0;        ///< PRG ROM size in bytes
	uint32_t _chrRomSize = 0;     ///< CHR ROM size in bytes
	uint32_t _chrRamSize = 0;     ///< CHR RAM size in bytes

	// RAM data pointers and sizes
	uint8_t* _saveRam = nullptr;    ///< Battery-backed save RAM
	uint32_t _saveRamSize = 0;      ///< Save RAM size in bytes
	uint32_t _workRamSize = 0;      ///< Work RAM size in bytes
	uint8_t* _workRam = nullptr;    ///< Work RAM (non-battery-backed)
	bool _hasChrBattery = false;    ///< CHR RAM is battery-backed

	uint8_t* _mapperRam = nullptr;  ///< Mapper-specific RAM
	uint32_t _mapperRamSize = 0;    ///< Mapper RAM size

	/// Initialize mapper state (called after ROM load)
	virtual void InitMapper() = 0;
	virtual void InitMapper(RomData& romData);

	/// Get PRG page size for bank switching (typically 8KB, 16KB, or 32KB)
	virtual uint16_t GetPrgPageSize() = 0;
	/// Get CHR page size for bank switching (typically 1KB, 2KB, or 8KB)
	virtual uint16_t GetChrPageSize() = 0;

	[[nodiscard]] bool IsNes20();

	virtual uint16_t GetChrRamPageSize() { return GetChrPageSize(); }

	// Save ram is battery backed and saved to disk
	virtual uint32_t GetSaveRamSize() { return 0x2000; }
	virtual uint32_t GetSaveRamPageSize() { return 0x2000; }
	virtual bool ForceChrBattery() { return false; }

	virtual bool ForceSaveRamSize() { return false; }
	virtual bool ForceWorkRamSize() { return false; }

	virtual uint32_t GetChrRamSize() { return 0x0000; }

	// Work ram is NOT saved - aka Expansion ram, etc.
	virtual uint32_t GetWorkRamSize() { return 0x2000; }
	virtual uint32_t GetWorkRamPageSize() { return 0x2000; }

	virtual uint32_t GetMapperRamSize() { return 0; }

	virtual uint16_t RegisterStartAddress() { return 0x8000; }
	virtual uint16_t RegisterEndAddress() { return 0xFFFF; }
	virtual bool AllowRegisterRead() { return false; }

	virtual bool EnableCpuClockHook() { return false; }
	virtual bool EnableCustomReadRam() { return false; }
	virtual bool EnableCustomVramRead() { return false; }
	virtual bool EnableVramAddressHook() { return false; }

	virtual uint32_t GetDipSwitchCount() { return 0; }
	virtual uint32_t GetNametableCount() { return 0; }

	[[nodiscard]] virtual bool HasBusConflicts() { return false; }

	uint8_t InternalReadRam(uint16_t addr);

	virtual void WriteRegister(uint16_t addr, uint8_t value);
	virtual uint8_t ReadRegister(uint16_t addr);

	void SelectPrgPage4x(uint16_t slot, uint16_t page, PrgMemoryType memoryType = PrgMemoryType::PrgRom);
	void SelectPrgPage2x(uint16_t slot, uint16_t page, PrgMemoryType memoryType = PrgMemoryType::PrgRom);
	virtual void SelectPrgPage(uint16_t slot, uint16_t page, PrgMemoryType memoryType = PrgMemoryType::PrgRom);
	void SetCpuMemoryMapping(uint16_t startAddr, uint16_t endAddr, int16_t pageNumber, PrgMemoryType type, int8_t accessType = -1);
	void SetCpuMemoryMapping(uint16_t startAddr, uint16_t endAddr, PrgMemoryType type, uint32_t sourceOffset, int8_t accessType);
	void SetCpuMemoryMapping(uint16_t startAddr, uint16_t endAddr, uint8_t* source, uint32_t sourceOffset, uint32_t sourceSize, int8_t accessType = -1);
	void RemoveCpuMemoryMapping(uint16_t startAddr, uint16_t endAddr);

	virtual void SelectChrPage8x(uint16_t slot, uint16_t page, ChrMemoryType memoryType = ChrMemoryType::Default);
	virtual void SelectChrPage4x(uint16_t slot, uint16_t page, ChrMemoryType memoryType = ChrMemoryType::Default);
	virtual void SelectChrPage2x(uint16_t slot, uint16_t page, ChrMemoryType memoryType = ChrMemoryType::Default);
	virtual void SelectChrPage(uint16_t slot, uint16_t page, ChrMemoryType memoryType = ChrMemoryType::Default);
	void SetPpuMemoryMapping(uint16_t startAddr, uint16_t endAddr, uint16_t pageNumber, ChrMemoryType type = ChrMemoryType::Default, int8_t accessType = -1);
	void SetPpuMemoryMapping(uint16_t startAddr, uint16_t endAddr, ChrMemoryType type, uint32_t sourceOffset, int8_t accessType);
	void SetPpuMemoryMapping(uint16_t startAddr, uint16_t endAddr, uint8_t* sourceMemory, uint32_t sourceOffset, uint32_t sourceSize, int8_t accessType = -1);
	void RemovePpuMemoryMapping(uint16_t startAddr, uint16_t endAddr);

	[[nodiscard]] bool HasBattery();
	virtual void LoadBattery();
	string GetBatteryFilename();

	uint32_t GetPrgPageCount();
	uint32_t GetChrRomPageCount();

	uint8_t GetPowerOnByte(uint8_t defaultValue = 0);
	uint32_t GetDipSwitches();

	void SetupDefaultWorkRam();

	void InitializeChrRam(int32_t chrRamSize = -1);

	void AddRegisterRange(uint16_t startAddr, uint16_t endAddr, MemoryOperation operation = MemoryOperation::Any);
	void RemoveRegisterRange(uint16_t startAddr, uint16_t endAddr, MemoryOperation operation = MemoryOperation::Any);

	void Serialize(Serializer& s) override;

	void RestorePrgChrState();

	void BaseProcessCpuClock();

	uint8_t* GetNametable(uint8_t nametableIndex);
	void SetNametable(uint8_t index, uint8_t nametableIndex);
	void SetNametables(uint8_t nametable1Index, uint8_t nametable2Index, uint8_t nametable3Index, uint8_t nametable4Index);
	void SetMirroringType(MirroringType type);
	MirroringType GetMirroringType();

	void InternalWriteVram(uint16_t addr, uint8_t value);

	__forceinline uint8_t InternalReadVram(uint16_t addr) {
		if (_chrMemoryAccess[addr >> 8] & MemoryAccessType::Read) {
			return _chrPages[addr >> 8][(uint8_t)addr];
		}

		// Open bus - "When CHR is disabled, the pattern tables are open bus. Theoretically, this should return the LSB of the address read, but real-world behavior varies."
		return addr;
	}

	virtual vector<MapperStateEntry> GetMapperStateEntries() { return {}; }

	void LoadRomPatch(vector<uint8_t>& orgPrgRom, vector<uint8_t>* orgChrRom = nullptr);
	void SaveRom(vector<uint8_t>& orgPrgRom, vector<uint8_t>* orgChrRom = nullptr);
	void SerializeRomDiff(Serializer& s, vector<uint8_t>& orgPrgRom, vector<uint8_t>* orgChrRom = nullptr);

public:
	static constexpr uint32_t NametableSize = 0x400;

	void Initialize(NesConsole* console, RomData& romData);
	void InitSpecificMapper(RomData& romData);

	BaseMapper();
	virtual ~BaseMapper();
	virtual void Reset(bool softReset);
	virtual void OnAfterResetPowerOn() {}

	GameSystem GetGameSystem();
	PpuModel GetPpuModel();

	Epsm* GetEpsm() { return _epsm.get(); }

	[[nodiscard]] bool HasDefaultWorkRam();

	void SetRegion(ConsoleRegion region);

	[[nodiscard]] __forceinline bool HasCustomReadRam() { return _hasCustomReadRam; }

	/// <summary>
	/// Fast-path CPU read for the common case where ReadRam is not overridden.
	/// Inlines the page table lookup to avoid virtual dispatch overhead.
	/// Mirrors the ReadVram/InternalReadVram pattern for VRAM reads.
	/// </summary>
	__forceinline uint8_t ReadRamFast(uint16_t addr) {
		if (!_hasCustomReadRam) [[likely]] {
			if (_allowRegisterRead && _isReadRegisterAddr[addr]) [[unlikely]] {
				return ReadRegister(addr);
			}
			if (_prgMemoryAccess[addr >> 8] & MemoryAccessType::Read) [[likely]] {
				return _prgPages[addr >> 8][(uint8_t)addr];
			}
			// Open bus fallback: use qualified (non-virtual) call to avoid
			// repeating NesMemoryManager::GetOpenBus() header dependency.
			return BaseMapper::ReadRam(addr);
		} else {
			return ReadRam(addr);
		}
	}

	[[nodiscard]] __forceinline bool HasCpuClockHook() { return _hasCpuClockHook; }
	virtual void ProcessCpuClock();

	[[nodiscard]] __forceinline bool HasVramAddressHook() { return _hasVramAddressHook; }
	virtual void NotifyVramAddressChange(uint16_t addr);

	virtual void GetMemoryRanges(MemoryRanges& ranges) override;
	virtual uint32_t GetInternalRamSize() { return 0x800; }

	virtual void SaveBattery();

	NesRomInfo GetRomInfo();
	uint32_t GetMapperDipSwitchCount();

	uint8_t ReadRam(uint16_t addr) override;
	uint8_t PeekRam(uint16_t addr) override;
	uint8_t DebugReadRam(uint16_t addr);
	void WriteRam(uint16_t addr, uint8_t value) override;
	void DebugWriteRam(uint16_t addr, uint8_t value);
	void WritePrgRam(uint16_t addr, uint8_t value);

	virtual uint8_t MapperReadVram(uint16_t addr, MemoryOperationType operationType);
	virtual void MapperWriteVram(uint16_t addr, uint8_t value);

	__forceinline uint8_t ReadVram(uint16_t addr, MemoryOperationType type = MemoryOperationType::PpuRenderingRead) {
		uint8_t value;
		if (!_hasCustomReadVram) {
			value = InternalReadVram(addr);
		} else {
			value = MapperReadVram(addr, type);
		}
		_emu->ProcessPpuRead<CpuType::Nes>(addr, value, MemoryType::NesPpuMemory, type);
		return value;
	}

	void DebugWriteVram(uint16_t addr, uint8_t value, bool disableSideEffects = true);
	void WriteVram(uint16_t addr, uint8_t value);

	uint8_t DebugReadVram(uint16_t addr, bool disableSideEffects = true);

	void CopyChrTile(uint32_t address, uint8_t* dest);

	// Debugger Helper Functions
	[[nodiscard]] bool HasChrRam();
	[[nodiscard]] bool HasChrRom();
	uint32_t GetChrRomSize() { return _chrRomSize; }

	CartridgeState GetState();

	AddressInfo GetAbsoluteAddress(uint16_t relativeAddr);
	void GetPpuAbsoluteAddress(uint16_t relativeAddr, AddressInfo& info);
	AddressInfo GetPpuAbsoluteAddress(uint32_t relativeAddr);
	AddressInfo GetRelativeAddress(AddressInfo& addr);
	int32_t GetPpuRelativeAddress(AddressInfo& addr);

	[[nodiscard]] bool IsWriteRegister(uint16_t addr);
	[[nodiscard]] bool IsReadRegister(uint16_t addr);

	void GetRomFileData(vector<uint8_t>& out, bool asIpsFile, uint8_t* header);

	vector<uint8_t> GetPrgChrCopy();
	void RestorePrgChrBackup(vector<uint8_t>& backupData);
	void RevertPrgChrChanges();
	[[nodiscard]] bool HasPrgChrChanges();
	void CopyPrgChrRom(BaseMapper* mapper);
	void SwapMemoryAccess(BaseMapper* sub, bool mainHasAccess);
};
