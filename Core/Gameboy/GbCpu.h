#if (defined(DUMMYCPU) && !defined(__DUMMYGBCPU__H)) || (!defined(DUMMYCPU) && !defined(__GBCPU__H))
#ifdef DUMMYCPU
#define __DUMMYGBCPU__H
#else
#define __GBCPU__H
#endif

#include "pch.h"
#include "Gameboy/GbTypes.h"
#include "Debugger/DebugTypes.h"
#include "Utilities/ISerializable.h"

class GbMemoryManager;
class Gameboy;
class GbPpu;
class Emulator;

/// <summary>
/// Game Boy CPU emulator - Sharp LR35902 implementation.
/// Modified Z80 core used in Game Boy, Game Boy Color, and Super Game Boy.
/// </summary>
/// <remarks>
/// The LR35902 is a hybrid CPU combining Z80 and 8080 features:
/// - 8-bit data bus, 16-bit address bus
/// - 7 8-bit registers: A, B, C, D, E, H, L
/// - Paired as 16-bit: AF, BC, DE, HL (HL used as memory pointer)
/// - Stack pointer (SP) and program counter (PC)
/// - 4 flag bits in F register: Z (zero), N (subtract), H (half-carry), C (carry)
///
/// **Differences from Z80:**
/// - No IX, IY index registers
/// - No alternate register set
/// - No I/O port instructions
/// - Different interrupt handling (IM modes → IE/IF registers)
/// - Simplified instruction set (no ED, DD, FD prefixes)
/// - CB prefix for bit operations and rotates
///
/// **Clock Speed:**
/// - DMG (original): 4.194304 MHz (1 M-cycle = 4 T-states)
/// - CGB (Color): 4.194304 MHz or 8.388608 MHz (double speed mode)
/// - SGB: Slightly different (~4.295 MHz)
///
/// **Interrupts (IF/IE at $FF0F/$FFFF):**
/// - V-Blank ($0040): Triggered at end of frame
/// - LCD STAT ($0048): Various LCD conditions
/// - Timer ($0050): Timer overflow
/// - Serial ($0058): Serial transfer complete
/// - Joypad ($0060): Button press (active low transition)
///
/// **Special Features:**
/// - HALT: Low-power mode until interrupt
/// - STOP: Very low-power mode (also triggers CGB speed switch)
/// - HALT bug: PC increment skipped if interrupts disabled
/// - OAM corruption: Certain operations during OAM scan cause glitches
/// </remarks>
class GbCpu : public ISerializable {
private:
	GbCpuState _state = {};  ///< CPU registers (A, F, B, C, D, E, H, L, SP, PC)

	// Register pair wrappers for 16-bit access
	Register16 _regAF = Register16(&_state.A, &_state.Flags);  ///< Accumulator + Flags
	Register16 _regBC = Register16(&_state.B, &_state.C);      ///< BC pair
	Register16 _regDE = Register16(&_state.D, &_state.E);      ///< DE pair
	Register16 _regHL = Register16(&_state.H, &_state.L);      ///< HL pair (memory pointer)

	GbMemoryManager* _memoryManager = nullptr;  ///< Memory mapping and bus access
	Emulator* _emu = nullptr;                   ///< Emulator for debugger hooks
	Gameboy* _gameboy = nullptr;                ///< Parent console
	GbPpu* _ppu = nullptr;                      ///< PPU reference for timing

	uint8_t _prevIrqVector = 0;  ///< Previous interrupt vector (for edge detection)

	void ExecOpCode(uint8_t opCode);  ///< Execute single instruction

	void ProcessCgbSpeedSwitch();     ///< Handle CGB double-speed mode toggle
	__noinline void ProcessHaltBug(); ///< Handle HALT bug (PC not incremented)

	__forceinline void ExecCpuCycle();    ///< Execute one M-cycle (4 T-states)
	__forceinline void ExecMasterCycle(); ///< Execute one T-state
	__forceinline uint8_t ReadOpCode();   ///< Fetch opcode at PC
	__forceinline uint8_t ReadCode();     ///< Read byte at PC, increment PC
	__forceinline uint16_t ReadCodeWord(); ///< Read word at PC (little-endian)

	/// Read memory with optional OAM corruption emulation
	template <GbOamCorruptionType oamCorruptionType = GbOamCorruptionType::Read>
	__forceinline uint8_t Read(uint16_t addr);

