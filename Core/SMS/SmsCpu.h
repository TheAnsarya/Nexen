#if (defined(DUMMYCPU) && !defined(__DUMMYSmsCpu__H)) || (!defined(DUMMYCPU) && !defined(__SmsCpu__H))
#ifdef DUMMYCPU
#define __DUMMYSmsCpu__H
#else
#define __SmsCpu__H
#endif

#include "pch.h"
#include "SMS/SmsTypes.h"
#include "Shared/MemoryOperationType.h"
#include "Shared/MemoryType.h"
#include "Utilities/ISerializable.h"

class Emulator;
class SmsConsole;
class SmsMemoryManager;
class SmsCpuParityTable;

/// <summary>
/// Z80 CPU emulator for Sega Master System/Game Gear.
/// Zilog Z80 running at 3.58 MHz (NTSC) or 3.55 MHz (PAL).
/// </summary>
/// <remarks>
/// The Z80 is an 8-bit CPU with:
/// - 8 8-bit registers: A, B, C, D, E, H, L, Flags
/// - Alternate register set (A', F', B', C', D', E', H', L')
/// - 16-bit index registers: IX, IY
/// - 16-bit stack pointer and program counter
/// - 8-bit interrupt vector (I) and refresh (R) registers
///
/// **Addressing Modes:**
/// - Immediate: LD A, $12
/// - Register: LD A, B
/// - Register indirect: LD A, (HL)
/// - Indexed: LD A, (IX+d)
/// - Extended: LD A, ($1234)
///
/// **Interrupts:**
/// - NMI: Non-maskable, jumps to $0066
/// - INT: Maskable, three modes:
///   - Mode 0: Execute instruction on bus
///   - Mode 1: RST $38 (used by SMS)
///   - Mode 2: Vector table (I register × 256 + bus value)
///
/// **Instruction Prefixes:**
/// - $CB: Bit operations (rotate, shift, bit test/set/reset)
/// - $DD: IX-indexed instructions
/// - $FD: IY-indexed instructions
/// - $ED: Extended instructions (block ops, I/O, 16-bit math)
///
/// **Undocumented Features:**
/// - SLL instruction (shift left, bit 0 = 1)
/// - IXH, IXL, IYH, IYL as 8-bit registers
/// - Undocumented flags (bits 3 and 5)
/// </remarks>
class SmsCpu final : public ISerializable {
private:
	static SmsCpuParityTable _parity;  ///< Lookup table for P/V flag calculation

	/// <summary>
	/// Helper class for 16-bit register access from two 8-bit halves.
	/// Provides unified interface for register pairs (BC, DE, HL, IX, IY).
	/// </summary>
	class Register16 {
		uint8_t* _high;  ///< Pointer to high byte
		uint8_t* _low;   ///< Pointer to low byte

	public:
		Register16(uint8_t* high, uint8_t* low) : _high(high), _low(low) {}

		void Write(uint16_t value) {
			*_high = (uint8_t)(value >> 8);
			*_low = (uint8_t)value;
		}

		uint16_t Read() { return (*_high << 8) | *_low; }
		void Inc() { Write(Read() + 1); }
		void Dec() { Write(Read() - 1); }
		operator uint16_t() { return Read(); }
	};

	Emulator* _emu = nullptr;
	SmsConsole* _console = nullptr;
	SmsMemoryManager* _memoryManager = nullptr;

	int32_t _cbAddress = -1;    ///< Address for indexed CB prefix instructions
	SmsCpuState _state = {};    ///< Current CPU state (registers, flags, etc.)

	// 16-bit register pair accessors
	Register16 _regAF = Register16(&_state.A, &_state.Flags);   ///< Accumulator + Flags
	Register16 _regBC = Register16(&_state.B, &_state.C);       ///< BC register pair
	Register16 _regDE = Register16(&_state.D, &_state.E);       ///< DE register pair
	Register16 _regHL = Register16(&_state.H, &_state.L);       ///< HL register pair
	Register16 _regIX = Register16(&_state.IXH, &_state.IXL);   ///< Index register X
	Register16 _regIY = Register16(&_state.IYH, &_state.IYL);   ///< Index register Y

