# Lynx Emulation — Code Plans

> **Purpose:** Class structures, method signatures, and key algorithms for Lynx emulation implementation  
> **Created:** 2026-02-15  
> **Parent Issues:** #270, #271, #272, #273, #274  

---

## 1. LynxConsole (IConsole) — Core/Lynx/LynxConsole.h

```cpp
#pragma once
#include "Shared/Interfaces/IConsole.h"
#include "Lynx/LynxTypes.h"

class LynxCpu;
class LynxMikey;
class LynxSuzy;
class LynxMemoryManager;
class LynxControlManager;
class LynxCart;
class LynxDefaultVideoFilter;

class LynxConsole final : public IConsole {
private:
	Emulator* _emu = nullptr;

	unique_ptr<LynxCpu> _cpu;
	unique_ptr<LynxMikey> _mikey;
	unique_ptr<LynxSuzy> _suzy;
	unique_ptr<LynxMemoryManager> _memoryManager;
	unique_ptr<LynxControlManager> _controlManager;
	unique_ptr<LynxCart> _cart;

	LynxModel _model = LynxModel::LynxII;  // Lynx I (65SC02) or Lynx II (65C02)
	bool _useHleBoot = true;

public:
	LynxConsole(Emulator* emu);
	~LynxConsole() override;

	// IConsole — Core lifecycle
	void Reset() override;
	LoadRomResult LoadRom(VirtualFile romFile) override;
	void RunFrame() override;
	void ProcessEndOfFrame() override;

	// IConsole — Console identification
	ConsoleType GetConsoleType() override { return ConsoleType::Lynx; }
	vector<CpuType> GetCpuTypes() override { return { CpuType::Lynx }; }
	DipSwitchInfo GetDipSwitchInfo() override { return {}; }

	// IConsole — Subsystem access
	BaseControlManager* GetControlManager() override;
	uint32_t GetMasterClockRate() override { return 16000000; } // 16 MHz crystal
	double GetFps() override { return 75.0; } // ~75 Hz refresh

	// IConsole — Timing
	TimingInfo GetTimingInfo() override;
	BaseVideoFilter* GetVideoFilter(bool getEmpty) override;
	AudioTrackInfo GetAudioTrackInfo() override;

	// IConsole — State
	void Serialize(Serializer& s) override;
	SaveStateCompatInfo ValidateSaveStateCompatibility(Serializer& s) override;

	// IConsole — Debug/address translation
	void GetConsoleSpecificStatus(StatusSummary& summary) override;
	AddressInfo GetAbsoluteAddress(AddressInfo& relAddr) override;
	AddressInfo GetRelativeAddress(AddressInfo& absAddr, CpuType cpuType) override;

	// Lynx-specific accessors
	LynxCpu* GetCpu() { return _cpu.get(); }
	LynxMikey* GetMikey() { return _mikey.get(); }
	LynxSuzy* GetSuzy() { return _suzy.get(); }
	LynxMemoryManager* GetMemoryManager() { return _memoryManager.get(); }
	LynxModel GetModel() const { return _model; }

private:
	void HleBoot();  // Set up post-boot state without running boot ROM
	bool DetectFormat(VirtualFile& file, LynxCartInfo& info);
};
```

---

## 2. LynxCpu — Core/Lynx/LynxCpu.h