	__forceinline void Write(uint16_t addr, uint8_t value);  ///< Write memory

	// Flag operations
	bool CheckFlag(uint8_t flag);
	void SetFlag(uint8_t flag);
	void ClearFlag(uint8_t flag);
	void SetFlagState(uint8_t flag, bool state);

	// Stack operations
	void PushByte(uint8_t value);
	void PushWord(uint16_t value);
	uint16_t PopWord();

	// Load instructions
	void LD(uint8_t& dst, uint8_t value);
	void LD(uint16_t& dst, uint16_t value);
	void LD(Register16& dst, uint16_t value);
	void LD_Indirect(uint16_t dst, uint8_t value);
	void LD_Indirect16(uint16_t dst, uint16_t value);
	void LD_HL(int8_t value);

	// Increment/decrement instructions
	void INC(uint8_t& dst);
	void INC(Register16& dst);
	void INC_SP();
	void INC_Indirect(uint16_t addr);
	void DEC(uint8_t& dst);
	void DEC(Register16& dst);
	void DEC_Indirect(uint16_t addr);
	void DEC_SP();

	// Arithmetic instructions
	void ADD(uint8_t value);
	void ADD_SP(int8_t value);
	void ADD(Register16& reg, uint16_t value);
	void ADC(uint8_t value);
	void SUB(uint8_t value);
	void SBC(uint8_t value);

	void AND(uint8_t value);
	void OR(uint8_t value);
	void XOR(uint8_t value);

	void CP(uint8_t value);

	void NOP();
	void InvalidOp();
	void STOP();
	void HALT();

	void CPL();

	void RL(uint8_t& dst);
	void RL_Indirect(uint16_t addr);
	void RLC(uint8_t& dst);
	void RLC_Indirect(uint16_t addr);
	void RR(uint8_t& dst);
	void RR_Indirect(uint16_t addr);
	void RRC(uint8_t& dst);
	void RRC_Indirect(uint16_t addr);
	void RRA();
	void RRCA();
	void RLCA();
	void RLA();
	void SRL(uint8_t& dst);
	void SRL_Indirect(uint16_t addr);
	void SRA(uint8_t& dst);
	void SRA_Indirect(uint16_t addr);
	void SLA(uint8_t& dst);
	void SLA_Indirect(uint16_t addr);

	void SWAP(uint8_t& dst);
	void SWAP_Indirect(uint16_t addr);

	template <MemoryOperationType type, GbOamCorruptionType oamCorruptionType>
	uint8_t ReadMemory(uint16_t addr);

	template <uint8_t bit>
	void BIT(uint8_t src);

	template <uint8_t bit>
	void RES(uint8_t& dst);

	template <uint8_t bit>
	void RES_Indirect(uint16_t addr);

	template <uint8_t bit>
	void SET(uint8_t& dst);

	template <uint8_t bit>
	void SET_Indirect(uint16_t addr);

	void DAA();

	void JP(uint16_t dstAddr);
	void JP_HL();
	void JP(bool condition, uint16_t dstAddr);
	void JR(int8_t offset);
	void JR(bool condition, int8_t offset);

	void CALL(uint16_t dstAddr);
	void CALL(bool condition, uint16_t dstAddr);
	void RET();
	void RET(bool condition);
	void RETI();
	void RST(uint8_t value);

	void POP(Register16& reg);
	void PUSH(Register16& reg);
	void POP_AF();

	void SCF();
	void CCF();

	void EI();
	void DI();
	void PREFIX();

	__forceinline void ProcessNextCycleStart();
	__noinline bool HandleStoppedState();

public:
	virtual ~GbCpu();

	void Init(Emulator* emu, Gameboy* gameboy, GbMemoryManager* memoryManager);

	GbCpuState& GetState();
	bool IsHalted();

	[[nodiscard]] uint64_t GetCycleCount() { return _state.CycleCount; }

	void Exec();
	void PowerOn();

	void Serialize(Serializer& s) override;

#ifdef DUMMYCPU
private:
	uint32_t _memOpCounter = 0;
	MemoryOperationInfo _memOperations[10] = {};

public:
	void SetDummyState(GbCpuState& state);
	[[nodiscard]] uint32_t GetOperationCount();
	void LogMemoryOperation(uint32_t addr, uint8_t value, MemoryOperationType type);
	MemoryOperationInfo GetOperationInfo(uint32_t index);
#endif
};
#endif