	/// <summary>Executes an opcode with the specified prefix (0=none, DD=IX, FD=IY).</summary>
	template <uint8_t prefix>
	void ExecOpCode(uint8_t opCode);

	__forceinline void ExecCycles(uint8_t cycles);    ///< Advances cycle counter
	__forceinline uint8_t ReadOpCode();               ///< Fetches opcode at PC
	__forceinline uint8_t ReadNextOpCode();           ///< Fetches next opcode (for lookahead)
	__forceinline uint8_t ReadCode();                 ///< Reads immediate byte, increments PC
	__forceinline uint16_t ReadCodeWord();            ///< Reads immediate word (little-endian)

	__forceinline uint8_t Read(uint16_t addr);        ///< Memory read
	__forceinline void Write(uint16_t addr, uint8_t value);  ///< Memory write

	__forceinline uint8_t ReadPort(uint8_t port);            ///< I/O port read
	__forceinline void WritePort(uint8_t port, uint8_t value); ///< I/O port write

	/// <summary>Sets standard flags (S, Z, H, P/V, N, C) based on result value.</summary>
	template <uint8_t mask>
	void SetStandardFlags(uint8_t value);

	// Flag manipulation
	bool CheckFlag(uint8_t flag);                ///< Tests if flag is set
	void SetFlag(uint8_t flag);                  ///< Sets flag bit
	void ClearFlag(uint8_t flag);                ///< Clears flag bit
	void SetFlagState(uint8_t flag, bool state); ///< Sets or clears flag

	// Stack operations
	void PushByte(uint8_t value);     ///< Pushes byte to stack
	void PushWord(uint16_t value);    ///< Pushes word to stack (high byte first)
	uint16_t PopWord();               ///< Pops word from stack

	// ========== Load Instructions ==========
	void LD(uint8_t& dst, uint8_t value);           ///< 8-bit register load
	void LD_IR(uint8_t& dst, uint8_t value);        ///< Load I or R register (special flags)
	void LD(uint16_t& dst, uint16_t value);         ///< 16-bit register load
	void LD(Register16& dst, uint16_t value);       ///< 16-bit register pair load
	void LD_Indirect(uint16_t dst, uint8_t value);  ///< Store to memory
	void LDA_Address(uint16_t addr);                ///< Load A from memory address
	void LD_Indirect_A(uint16_t dst);               ///< Store A to memory
	void LD_IndirectImm(uint16_t dst);              ///< Store immediate to memory
	void LD_Indirect16(uint16_t dst, uint16_t value); ///< Store 16-bit to memory
	void LDReg_Indirect16(uint16_t& dst, uint16_t addr);    ///< Load 16-bit from memory
	void LDReg_Indirect16(Register16& dst, uint16_t addr);  ///< Load register pair from memory

	// ========== Arithmetic Instructions ==========
	void INC(uint8_t& dst);               ///< 8-bit increment
	void INC(Register16& dst);            ///< 16-bit increment
	void INC_SP();                        ///< Increment stack pointer
	void INC_Indirect(uint16_t addr);     ///< Increment memory
	void DEC(uint8_t& dst);               ///< 8-bit decrement
	void DEC(Register16& dst);            ///< 16-bit decrement
	void DEC_Indirect(uint16_t addr);     ///< Decrement memory
	void DEC_SP();                        ///< Decrement stack pointer

	void ADD(uint8_t value);              ///< A = A + value
	void ADD(Register16& reg, uint16_t value);  ///< 16-bit add
	void ADC(uint8_t value);              ///< A = A + value + carry
	void ADC16(uint16_t value);           ///< 16-bit add with carry
	void SUB(uint8_t value);              ///< A = A - value

	void SBC(uint8_t value);              ///< A = A - value - carry
	void SBC16(uint16_t value);           ///< 16-bit subtract with carry

