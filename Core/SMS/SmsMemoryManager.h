#pragma once
#include "pch.h"
#include "SMS/SmsTypes.h"
#include "SMS/SmsVdp.h"
#include "SMS/Carts/SmsCart.h"
#include "Shared/Emulator.h"
#include "Shared/CheatManager.h"
#include "Debugger/AddressInfo.h"
#include "Shared/MemoryOperationType.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

class Emulator;
class SmsConsole;
class SmsControlManager;
class SmsCart;
class SmsPsg;
class SmsFmAudio;
class SmsBiosMapper;

/// <summary>
/// Sega Master System/Game Gear/SG-1000/ColecoVision memory manager.
/// Handles memory mapping, I/O ports, and cartridge banking.
/// </summary>
/// <remarks>
/// **SMS Memory Map:**
/// - $0000-$BFFF: Cartridge ROM (banked via mapper registers)
/// - $C000-$DFFF: System RAM (8KB, mirrored to $FFFF)
/// - Mapper registers at $FFFC-$FFFF (SMS mapper) or $4000-$7FFF (Codemasters)
///
/// **I/O Ports:**
/// - $00-$06: Game Gear specific (link, stereo)
/// - $3E: Memory control (enable/disable ROM/RAM/BIOS)
/// - $3F: I/O port control (nationality, joystick TH)
/// - $7E/$7F: VDP V-counter/H-counter (read)
/// - $7E/$7F: PSG write
/// - $BE/$BF: VDP data/control
/// - $DC/$DD: Joypad ports
/// - $F0-$F2: FM audio (YM2413, Japanese SMS)
///
/// **ColecoVision Map:**
/// - $0000-$1FFF: BIOS ROM
/// - $2000-$7FFF: Expansion
/// - $6000-$7FFF: RAM (1KB mirrored)
/// - $8000-$FFFF: Cartridge ROM
///
/// **SG-1000 Map:**
/// - $0000-$BFFF: Cartridge ROM
/// - $C000-$FFFF: RAM (1KB-8KB depending on cartridge)
/// </remarks>
class SmsMemoryManager final : public ISerializable {
private:
	/// <summary>SMS work RAM size (8KB).</summary>
	static constexpr uint32_t SmsWorkRamSize = 0x2000;

	/// <summary>Maximum cartridge RAM size (32KB).</summary>
	static constexpr uint32_t CartRamMaxSize = 0x8000;

	/// <summary>ColecoVision work RAM size (1KB).</summary>
	static constexpr uint32_t CvWorkRamSize = 0x400;

	/// <summary>Emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Console instance.</summary>
	SmsConsole* _console = nullptr;

	/// <summary>Video Display Processor.</summary>
	SmsVdp* _vdp = nullptr;

	/// <summary>Controller manager.</summary>
	SmsControlManager* _controlManager = nullptr;

	/// <summary>Cartridge handler.</summary>
	SmsCart* _cart = nullptr;

	/// <summary>Programmable Sound Generator.</summary>
	SmsPsg* _psg = nullptr;

	/// <summary>FM audio (YM2413).</summary>
	SmsFmAudio* _fmAudio = nullptr;

	/// <summary>BIOS mapper (optional).</summary>
	unique_ptr<SmsBiosMapper> _biosMapper;

	/// <summary>Memory manager state.</summary>
	SmsMemoryManagerState _state = {};

	/// <summary>Work RAM buffer.</summary>
	uint8_t* _workRam = nullptr;

	/// <summary>Work RAM size.</summary>
	uint32_t _workRamSize = 0;

	/// <summary>Cartridge RAM buffer.</summary>
	uint8_t* _cartRam = nullptr;

	/// <summary>Cartridge RAM size.</summary>
	uint32_t _cartRamSize = 0;

	/// <summary>Original cartridge RAM pointer.</summary>
	uint8_t* _originalCartRam = nullptr;

	/// <summary>Program ROM data.</summary>
	uint8_t* _prgRom = nullptr;

	/// <summary>Program ROM size.</summary>
	uint32_t _prgRomSize = 0;

	/// <summary>BIOS ROM data.</summary>
	uint8_t* _biosRom = nullptr;

	/// <summary>BIOS ROM size.</summary>
	uint32_t _biosRomSize = 0;

	/// <summary>Master clock cycle counter.</summary>
	uint64_t _masterClock = 0;

	/// <summary>Read mapping table (256 pages).</summary>
	uint8_t* _reads[0x100] = {};

	/// <summary>Write mapping table (256 pages).</summary>
	uint8_t* _writes[0x100] = {};