```cpp
#pragma once
#include "Shared/CpuTypes.h"
#include "Lynx/LynxTypes.h"

class LynxConsole;
class LynxMemoryManager;
class Emulator;

class LynxCpu {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxMemoryManager* _memoryManager = nullptr;

	// Registers
	uint8_t _a = 0;    // Accumulator
	uint8_t _x = 0;    // X index
	uint8_t _y = 0;    // Y index
	uint8_t _sp = 0;   // Stack pointer
	uint16_t _pc = 0;  // Program counter
	uint8_t _ps = 0;   // Processor status (NV-BDIZC)

	// Internal state
	uint64_t _cycleCount = 0;
	bool _irqPending = false;
	bool _nmiPending = false;  // Not used in normal Lynx operation
	bool _halted = false;      // CPU halted by Suzy bus contention
	bool _waiting = false;     // WAI instruction — waiting for interrupt
	bool _stopped = false;     // STP instruction — fully stopped

	// 65C02 variant
	bool _is65C02 = true;      // false = 65SC02 (Lynx I, no Rockwell extensions)

	// Instruction handling
	using OpHandler = void (LynxCpu::*)();
	OpHandler _opTable[256];
	uint8_t _cycleCounts[256];

public:
	LynxCpu(Emulator* emu, LynxConsole* console, LynxMemoryManager* memMgr);

	void Reset();
	void Exec();           // Execute one instruction
	void SetIrq(bool set);
	void HaltCpu(bool halt);  // Called by Suzy during sprite processing

	// State
	LynxCpuState GetState() const;
	void SetState(const LynxCpuState& state);
	void Serialize(Serializer& s);

	// Debugger support
	uint16_t GetPC() const { return _pc; }
	void SetPC(uint16_t pc) { _pc = pc; }
	uint64_t GetCycleCount() const { return _cycleCount; }

private:
	// Memory access (with debugger hooks)
	uint8_t Read(uint16_t addr);
	void Write(uint16_t addr, uint8_t value);
	uint8_t ReadImmediate();
	uint16_t ReadWord(uint16_t addr);

	// Addressing modes
	uint16_t AddrImmediate();
	uint16_t AddrZeroPage();
	uint16_t AddrZeroPageX();
	uint16_t AddrZeroPageY();
	uint16_t AddrAbsolute();
	uint16_t AddrAbsoluteX(bool checkPage = true);
	uint16_t AddrAbsoluteY(bool checkPage = true);
	uint16_t AddrIndirect();
	uint16_t AddrIndirectX();
	uint16_t AddrIndirectY(bool checkPage = true);
	uint16_t AddrZeroPageIndirect();   // 65C02 new mode: (zp)
	uint16_t AddrRelative();

	// Flag helpers
	void SetZN(uint8_t value);
	bool GetFlag(uint8_t flag) const;
	void SetFlag(uint8_t flag, bool set);

	// Instructions (selected key ones)
	void ADC(); void SBC();     // With BCD support
	void AND(); void ORA(); void EOR();
	void ASL(); void LSR(); void ROL(); void ROR();
	void BCC(); void BCS(); void BEQ(); void BNE();
	void BPL(); void BMI(); void BVC(); void BVS();
	void BRA();                 // 65C02: Branch always
	void BIT();
	void CMP(); void CPX(); void CPY();
	void DEC(); void DEX(); void DEY();
	void INC(); void INX(); void INY();
	void JMP(); void JSR(); void RTS(); void RTI();
	void LDA(); void LDX(); void LDY();
	void STA(); void STX(); void STY();
	void STZ();                 // 65C02: Store zero
	void TAX(); void TXA(); void TAY(); void TYA();
	void TXS(); void TSX();
	void PHA(); void PLA();
	void PHP(); void PLP();
	void PHX(); void PLX();    // 65C02: Push/pull X
	void PHY(); void PLY();    // 65C02: Push/pull Y
	void TRB(); void TSB();    // 65C02: Test and reset/set bits
	void SEC(); void CLC(); void SED(); void CLD();
	void SEI(); void CLI(); void CLV();
	void BRK(); void NOP();
	void WAI();                // 65C02: Wait for interrupt
	void STP();                // 65C02: Stop (halt until reset)

	// Rockwell extensions (65C02 only, not 65SC02)
	void RMB(); void SMB();    // Reset/set memory bit
	void BBR(); void BBS();    // Branch on bit reset/set

	void InitOpTable();
	void HandleIRQ();
};
```

---

## 3. LynxMikey — Core/Lynx/LynxMikey.h