	// ========== Logical Instructions ==========
	void AND(uint8_t value);              ///< A = A & value
	void OR(uint8_t value);               ///< A = A | value
	void XOR(uint8_t value);              ///< A = A ^ value

	void UpdateLogicalOpFlags(uint8_t value);  ///< Sets flags after logical op

	void CP(uint8_t value);               ///< Compare A with value (sets flags only)

	// ========== Control Instructions ==========
	void NOP();    ///< No operation
	void HALT();   ///< Halt until interrupt

	void CPL();    ///< Complement A (one's complement)

	// ========== Rotate/Shift Instructions ==========
	void RL(uint8_t& dst);                ///< Rotate left through carry
	void RL_Indirect(uint16_t addr);
	void RLC(uint8_t& dst);               ///< Rotate left circular
	void RLC_Indirect(uint16_t addr);
	void RR(uint8_t& dst);                ///< Rotate right through carry
	void RR_Indirect(uint16_t addr);
	void RRC(uint8_t& dst);               ///< Rotate right circular
	void RRC_Indirect(uint16_t addr);
	void RRA();                           ///< Rotate A right through carry
	void RRCA();                          ///< Rotate A right circular
	void RLCA();                          ///< Rotate A left circular
	void RLA();                           ///< Rotate A left through carry
	void SRL(uint8_t& dst);               ///< Shift right logical (bit 7 = 0)
	void SRL_Indirect(uint16_t addr);
	void SRA(uint8_t& dst);               ///< Shift right arithmetic (bit 7 preserved)
	void SRA_Indirect(uint16_t addr);
	void SLA(uint8_t& dst);               ///< Shift left arithmetic
	void SLA_Indirect(uint16_t addr);
	void SLL(uint8_t& dst);               ///< Undocumented: Shift left, bit 0 = 1
	void SLL_Indirect(uint16_t dst);

	/// <summary>Reads memory with operation type tracking for debugging.</summary>
	template <MemoryOperationType type>
	uint8_t ReadMemory(uint16_t addr);

	// ========== Bit Instructions (CB prefix) ==========
	/// <summary>Tests specified bit, sets Z flag if bit is 0.</summary>
	template <uint8_t bit>
	void BIT(uint8_t src);

	template <uint8_t bit>
	void BIT_Indirect(Register16& src);

	/// <summary>Resets (clears) specified bit.</summary>
	template <uint8_t bit>
	void RES(uint8_t& dst);

	template <uint8_t bit>
	void RES_Indirect(uint16_t addr);

	/// <summary>Sets specified bit.</summary>
	template <uint8_t bit>
	void SET(uint8_t& dst);

	template <uint8_t bit>
	void SET_Indirect(uint16_t addr);

	void DAA();   ///< Decimal adjust A for BCD arithmetic

	// ========== Jump/Call Instructions ==========
	void JP(uint16_t dstAddr);                    ///< Unconditional jump
	void JP(bool condition, uint16_t dstAddr);   ///< Conditional jump
	void JR(int8_t offset);                       ///< Relative jump
	void JR(bool condition, int8_t offset);       ///< Conditional relative jump

	void CALL(uint16_t dstAddr);                 ///< Call subroutine
	void CALL(bool condition, uint16_t dstAddr); ///< Conditional call
	void RET();                                   ///< Return from subroutine
	void RET(bool condition);                     ///< Conditional return
	void RETI();                                  ///< Return from interrupt
	void RST(uint8_t value);                      ///< Restart (call to page 0)

	void POP(Register16& reg);   ///< Pop 16-bit value to register pair
	void PUSH(Register16& reg);  ///< Push 16-bit register pair
	void POP_AF();               ///< Pop AF (special handling for flags)

	void SCF();   ///< Set carry flag
	void CCF();   ///< Complement carry flag

	// ========== Interrupt Control ==========
	void IM(uint8_t mode);   ///< Set interrupt mode (0, 1, or 2)
	void EI();               ///< Enable interrupts
	void DI();               ///< Disable interrupts

