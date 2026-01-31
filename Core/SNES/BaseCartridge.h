#pragma once
#include "pch.h"
#include "SNES/IMemoryHandler.h"
#include "SNES/CartTypes.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"
#include "Utilities/ISerializable.h"
#include "Shared/RomInfo.h"

class MemoryMappings;
class VirtualFile;
class EmuSettings;
class NecDsp;
class Sa1;
class Gsu;
class St018;
class Cx4;
class SuperGameboy;
class BsxCart;
class BsxMemoryPack;
class Gameboy;
class SnesConsole;
class Emulator;
class SpcFileData;
class SufamiTurbo;
enum class ConsoleRegion;
enum class RamState;

/// <summary>
/// SNES cartridge base class - handles ROM loading, memory mapping, and coprocessors.
/// Supports all SNES cartridge types including special chips and add-ons.
/// </summary>
/// <remarks>
/// **Memory Layout:**
/// - **PRG ROM**: Program ROM, up to 6MB (48Mbit)
/// - **Save RAM**: Battery-backed SRAM, up to 512KB
/// - **Coprocessor RAM**: Additional RAM for enhancement chips
///
/// **Mapping Modes:**
/// - **LoROM**: Banks $00-$7D, $80-$FF with 32KB windows
/// - **HiROM**: Banks $40-$7D, $C0-$FF with 64KB windows
/// - **ExLoROM/ExHiROM**: Extended addressing for large ROMs
/// - **Special mappings**: SA-1, Super FX, BS-X, etc.
///
/// **Supported Coprocessors:**
/// - **DSP-1/2/3/4**: Math coprocessors (fixed-point, matrix ops)
/// - **SA-1**: 10.74 MHz 65816 with bank switching and math
/// - **Super FX (GSU)**: RISC processor for 3D graphics
/// - **Cx4**: Wireframe 3D (Mega Man X2/X3)
/// - **ST010/ST011**: AI processors (racing games)
/// - **S-DD1**: Decompression chip (Star Ocean)
/// - **SPC7110**: Decompression + RTC (Far East of Eden)
/// - **OBC1**: Sprite management (Metal Combat)
/// - **S-RTC**: Real-time clock
///
/// **Add-ons:**
/// - **Super Game Boy**: Play Game Boy games
/// - **Sufami Turbo**: Mini-cartridge adapter
/// - **Satellaview (BS-X)**: Satellite download add-on
///
/// **Battery Management:**
/// - Save RAM persisted to .srm files
/// - RTC data saved for games with real-time clocks
/// </remarks>
class BaseCartridge : public ISerializable {
private:
	Emulator* _emu = nullptr;
	SnesConsole* _console = nullptr;

	vector<unique_ptr<IMemoryHandler>> _prgRomHandlers;
	vector<unique_ptr<IMemoryHandler>> _saveRamHandlers;
	SnesCartInformation _cartInfo = {};
	uint32_t _headerOffset = 0;

	bool _needCoprocSync = false;
	unique_ptr<BaseCoprocessor> _coprocessor;

	NecDsp* _necDsp = nullptr;
	Sa1* _sa1 = nullptr;
	Gsu* _gsu = nullptr;
	Cx4* _cx4 = nullptr;
	St018* _st018 = nullptr;
	SuperGameboy* _sgb = nullptr;
	BsxCart* _bsx = nullptr;
	unique_ptr<BsxMemoryPack> _bsxMemPack;
	unique_ptr<Gameboy> _gameboy;
	unique_ptr<SufamiTurbo> _sufamiTurbo;

	CartFlags::CartFlags _flags = CartFlags::CartFlags::None;
	CoprocessorType _coprocessorType = CoprocessorType::None;
	bool _hasBattery = false;
	bool _hasRtc = false;
	string _romPath;

	uint8_t* _prgRom = nullptr;
	uint8_t* _saveRam = nullptr;

	uint32_t _prgRomSize = 0;
	uint32_t _saveRamSize = 0;
	uint32_t _coprocessorRamSize = 0;

	unique_ptr<SpcFileData> _spcData;
	vector<uint8_t> _embeddedFirmware;

	RamState _ramPowerOnState = {};

	void LoadBattery();

	int32_t GetHeaderScore(uint32_t addr);
	void DisplayCartInfo(bool showCorruptedHeaderWarning);

	bool IsCorruptedHeader();

	CoprocessorType GetCoprocessorType();
	CoprocessorType GetSt01xVersion();
	CoprocessorType GetDspVersion();

	bool MapSpecificCarts(MemoryMappings& mm);
	void MapBsxMemoryPack(MemoryMappings& mm);

	void InitRamPowerOnState();

	void LoadRom();

	void LoadSpc();

	bool LoadSufamiTurbo(VirtualFile& romFile);

	bool LoadGameboy(VirtualFile& romFile);

	void SetupCpuHalt();
	void InitCoprocessor();
	void LoadEmbeddedFirmware();

	string GetCartName();
	string GetGameCode();

public:
	virtual ~BaseCartridge();

	static unique_ptr<BaseCartridge> CreateCartridge(SnesConsole* console, VirtualFile& romFile);

	static void EnsureValidPrgRomSize(uint32_t& size, uint8_t*& rom);

	void Reset();

	void SaveBattery();

	void Init(MemoryMappings& mm);

	SnesCartInformation GetHeader();
	uint32_t GetHeaderOffset();

	RamState GetRamPowerOnState();

	ConsoleRegion GetRegion();
	uint32_t GetCrc32();
	string GetSha1Hash();
	CartFlags::CartFlags GetCartFlags();

	void RegisterHandlers(MemoryMappings& mm);

	uint8_t* DebugGetPrgRom() { return _prgRom; }
	uint8_t* DebugGetSaveRam() { return _saveRam; }
	uint32_t DebugGetPrgRomSize() { return _prgRomSize; }
	uint32_t DebugGetSaveRamSize() { return _saveRamSize; }

	NecDsp* GetDsp();
	Sa1* GetSa1();
	Gsu* GetGsu();
	Cx4* GetCx4();
	St018* GetSt018();
	SuperGameboy* GetSuperGameboy();
	BsxCart* GetBsx();
	BsxMemoryPack* GetBsxMemoryPack();
	Gameboy* GetGameboy();

	void RunCoprocessors();

	__forceinline void SyncCoprocessors() {
		if (_needCoprocSync) {
			_coprocessor->Run();
		}
	}

	BaseCoprocessor* GetCoprocessor();

	vector<unique_ptr<IMemoryHandler>>& GetPrgRomHandlers();
	vector<unique_ptr<IMemoryHandler>>& GetSaveRamHandlers();

	SpcFileData* GetSpcData();

	void Serialize(Serializer& s) override;
};