```cpp
#pragma once
#include "Lynx/LynxTypes.h"

class Emulator;
class LynxConsole;
class LynxCpu;
class LynxApu;

struct LynxTimer {
	uint8_t backup = 0;       // Reload value
	uint8_t control = 0;      // Control register
	uint8_t count = 0;        // Current count
	uint8_t dynControl = 0;   // Dynamic control
	bool borrowOut = false;   // Timer underflowed
	bool irqPending = false;  // IRQ flag for this timer
	uint16_t prescaler = 0;   // Internal divider counter

	// Control bits
	bool IsEnabled() const;
	bool IsLinked() const;    // Count from linked timer instead of clock
	uint8_t GetClockDivider() const;
	bool IsIrqEnabled() const;
};

class LynxMikey {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxCpu* _cpu = nullptr;

	// 8 Timers
	LynxTimer _timers[8];

	// Interrupt controller
	uint8_t _intrst = 0;   // Interrupt status (read = pending, write = clear)
	uint8_t _intset = 0;   // Interrupt set (write to force interrupt)

	// Display
	uint8_t _dispctl = 0;        // Display control
	uint8_t _pbkup = 0;          // Period backup
	uint16_t _dispadr = 0;       // Display address (frame buffer start)
	bool _displayEnabled = true;

	// Palette (16 entries, 12-bit color each)
	uint16_t _palette[16] = {};   // Packed: 4-bit R, 4-bit G, 4-bit B

	// Serial
	uint8_t _serctl = 0;
	uint8_t _serdat = 0;

	// System
	uint8_t _iodir = 0;
	uint8_t _iodat = 0;

	// Audio (delegated to APU)
	unique_ptr<LynxApu> _apu;

	// Video output buffer
	uint16_t _frameBuffer[160 * 102] = {};  // Indexed color output

public:
	LynxMikey(Emulator* emu, LynxConsole* console);
	~LynxMikey();

	void Reset();
	void Tick(uint32_t cpuCycles);   // Advance timers by CPU cycles

	// Register access
	uint8_t Read(uint16_t addr);
	void Write(uint16_t addr, uint8_t value);

	// Timer management
	void TickTimer(int timerIndex);
	void UpdateTimerLinking();
	bool IsIrqPending() const;

	// Display
	void RenderFrame();          // DMA frame buffer to output
	const uint16_t* GetFrameBuffer() const { return _frameBuffer; }
	uint16_t GetDisplayAddress() const { return _dispadr; }

	// Palette
	uint32_t GetRgbColor(uint8_t index) const;  // Convert 12-bit to 32-bit ARGB

	// State
	LynxMikeyState GetState() const;
	void Serialize(Serializer& s);

	// Accessors
	LynxApu* GetApu() { return _apu.get(); }
};
```

---

## 4. LynxSuzy — Core/Lynx/LynxSuzy.h

```cpp
#pragma once
#include "Lynx/LynxTypes.h"

class Emulator;
class LynxConsole;
class LynxCpu;
class LynxMemoryManager;

// Sprite Control Block structure
struct LynxSCB {
	uint8_t sprctl0 = 0;     // Sprite type, bpp, H/V flip
	uint8_t sprctl1 = 0;     // Reload, sizing, literal
	uint8_t sprcoll = 0;     // Collision number
	uint16_t scbnext = 0;    // Next SCB address (0 = end)
	uint16_t sprdline = 0;   // Sprite data start
	int16_t hpos = 0;        // Horizontal position
	int16_t vpos = 0;        // Vertical position
	uint16_t hsiz = 0;       // Horizontal size
	uint16_t vsiz = 0;       // Vertical size
	uint16_t stretch = 0;    // Stretch factor
	uint16_t tilt = 0;       // Tilt factor
};

class LynxSuzy {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxMemoryManager* _memManager = nullptr;

	// Sprite engine state
	LynxSCB _currentSCB;
	uint16_t _scbAddr = 0;       // Current SCB address
	bool _spriteActive = false;  // Sprite engine running
	uint16_t _vidbas = 0;        // Video buffer base
	uint16_t _collbas = 0;       // Collision buffer base

	// Math coprocessor
	uint8_t _mathA = 0, _mathB = 0, _mathC = 0, _mathD = 0;
	uint8_t _mathE = 0, _mathF = 0, _mathG = 0, _mathH = 0;
	uint8_t _mathJ = 0, _mathK = 0, _mathL = 0, _mathM = 0, _mathN = 0, _mathP = 0;
	bool _mathSign = false;      // Sign of last operation
	bool _mathAccumulate = false;

	// Input
	uint8_t _joystick = 0;       // D-pad + option buttons
	uint8_t _switches = 0;       // A/B + Pause

	// Cart address generator
	uint32_t _cartShift = 0;
	uint8_t _cartCounter0 = 0;
	uint8_t _cartCounter1 = 0;

	// Control registers
	uint8_t _sprctl0 = 0;
	uint8_t _sprctl1 = 0;
	uint8_t _sprsys = 0;
	uint8_t _sprinit = 0;
	bool _suzyBusEn = false;
	bool _spriteGo = false;

	// Collision depository
	uint8_t _collision[16] = {};

public:
	LynxSuzy(Emulator* emu, LynxConsole* console, LynxMemoryManager* memMgr);

	void Reset();

	// Register access
	uint8_t Read(uint16_t addr);
	void Write(uint16_t addr, uint8_t value);

	// Sprite engine
	void ProcessSprites();           // Main sprite processing loop
	uint32_t RenderSprite(LynxSCB& scb);  // Render single sprite, returns cycles
	void ParseSCB(uint16_t addr, LynxSCB& scb);  // Read SCB from memory

	// Math
	void DoMultiply();
	void DoDivide();

	// Input
	void SetButtonState(uint8_t joystick, uint8_t switches);
	uint8_t GetJoystick() const { return _joystick; }

	// Cart
	uint8_t ReadCart(int bank);
	void WriteCart(int bank, uint8_t value);

	// State
	LynxSuzyState GetState() const;
	void Serialize(Serializer& s);
};
```