	/// <summary>Handles CB-prefixed instructions (bit operations).</summary>
	template <uint8_t prefix>
	void PREFIX_CB();

	uint8_t GetCbValue(uint8_t dst);            ///< Gets value for CB instruction
	void SetCbValue(uint8_t& dst, uint8_t val); ///< Sets value after CB instruction

	void PREFIX_ED();   ///< Handles ED-prefixed instructions (extended)

	// ========== Exchange Instructions ==========
	void EXX();           ///< Exchange BC,DE,HL with BC',DE',HL'
	void ExchangeAf();    ///< Exchange AF with AF'
	void ExchangeSp(Register16& reg);  ///< Exchange (SP) with register
	void ExchangeDeHl();  ///< Exchange DE and HL

	void DJNZ();          ///< Decrement B, jump if not zero

	// ========== I/O Instructions ==========
	void OUT(uint8_t src, uint8_t port);   ///< Output to port
	void OUT_Imm(uint8_t port);            ///< Output A to immediate port
	void IN(uint8_t& dst, uint8_t port);   ///< Input from port to register
	uint8_t IN(uint8_t port);              ///< Input from port (flags only)
	void IN_Imm(uint8_t port);             ///< Input from immediate port to A

	void NEG();   ///< Negate A (two's complement)

	void RRD();   ///< Rotate right decimal (BCD nibble rotation)
	void RLD();   ///< Rotate left decimal (BCD nibble rotation)

	// ========== Block Transfer Instructions ==========
	template <bool forInc = false>
	void LDD();    ///< Load and decrement (BC--, DE--, HL--)
	void LDDR();   ///< LDD repeated until BC = 0
	void LDI();    ///< Load and increment (BC--, DE++, HL++)
	void LDIR();   ///< LDI repeated until BC = 0

	// ========== Block Compare Instructions ==========
	template <bool forInc = false>
	void CPD();    ///< Compare and decrement
	void CPDR();   ///< CPD repeated until match or BC = 0
	void CPI();    ///< Compare and increment
	void CPIR();   ///< CPI repeated until match or BC = 0

	// ========== Block I/O Instructions ==========
	template <bool forInc = false>
	void OUTD();   ///< Output and decrement
	void OTDR();   ///< OUTD repeated until B = 0
	void OUTI();   ///< Output and increment
	void OTIR();   ///< OUTI repeated until B = 0

	template <bool forInc = false>
	void IND();    ///< Input and decrement
	void INDR();   ///< IND repeated until B = 0
	void INI();    ///< Input and increment
	void INIR();   ///< INI repeated until B = 0

	void UpdateInOutRepeatFlags();  ///< Updates flags for block I/O ops

	void IncrementR();  ///< Increments R register (memory refresh counter)

	void InitPostBiosState();  ///< Initializes state after BIOS execution

public:
	void Init(Emulator* emu, SmsConsole* console, SmsMemoryManager* memoryManager);

	SmsCpuState& GetState();

	[[nodiscard]] uint64_t GetCycleCount() { return _state.CycleCount; }

	void SetIrqSource(SmsIrqSource source) { _state.ActiveIrqs |= (int)source; }
	void ClearIrqSource(SmsIrqSource source) { _state.ActiveIrqs &= ~(int)source; }
	void SetNmiLevel(bool nmiLevel);  ///< Sets NMI input level (edge-triggered)

	void Exec();  ///< Executes one instruction

	void Serialize(Serializer& s) override;

#ifdef DUMMYCPU
private:
	uint32_t _memOpCounter = 0;
	MemoryOperationInfo _memOperations[10] = {};

public:
	void SetDummyState(SmsCpuState& state);
	uint32_t GetOperationCount();
	void LogMemoryOperation(uint32_t addr, uint8_t value, MemoryOperationType type, MemoryType memType);
	MemoryOperationInfo GetOperationInfo(uint32_t index);
#endif
};
#endif