	/// <summary>SG-1000 RAM mapping address (-1 if none).</summary>
	int32_t _sgRamMapAddress = -1;

	/// <summary>Current console model (SMS, GG, SG, CV).</summary>
	SmsModel _model = {};

	/// <summary>Loads battery-backed save RAM.</summary>
	void LoadBattery();

	/// <summary>Internal I/O port read.</summary>
	template <bool isPeek = false>
	__forceinline uint8_t InternalReadPort(uint8_t port);

	/// <summary>SMS-specific port read.</summary>
	template <bool isPeek>
	uint8_t ReadSmsPort(uint8_t port);

	/// <summary>ColecoVision-specific port read.</summary>
	template <bool isPeek>
	uint8_t ReadColecoVisionPort(uint8_t port);

	/// <summary>Game Gear-specific port read.</summary>
	template <bool isPeek>
	uint8_t ReadGameGearPort(uint8_t port);

	/// <summary>Game Gear-specific port write.</summary>
	void WriteGameGearPort(uint8_t port, uint8_t value);

	/// <summary>SMS-specific port write.</summary>
	void WriteSmsPort(uint8_t port, uint8_t value);

	/// <summary>ColecoVision-specific port write.</summary>
	void WriteColecoVisionPort(uint8_t port, uint8_t value);

	/// <summary>Detects SG-1000 cartridge RAM requirements.</summary>
	uint32_t DetectSgCartRam(vector<uint8_t>& romData);

public:
	/// <summary>Initializes memory manager with all components.</summary>
	void Init(Emulator* emu, SmsConsole* console, vector<uint8_t>& romData, vector<uint8_t>& biosRom, SmsVdp* vdp, SmsControlManager* controlManager, SmsCart* cart, SmsPsg* psg, SmsFmAudio* fmAudio);
	SmsMemoryManager();
	~SmsMemoryManager();

	/// <summary>Gets current memory manager state.</summary>
	SmsMemoryManagerState& GetState() {
		return _state;
	}

	/// <summary>Executes specified clock cycles.</summary>
	__forceinline void Exec(uint8_t clocks) {
		_masterClock += clocks;
		_vdp->Run(_masterClock);
	}

	/// <summary>Refreshes memory bank mappings.</summary>
	void RefreshMappings();

	/// <summary>Checks if BIOS is present.</summary>
	bool HasBios();

	/// <summary>Saves battery-backed RAM.</summary>
	void SaveBattery();

	/// <summary>Gets absolute address from relative address.</summary>
	AddressInfo GetAbsoluteAddress(uint16_t addr);

	/// <summary>Gets relative address from absolute address.</summary>
	int32_t GetRelativeAddress(AddressInfo& absAddress);

	/// <summary>Maps memory range to type and offset.</summary>
	void Map(uint16_t start, uint16_t end, MemoryType type, uint32_t offset, bool readonly);

	/// <summary>Unmaps memory range.</summary>
	void Unmap(uint16_t start, uint16_t end);

	/// <summary>Maps register handlers to address range.</summary>
	void MapRegisters(uint16_t start, uint16_t end, SmsRegisterAccess access);

	/// <summary>Gets current open bus value.</summary>
	uint8_t GetOpenBus();

	/// <summary>Reads from memory.</summary>
	__forceinline uint8_t Read(uint16_t addr, MemoryOperationType opType) {
		uint8_t value;
		if (_state.IsReadRegister[addr >> 8]) {
			value = _cart->ReadRegister(addr);
		} else if (_reads[addr >> 8]) {
			value = _reads[addr >> 8][(uint8_t)addr];
		} else {
			value = GetOpenBus();
		}
		if (_emu->GetCheatManager()->HasCheats<CpuType::Sms>()) {
			_emu->GetCheatManager()->ApplyCheat<CpuType::Sms>(addr, value);
		}
		_state.OpenBus = value;
		_emu->ProcessMemoryRead<CpuType::Sms>(addr, value, opType);
		return value;
	}

	/// <summary>Debug read (no side effects).</summary>
	uint8_t DebugRead(uint16_t addr);

	/// <summary>Writes to memory.</summary>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>Debug write (no side effects).</summary>
	void DebugWrite(uint16_t addr, uint8_t value);

	/// <summary>Debug I/O port read.</summary>
	uint8_t DebugReadPort(uint8_t port);

	/// <summary>Reads from I/O port.</summary>
	uint8_t ReadPort(uint8_t port);

	/// <summary>Writes to I/O port.</summary>
	void WritePort(uint8_t port, uint8_t value);

	/// <summary>Serializes state for save states.</summary>
	void Serialize(Serializer& s) override;
};