---

## 5. LynxMemoryManager — Core/Lynx/LynxMemoryManager.h

```cpp
#pragma once
#include "Lynx/LynxTypes.h"

class Emulator;
class LynxConsole;
class LynxMikey;
class LynxSuzy;

class LynxMemoryManager {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxMikey* _mikey = nullptr;
	LynxSuzy* _suzy = nullptr;

	// Memory
	uint8_t* _ram = nullptr;      // 64KB RAM
	uint8_t* _bootRom = nullptr;  // 512 bytes ($FE00-$FFFF in Mikey)
	uint32_t _bootRomSize = 0;

	// MAPCTL state
	uint8_t _mapctl = 0;          // Memory overlay control at $FFF9
	bool _vectorsFromRom = true;  // Bit 0
	bool _mikeyEnabled = true;    // Bit 1
	bool _suzyEnabled = true;     // Bit 2
	bool _romEnabled = true;      // Bit 3
	bool _seqCartDisable0 = false; // Bit 6
	bool _seqCartDisable1 = false; // Bit 7

public:
	LynxMemoryManager(Emulator* emu, LynxConsole* console);
	~LynxMemoryManager();

	void Reset();
	void SetSubsystems(LynxMikey* mikey, LynxSuzy* suzy);

	// Memory access
	uint8_t Read(uint16_t addr, MemoryOperationType type = MemoryOperationType::Read);
	void Write(uint16_t addr, uint8_t value, MemoryOperationType type = MemoryOperationType::Write);

	// Direct access (for DMA, no debugger hooks)
	uint8_t PeekRam(uint16_t addr) const { return _ram[addr]; }
	void PokeRam(uint16_t addr, uint8_t value) { _ram[addr] = value; }

	// Boot ROM
	void LoadBootRom(uint8_t* data, uint32_t size);
	bool HasBootRom() const { return _bootRomSize > 0; }

	// MAPCTL
	void UpdateMapctl(uint8_t value);
	uint8_t GetMapctl() const { return _mapctl; }

	// State
	void Serialize(Serializer& s);

	// Debug
	uint8_t* GetRam() { return _ram; }
	AddressInfo GetAbsoluteAddress(uint16_t addr);
	uint16_t GetRelativeAddress(AddressInfo& absAddr);

private:
	bool IsMikeyRange(uint16_t addr) const;  // $FD00-$FDFF
	bool IsSuzyRange(uint16_t addr) const;   // $FC00-$FCFF
	bool IsRomRange(uint16_t addr) const;    // $FE00-$FFFF
	bool IsVectorRange(uint16_t addr) const; // $FFFA-$FFFF
};
```

---

## 6. LynxTypes.h — Core/Lynx/LynxTypes.h

```cpp
#pragma once
#include <cstdint>

// Hardware model
enum class LynxModel : uint8_t {
	LynxI,    // 65SC02, mono audio
	LynxII    // 65C02 (full), stereo audio
};

// Screen rotation
enum class LynxRotation : uint8_t {
	None = 0,
	Left = 1,
	Right = 2
};

// CPU state (for debugger)
struct LynxCpuState {
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t SP;
	uint16_t PC;
	uint8_t PS;        // NV-BDIZC
	uint64_t CycleCount;
	bool IRQPending;
	bool Halted;
	bool Waiting;      // WAI state
	bool Stopped;      // STP state
};

// Timer state (for debugger)
struct LynxTimerState {
	uint8_t Backup;
	uint8_t Control;
	uint8_t Count;
	uint8_t DynControl;
	bool BorrowOut;
	bool IrqPending;
};

// Mikey state (for debugger)
struct LynxMikeyState {
	LynxTimerState Timers[8];
	uint8_t IntRst;
	uint8_t IntSet;
	uint8_t DispCtl;
	uint8_t PbkUp;
	uint16_t DispAdr;
	uint16_t Palette[16];
	uint8_t SerCtl;
	uint8_t SerDat;
	uint8_t IoDir;
	uint8_t IoDat;
};

// Suzy state (for debugger)
struct LynxSuzyState {
	uint8_t SprCtl0;
	uint8_t SprCtl1;
	uint8_t SprSys;
	uint8_t SprInit;
	uint16_t ScbNext;
	uint16_t VidBas;
	uint16_t CollBas;
	int16_t HPos;
	int16_t VPos;
	uint16_t HSiz;
	uint16_t VSiz;
	uint8_t MathA, MathB, MathC, MathD;
	uint8_t Joystick;
	uint8_t Switches;
	bool SpriteActive;
	uint8_t Collision[16];
};

// Memory manager state
struct LynxMemoryManagerState {
	uint8_t MapCtl;
	bool VectorsFromRom;
	bool MikeyEnabled;
	bool SuzyEnabled;
	bool RomEnabled;
};

// Aggregate state for debugger
struct LynxState {
	LynxCpuState Cpu;
	LynxMikeyState Mikey;
	LynxSuzyState Suzy;
	LynxMemoryManagerState MemoryManager;
	LynxModel Model;
};

// Cart info from LNX header
struct LynxCartInfo {
	uint16_t PageSizeBank0;
	uint16_t PageSizeBank1;
	uint16_t Version;
	char CartName[33];           // 32 chars + null
	uint16_t ManufacturerId;
	LynxRotation Rotation;
	uint32_t RomSize;
	bool HasHeader;              // true for .lnx, false for .lyx/.o
};

// P flag masks
constexpr uint8_t LynxFlagC = 0x01;  // Carry
constexpr uint8_t LynxFlagZ = 0x02;  // Zero
constexpr uint8_t LynxFlagI = 0x04;  // IRQ disable
constexpr uint8_t LynxFlagD = 0x08;  // BCD mode
constexpr uint8_t LynxFlagB = 0x10;  // Break
constexpr uint8_t LynxFlagU = 0x20;  // Unused (always 1)
constexpr uint8_t LynxFlagV = 0x40;  // Overflow
constexpr uint8_t LynxFlagN = 0x80;  // Negative

// Timing constants
constexpr uint32_t LynxMasterClock = 16000000;  // 16 MHz
constexpr uint32_t LynxCpuDivider = 4;          // CPU @ 4 MHz
constexpr uint32_t LynxScreenWidth = 160;
constexpr uint32_t LynxScreenHeight = 102;
constexpr uint32_t LynxFps = 75;                // ~75 Hz refresh
constexpr uint32_t LynxRamSize = 65536;         // 64KB
constexpr uint32_t LynxBootRomSize = 512;       // 512 bytes
```

---

## 7. Key Algorithms

### 7.1 Timer Tick (Mikey)

```text
for each active timer T:
    if T is linked to previous timer:
        if previous timer borrowOut:
            T.count--
    else:
        T.prescaler += cpuCycles
        while T.prescaler >= T.divider:
            T.prescaler -= T.divider
            T.count--

    if T.count underflows (was 0, decremented):
        T.borrowOut = true
        T.count = T.backup
        if T.irqEnabled:
            set interrupt bit in INTRST
            signal IRQ to CPU
    else:
        T.borrowOut = false
```

### 7.2 Sprite Processing (Suzy)

```text
addr = SCBNEXT register
while addr != 0:
    scb = ParseSCB(addr)
    if scb.sprctl0 & SPRITE_SKIP:
        addr = scb.scbnext
        continue

    cycles = RenderSprite(scb)
    cpu.halt(cycles)  // CPU stops during sprite rendering

    if scb.sprcoll != 0:
        UpdateCollisionBuffer(scb)

    addr = scb.scbnext

clear SPRGO flag
resume CPU
```

### 7.3 Memory Read (MAPCTL)

```text
if addr >= 0xFC00 && addr <= 0xFCFF && suzyEnabled:
    return suzy.Read(addr)
if addr >= 0xFD00 && addr <= 0xFDFF && mikeyEnabled:
    return mikey.Read(addr)
if addr >= 0xFE00 && addr <= 0xFEFF && romEnabled:
    return bootRom[addr - 0xFE00]
if addr == 0xFFF9:
    return mapctl
if addr >= 0xFFFA && vectorsFromRom:
    return bootRom[addr - 0xFE00]
return ram[addr]
```

### 7.4 Hardware Multiply (Suzy)

```text
// Triggered on write to MATHD (or MATHA depending on mode)
uint32_t result = (uint16_t)(mathB << 8 | mathA) * (uint16_t)(mathD << 8 | mathC)
mathA = result & 0xFF
mathB = (result >> 8) & 0xFF
mathC = (result >> 16) & 0xFF
mathD = (result >> 24) & 0xFF

// If accumulate mode:
// Add to MATHEFGH
```

---

## 8. LynxApu — Core/Lynx/APU/LynxApu.h

```cpp
#pragma once
#include "Lynx/LynxTypes.h"

class LynxApuChannel;

class LynxApu {
private:
	unique_ptr<LynxApuChannel> _channels[4];
	uint8_t _attenuation[4] = {};  // Lynx II stereo attenuation
	uint8_t _stereo = 0;           // Stereo channel assignment
	bool _isStereo = false;        // Lynx II mode

public:
	LynxApu(bool stereo);
	~LynxApu();

	void Reset();
	void TickChannel(int channel);  // Called from timer system

	// Register access
	uint8_t ReadRegister(uint16_t addr);
	void WriteRegister(uint16_t addr, uint8_t value);

	// Audio output
	void MixOutput(int16_t* outLeft, int16_t* outRight);

	void Serialize(Serializer& s);
};

class LynxApuChannel {
private:
	uint8_t _volume = 0;
	uint8_t _feedback = 0;        // LFSR feedback taps
	uint8_t _output = 0;          // Current output value
	uint8_t _backup = 0;          // Period backup
	uint8_t _control = 0;         // Clock source, taps, etc.
	uint8_t _count = 0;           // Current counter
	uint8_t _other = 0;           // Other register

	uint32_t _shiftRegister = 0;  // 12-bit LFSR
	bool _integrationMode = false; // PCM integration mode

public:
	void Reset();
	void Tick();                   // Process one sample period
	int16_t GetOutput() const;
	void Serialize(Serializer& s);

	uint8_t ReadRegister(int reg);
	void WriteRegister(int reg, uint8_t value);
};
```

---

## 9. Implementation Priority Order

### Session 2 (First Implementation Session)

1. Create `LynxTypes.h` with all structs and constants
2. Add enum values to all files (#303)
3. Create `LynxConsole` skeleton (#304)
4. Create `LynxMemoryManager` with MAPCTL (#305)
5. Create `LynxCpu` with instruction decode (#279, #280)
6. Wire into `Emulator.cpp` for ROM detection
7. Create build integration (#310)

### Session 3

1. Complete CPU (cycle timing #281, BCD #282, IRQ #283)
2. Cart loading (#306)
3. Boot ROM (#307)
4. Basic timer skeleton (#287)

### Session 4

1. Full timer system (#287, #288)
2. Video DMA (#289)
3. Palette (#295)
4. Get first screens displaying

### Sessions 5-6

1. Suzy sprite engine (#296, #297, #298)
2. Math coprocessor (#299)
3. Input (#300, #312)
4. Cart addressing (#301)

And so on per the master plan session schedule.
