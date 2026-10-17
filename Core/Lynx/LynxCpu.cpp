#include "pch.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxMemoryManager.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

#ifndef DUMMYCPU
LynxCpu::LynxCpu(Emulator* emu, LynxConsole* console, LynxMemoryManager* memoryManager) {
	_emu = emu;
	_console = console;
	_memoryManager = memoryManager;

	InitOpTable();

	// Initial state: reset vector
	_state.SP = 0xfd;
	_state.PS = LynxPSFlags::Interrupt | LynxPSFlags::Reserved;
	_state.CycleCount = 0;
	_state.StopState = LynxCpuStopState::Running;

	// Read reset vector ($FFFC-$FFFD)
	_state.PC = MemoryReadWord(0xfffc);
}
#endif

// =============================================================================
// Memory Access — each access is one CPU cycle
// =============================================================================

#ifndef DUMMYCPU
uint8_t LynxCpu::MemoryRead(uint16_t addr, MemoryOperationType opType) {
	_state.CycleCount++;
	return _memoryManager->Read(addr, opType);
}

void LynxCpu::MemoryWrite(uint16_t addr, uint8_t value, MemoryOperationType opType) {
	_state.CycleCount++;
	_memoryManager->Write(addr, value, opType);
}
#endif

// =============================================================================
// Exec — execute one instruction
// =============================================================================

void LynxCpu::Exec() {
	// Handle stop states
	if (_state.StopState != LynxCpuStopState::Running) [[unlikely]] {
		if (_state.StopState == LynxCpuStopState::WaitingForIrq) {
			_state.CycleCount++;
			if (_irqPending) {
				// WDC 65C02 WAI behavior: wake on ANY IRQ assertion
				// - If I=0: wake and vector to IRQ handler (normal flow below)
				// - If I=1: wake and continue at next instruction (skip vectoring)
				_state.StopState = LynxCpuStopState::Running;
				if (CheckFlag(LynxPSFlags::Interrupt)) {
					// I=1: IRQs masked, just continue execution without vectoring
					return;
				}
				// I=0: fall through to execute next instruction (IRQ will be handled)
			} else {
				return;
			}
		} else {
			// STP — halted until reset
			_state.CycleCount++;
			return;
		}
	}

#ifndef DUMMYCPU
	_emu->ProcessInstruction<CpuType::Lynx>();
#endif

	// Fetch opcode
	uint8_t opCode = MemoryRead(_state.PC, MemoryOperationType::ExecOpCode);
	_state.PC++;

	// Decode addressing mode and fetch operand
	_instAddrMode = _addrMode[opCode];
	_operand = FetchOperand();

	// Execute instruction
	(this->*_opTable[opCode])();

	// Check IRQ after instruction
	if (_irqPending && !CheckFlag(LynxPSFlags::Interrupt)) [[unlikely]] {
		HandleIrq();
	}
}

// =============================================================================
// Operand Fetching
// =============================================================================

uint16_t LynxCpu::FetchOperand() {
	switch (_instAddrMode) {
		case LynxAddrMode::Acc:
		case LynxAddrMode::Imp:      DummyRead(); return 0;
		case LynxAddrMode::Imm:
		case LynxAddrMode::Rel:      return GetImmediate();
		case LynxAddrMode::Zpg:      return GetZpgAddr();
		case LynxAddrMode::ZpgX:     return GetZpgXAddr();
		case LynxAddrMode::ZpgY:     return GetZpgYAddr();
		case LynxAddrMode::Abs:      return GetAbsAddr();
		case LynxAddrMode::AbsX:     return GetAbsXAddr(false);
		case LynxAddrMode::AbsXW:    return GetAbsXAddr(true);
		case LynxAddrMode::AbsY:     return GetAbsYAddr(false);
		case LynxAddrMode::AbsYW:    return GetAbsYAddr(true);
		case LynxAddrMode::Ind:      return GetIndAddr();
		case LynxAddrMode::IndX:     return GetIndXAddr();
		case LynxAddrMode::IndY:     return GetIndYAddr(false);
		case LynxAddrMode::IndYW:    return GetIndYAddr(true);
		case LynxAddrMode::ZpgInd:   return GetZpgIndAddr();
		case LynxAddrMode::AbsIndX:  return GetAbsIndXAddr();
		case LynxAddrMode::None:
		default:                     return 0;
	}
}

// =============================================================================
// Branches
// =============================================================================

void LynxCpu::BranchRelative(bool branch) {
	int8_t offset = static_cast<int8_t>(GetOperand());
	if (branch) {
		DummyRead();
		if (CheckPageCrossed(_state.PC, offset)) {
			DummyRead(); // Page cross penalty
		}
		SetPC(_state.PC + offset);
	}
}

// =============================================================================
// ADC / SBC — with 65C02 decimal mode fixes
// =============================================================================

void LynxCpu::ADC() {
	uint8_t operand = GetOperandValue();
	uint8_t carry = CheckFlag(LynxPSFlags::Carry) ? 1 : 0;

	if (CheckFlag(LynxPSFlags::Decimal)) [[unlikely]] {
		// 65C02 BCD mode — correct flag behavior
		uint8_t al = (A() & 0x0f) + (operand & 0x0f) + carry;
		if (al > 9) al += 6;
		uint16_t ah = (A() >> 4) + (operand >> 4) + (al > 15 ? 1 : 0);
		al &= 0x0f;

		// Overflow uses binary arithmetic result
		uint16_t binResult = static_cast<uint16_t>(A()) + operand + carry;
		SetFlagValue(LynxPSFlags::Overflow, (~(A() ^ operand) & (A() ^ static_cast<uint8_t>(binResult)) & 0x80) != 0);

		if (ah > 9) ah += 6;
		SetFlagValue(LynxPSFlags::Carry, ah > 15);

		uint8_t result = (al & 0x0f) | ((ah & 0x0f) << 4);
		// 65C02: Z and N flags set from BCD result (unlike NMOS)
		SetZeroNeg(result);
		_state.A = result;

		DummyRead(); // 65C02: decimal mode extra cycle
	} else {
		uint16_t result = static_cast<uint16_t>(A()) + operand + carry;
		SetFlagValue(LynxPSFlags::Carry, result > 0xff);
		SetFlagValue(LynxPSFlags::Overflow, (~(A() ^ operand) & (A() ^ static_cast<uint8_t>(result)) & 0x80) != 0);
		SetA(static_cast<uint8_t>(result));
	}
}

void LynxCpu::SBC() {
	uint8_t operand = GetOperandValue();
	uint8_t carry = CheckFlag(LynxPSFlags::Carry) ? 0 : 1;

	if (CheckFlag(LynxPSFlags::Decimal)) [[unlikely]] {
		// 65C02 BCD subtraction
		int8_t al = (A() & 0x0f) - (operand & 0x0f) - carry;
		if (al < 0) al = ((al - 6) & 0x0f) | 0x80; // Borrow from high nibble
		int16_t ah = (A() >> 4) - (operand >> 4) + (al < 0 ? -1 : 0);
		al &= 0x0f;

		uint16_t binResult = static_cast<uint16_t>(A()) - operand - carry;
		SetFlagValue(LynxPSFlags::Carry, binResult < 0x100);
		SetFlagValue(LynxPSFlags::Overflow, ((A() ^ operand) & (A() ^ static_cast<uint8_t>(binResult)) & 0x80) != 0);

		if (ah < 0) ah -= 6;

		uint8_t result = (al & 0x0f) | ((ah & 0x0f) << 4);
		SetZeroNeg(result);
		_state.A = result;

		DummyRead(); // 65C02: decimal mode extra cycle
	} else {
		uint16_t result = static_cast<uint16_t>(A()) - operand - carry;
		SetFlagValue(LynxPSFlags::Carry, result < 0x100);
		SetFlagValue(LynxPSFlags::Overflow, ((A() ^ operand) & (A() ^ static_cast<uint8_t>(result)) & 0x80) != 0);
		SetA(static_cast<uint8_t>(result));
	}
}

// =============================================================================
// BIT
// =============================================================================

void LynxCpu::BIT() {
	uint8_t val = GetOperandValue();
	SetFlagValue(LynxPSFlags::Zero, (A() & val) == 0);
	SetFlagValue(LynxPSFlags::Overflow, (val & 0x40) != 0);
	SetFlagValue(LynxPSFlags::Negative, (val & 0x80) != 0);
}

void LynxCpu::BIT_Imm() {
	// 65C02: BIT immediate only affects Z flag, not N/V
	uint8_t val = GetOperandValue();
	SetFlagValue(LynxPSFlags::Zero, (A() & val) == 0);
}

// =============================================================================
// Jumps/Calls
// =============================================================================

void LynxCpu::JSR() {
	uint16_t addr = GetOperand();
	DummyRead(); // Internal operation
	PushWord(_state.PC - 1);
	SetPC(addr);
}

void LynxCpu::RTS() {
	DummyRead();
	uint16_t addr = PopWord();
	DummyRead();
	SetPC(addr + 1);
}

void LynxCpu::RTI() {
	DummyRead();
	SetPS(Pop());
	SetPC(PopWord());
}

void LynxCpu::BRK() {
	ReadByte(); // Read and discard next byte (signature byte)
	PushWord(_state.PC);
	Push(PS() | LynxPSFlags::Break | LynxPSFlags::Reserved);
	SetFlag(LynxPSFlags::Interrupt);
	ClearFlag(LynxPSFlags::Decimal); // 65C02: BRK clears D flag
	SetPC(MemoryReadWord(0xfffe));
}

// =============================================================================
// WAI / STP — 65C02 specific
// =============================================================================

void LynxCpu::WAI() {
	// Wait for interrupt — CPU halts until IRQ
	// HW Bug 13.1: On real hardware, the CPU can only be woken from sleep
	// if Suzy holds the bus (SUZY_BUSEN asserted). Without Suzy bus request,
	// a STP/WAI is permanent and requires a hardware reset. Most games avoid
	// this by ensuring sprites are being processed or by using WAI (which
	// automatically wakes on any IRQ regardless of bus state in our emulation).
	_state.StopState = LynxCpuStopState::WaitingForIrq;
}

void LynxCpu::STP() {
	// Stop — CPU halts until reset
	// HW Bug 13.1: On real hardware, STP is only recoverable from if Suzy
	// is holding the bus. We emulate this as a permanent halt (requires reset).
	_state.StopState = LynxCpuStopState::Stopped;
}

// =============================================================================
// IRQ Handling
// =============================================================================

void LynxCpu::HandleIrq() {
	DummyRead();
	DummyRead();
	PushWord(_state.PC);
	Push((PS() & ~LynxPSFlags::Break) | LynxPSFlags::Reserved);
	SetFlag(LynxPSFlags::Interrupt);
	ClearFlag(LynxPSFlags::Decimal); // 65C02: IRQ clears D flag
	SetPC(MemoryReadWord(0xfffe));
}

// =============================================================================
// Serialize
// =============================================================================

#ifndef DUMMYCPU
void LynxCpu::Serialize(Serializer& s) {
	SV(_state.PC);
	SV(_state.SP);
	SV(_state.PS);
	SV(_state.A);
	SV(_state.X);
	SV(_state.Y);
	SV(_state.CycleCount);
	SV(_state.StopState);
	SV(_irqPending);
	SV(_state.IRQFlag);
	// NmiFlag not serialized — Lynx has no NMI line
}
#endif

// =============================================================================
// Opcode Table Initialization
// =============================================================================

void LynxCpu::InitOpTable() {
	// Initialize all opcodes to NOP (undefined opcodes are NOPs on 65C02)
	for (int i = 0; i < 256; i++) {
		_opTable[i] = &LynxCpu::NOP;
		_addrMode[i] = LynxAddrMode::Imp;
	}

	// -------------------------------------------------------------------------
	// Standard 6502 opcodes (preserved from NMOS)
	// -------------------------------------------------------------------------

	// $00 BRK
	_opTable[0x00] = &LynxCpu::BRK;     _addrMode[0x00] = LynxAddrMode::None;

	// $01 ORA (zp,X)
	_opTable[0x01] = &LynxCpu::ORA;     _addrMode[0x01] = LynxAddrMode::IndX;
	// $05 ORA zp
	_opTable[0x05] = &LynxCpu::ORA;     _addrMode[0x05] = LynxAddrMode::Zpg;
	// $09 ORA #imm
	_opTable[0x09] = &LynxCpu::ORA;     _addrMode[0x09] = LynxAddrMode::Imm;
	// $0D ORA abs
	_opTable[0x0d] = &LynxCpu::ORA;     _addrMode[0x0d] = LynxAddrMode::Abs;
	// $11 ORA (zp),Y
	_opTable[0x11] = &LynxCpu::ORA;     _addrMode[0x11] = LynxAddrMode::IndY;
	// $12 ORA (zp) — 65C02
	_opTable[0x12] = &LynxCpu::ORA;     _addrMode[0x12] = LynxAddrMode::ZpgInd;
	// $15 ORA zp,X
	_opTable[0x15] = &LynxCpu::ORA;     _addrMode[0x15] = LynxAddrMode::ZpgX;
	// $19 ORA abs,Y
	_opTable[0x19] = &LynxCpu::ORA;     _addrMode[0x19] = LynxAddrMode::AbsY;
	// $1D ORA abs,X
	_opTable[0x1d] = &LynxCpu::ORA;     _addrMode[0x1d] = LynxAddrMode::AbsX;

	// $21 AND (zp,X)
	_opTable[0x21] = &LynxCpu::AND;     _addrMode[0x21] = LynxAddrMode::IndX;
	// $25 AND zp
	_opTable[0x25] = &LynxCpu::AND;     _addrMode[0x25] = LynxAddrMode::Zpg;
	// $29 AND #imm
	_opTable[0x29] = &LynxCpu::AND;     _addrMode[0x29] = LynxAddrMode::Imm;
	// $2D AND abs
	_opTable[0x2d] = &LynxCpu::AND;     _addrMode[0x2d] = LynxAddrMode::Abs;
	// $31 AND (zp),Y
	_opTable[0x31] = &LynxCpu::AND;     _addrMode[0x31] = LynxAddrMode::IndY;
	// $32 AND (zp) — 65C02
	_opTable[0x32] = &LynxCpu::AND;     _addrMode[0x32] = LynxAddrMode::ZpgInd;
	// $35 AND zp,X
	_opTable[0x35] = &LynxCpu::AND;     _addrMode[0x35] = LynxAddrMode::ZpgX;
	// $39 AND abs,Y
	_opTable[0x39] = &LynxCpu::AND;     _addrMode[0x39] = LynxAddrMode::AbsY;
	// $3D AND abs,X
	_opTable[0x3d] = &LynxCpu::AND;     _addrMode[0x3d] = LynxAddrMode::AbsX;

	// $41 EOR (zp,X)
	_opTable[0x41] = &LynxCpu::EOR;     _addrMode[0x41] = LynxAddrMode::IndX;
	// $45 EOR zp
	_opTable[0x45] = &LynxCpu::EOR;     _addrMode[0x45] = LynxAddrMode::Zpg;
	// $49 EOR #imm
	_opTable[0x49] = &LynxCpu::EOR;     _addrMode[0x49] = LynxAddrMode::Imm;
	// $4D EOR abs
	_opTable[0x4d] = &LynxCpu::EOR;     _addrMode[0x4d] = LynxAddrMode::Abs;
	// $51 EOR (zp),Y
	_opTable[0x51] = &LynxCpu::EOR;     _addrMode[0x51] = LynxAddrMode::IndY;
	// $52 EOR (zp) — 65C02
	_opTable[0x52] = &LynxCpu::EOR;     _addrMode[0x52] = LynxAddrMode::ZpgInd;
	// $55 EOR zp,X
	_opTable[0x55] = &LynxCpu::EOR;     _addrMode[0x55] = LynxAddrMode::ZpgX;
	// $59 EOR abs,Y
	_opTable[0x59] = &LynxCpu::EOR;     _addrMode[0x59] = LynxAddrMode::AbsY;
	// $5D EOR abs,X
	_opTable[0x5d] = &LynxCpu::EOR;     _addrMode[0x5d] = LynxAddrMode::AbsX;

	// $61 ADC (zp,X)
	_opTable[0x61] = &LynxCpu::ADC;     _addrMode[0x61] = LynxAddrMode::IndX;
	// $65 ADC zp
	_opTable[0x65] = &LynxCpu::ADC;     _addrMode[0x65] = LynxAddrMode::Zpg;
	// $69 ADC #imm
	_opTable[0x69] = &LynxCpu::ADC;     _addrMode[0x69] = LynxAddrMode::Imm;
	// $6D ADC abs
	_opTable[0x6d] = &LynxCpu::ADC;     _addrMode[0x6d] = LynxAddrMode::Abs;
	// $71 ADC (zp),Y
	_opTable[0x71] = &LynxCpu::ADC;     _addrMode[0x71] = LynxAddrMode::IndY;
	// $72 ADC (zp) — 65C02
	_opTable[0x72] = &LynxCpu::ADC;     _addrMode[0x72] = LynxAddrMode::ZpgInd;
	// $75 ADC zp,X
	_opTable[0x75] = &LynxCpu::ADC;     _addrMode[0x75] = LynxAddrMode::ZpgX;
	// $79 ADC abs,Y
	_opTable[0x79] = &LynxCpu::ADC;     _addrMode[0x79] = LynxAddrMode::AbsY;
	// $7D ADC abs,X
	_opTable[0x7d] = &LynxCpu::ADC;     _addrMode[0x7d] = LynxAddrMode::AbsX;

	// $E1 SBC (zp,X)
	_opTable[0xe1] = &LynxCpu::SBC;     _addrMode[0xe1] = LynxAddrMode::IndX;
	// $E5 SBC zp
	_opTable[0xe5] = &LynxCpu::SBC;     _addrMode[0xe5] = LynxAddrMode::Zpg;
	// $E9 SBC #imm
	_opTable[0xe9] = &LynxCpu::SBC;     _addrMode[0xe9] = LynxAddrMode::Imm;
	// $ED SBC abs
	_opTable[0xed] = &LynxCpu::SBC;     _addrMode[0xed] = LynxAddrMode::Abs;
	// $F1 SBC (zp),Y
	_opTable[0xf1] = &LynxCpu::SBC;     _addrMode[0xf1] = LynxAddrMode::IndY;
	// $F2 SBC (zp) — 65C02
	_opTable[0xf2] = &LynxCpu::SBC;     _addrMode[0xf2] = LynxAddrMode::ZpgInd;
	// $F5 SBC zp,X
	_opTable[0xf5] = &LynxCpu::SBC;     _addrMode[0xf5] = LynxAddrMode::ZpgX;
	// $F9 SBC abs,Y
	_opTable[0xf9] = &LynxCpu::SBC;     _addrMode[0xf9] = LynxAddrMode::AbsY;
	// $FD SBC abs,X
	_opTable[0xfd] = &LynxCpu::SBC;     _addrMode[0xfd] = LynxAddrMode::AbsX;

	// --- CMP ---
	_opTable[0xc1] = &LynxCpu::CMP;     _addrMode[0xc1] = LynxAddrMode::IndX;
	_opTable[0xc5] = &LynxCpu::CMP;     _addrMode[0xc5] = LynxAddrMode::Zpg;
	_opTable[0xc9] = &LynxCpu::CMP;     _addrMode[0xc9] = LynxAddrMode::Imm;
	_opTable[0xcd] = &LynxCpu::CMP;     _addrMode[0xcd] = LynxAddrMode::Abs;
	_opTable[0xd1] = &LynxCpu::CMP;     _addrMode[0xd1] = LynxAddrMode::IndY;
	_opTable[0xd2] = &LynxCpu::CMP;     _addrMode[0xd2] = LynxAddrMode::ZpgInd;  // 65C02
	_opTable[0xd5] = &LynxCpu::CMP;     _addrMode[0xd5] = LynxAddrMode::ZpgX;
	_opTable[0xd9] = &LynxCpu::CMP;     _addrMode[0xd9] = LynxAddrMode::AbsY;
	_opTable[0xdd] = &LynxCpu::CMP;     _addrMode[0xdd] = LynxAddrMode::AbsX;

	// --- CPX ---
	_opTable[0xe0] = &LynxCpu::CPX;     _addrMode[0xe0] = LynxAddrMode::Imm;
	_opTable[0xe4] = &LynxCpu::CPX;     _addrMode[0xe4] = LynxAddrMode::Zpg;
	_opTable[0xec] = &LynxCpu::CPX;     _addrMode[0xec] = LynxAddrMode::Abs;

	// --- CPY ---
	_opTable[0xc0] = &LynxCpu::CPY;     _addrMode[0xc0] = LynxAddrMode::Imm;
	_opTable[0xc4] = &LynxCpu::CPY;     _addrMode[0xc4] = LynxAddrMode::Zpg;
	_opTable[0xcc] = &LynxCpu::CPY;     _addrMode[0xcc] = LynxAddrMode::Abs;

	// --- LDA ---
	_opTable[0xa1] = &LynxCpu::LDA;     _addrMode[0xa1] = LynxAddrMode::IndX;
	_opTable[0xa5] = &LynxCpu::LDA;     _addrMode[0xa5] = LynxAddrMode::Zpg;
	_opTable[0xa9] = &LynxCpu::LDA;     _addrMode[0xa9] = LynxAddrMode::Imm;
	_opTable[0xad] = &LynxCpu::LDA;     _addrMode[0xad] = LynxAddrMode::Abs;
	_opTable[0xb1] = &LynxCpu::LDA;     _addrMode[0xb1] = LynxAddrMode::IndY;
	_opTable[0xb2] = &LynxCpu::LDA;     _addrMode[0xb2] = LynxAddrMode::ZpgInd;  // 65C02
	_opTable[0xb5] = &LynxCpu::LDA;     _addrMode[0xb5] = LynxAddrMode::ZpgX;
	_opTable[0xb9] = &LynxCpu::LDA;     _addrMode[0xb9] = LynxAddrMode::AbsY;
	_opTable[0xbd] = &LynxCpu::LDA;     _addrMode[0xbd] = LynxAddrMode::AbsX;

	// --- LDX ---
	_opTable[0xa2] = &LynxCpu::LDX;     _addrMode[0xa2] = LynxAddrMode::Imm;
	_opTable[0xa6] = &LynxCpu::LDX;     _addrMode[0xa6] = LynxAddrMode::Zpg;
	_opTable[0xae] = &LynxCpu::LDX;     _addrMode[0xae] = LynxAddrMode::Abs;
	_opTable[0xb6] = &LynxCpu::LDX;     _addrMode[0xb6] = LynxAddrMode::ZpgY;
	_opTable[0xbe] = &LynxCpu::LDX;     _addrMode[0xbe] = LynxAddrMode::AbsY;

	// --- LDY ---
	_opTable[0xa0] = &LynxCpu::LDY;     _addrMode[0xa0] = LynxAddrMode::Imm;
	_opTable[0xa4] = &LynxCpu::LDY;     _addrMode[0xa4] = LynxAddrMode::Zpg;
	_opTable[0xac] = &LynxCpu::LDY;     _addrMode[0xac] = LynxAddrMode::Abs;
	_opTable[0xb4] = &LynxCpu::LDY;     _addrMode[0xb4] = LynxAddrMode::ZpgX;
	_opTable[0xbc] = &LynxCpu::LDY;     _addrMode[0xbc] = LynxAddrMode::AbsX;

	// --- STA ---
	_opTable[0x81] = &LynxCpu::STA;     _addrMode[0x81] = LynxAddrMode::IndX;
	_opTable[0x85] = &LynxCpu::STA;     _addrMode[0x85] = LynxAddrMode::Zpg;
	_opTable[0x8d] = &LynxCpu::STA;     _addrMode[0x8d] = LynxAddrMode::Abs;
	_opTable[0x91] = &LynxCpu::STA;     _addrMode[0x91] = LynxAddrMode::IndYW;
	_opTable[0x92] = &LynxCpu::STA;     _addrMode[0x92] = LynxAddrMode::ZpgInd;  // 65C02
	_opTable[0x95] = &LynxCpu::STA;     _addrMode[0x95] = LynxAddrMode::ZpgX;
	_opTable[0x99] = &LynxCpu::STA;     _addrMode[0x99] = LynxAddrMode::AbsYW;
	_opTable[0x9d] = &LynxCpu::STA;     _addrMode[0x9d] = LynxAddrMode::AbsXW;

	// --- STX ---
	_opTable[0x86] = &LynxCpu::STX;     _addrMode[0x86] = LynxAddrMode::Zpg;
	_opTable[0x8e] = &LynxCpu::STX;     _addrMode[0x8e] = LynxAddrMode::Abs;
	_opTable[0x96] = &LynxCpu::STX;     _addrMode[0x96] = LynxAddrMode::ZpgY;

	// --- STY ---
	_opTable[0x84] = &LynxCpu::STY;     _addrMode[0x84] = LynxAddrMode::Zpg;
	_opTable[0x8c] = &LynxCpu::STY;     _addrMode[0x8c] = LynxAddrMode::Abs;
	_opTable[0x94] = &LynxCpu::STY;     _addrMode[0x94] = LynxAddrMode::ZpgX;

	// --- STZ (65C02) ---
	_opTable[0x64] = &LynxCpu::STZ;     _addrMode[0x64] = LynxAddrMode::Zpg;
	_opTable[0x74] = &LynxCpu::STZ;     _addrMode[0x74] = LynxAddrMode::ZpgX;
	_opTable[0x9c] = &LynxCpu::STZ;     _addrMode[0x9c] = LynxAddrMode::Abs;
	_opTable[0x9e] = &LynxCpu::STZ;     _addrMode[0x9e] = LynxAddrMode::AbsXW;

	// --- Transfer ---
	_opTable[0xaa] = &LynxCpu::TAX;     _addrMode[0xaa] = LynxAddrMode::Imp;
	_opTable[0xa8] = &LynxCpu::TAY;     _addrMode[0xa8] = LynxAddrMode::Imp;
	_opTable[0x8a] = &LynxCpu::TXA;     _addrMode[0x8a] = LynxAddrMode::Imp;
	_opTable[0x98] = &LynxCpu::TYA;     _addrMode[0x98] = LynxAddrMode::Imp;
	_opTable[0xba] = &LynxCpu::TSX;     _addrMode[0xba] = LynxAddrMode::Imp;
	_opTable[0x9a] = &LynxCpu::TXS;     _addrMode[0x9a] = LynxAddrMode::Imp;

	// --- Stack ---
	_opTable[0x48] = &LynxCpu::PHA;     _addrMode[0x48] = LynxAddrMode::Imp;
	_opTable[0x68] = &LynxCpu::PLA;     _addrMode[0x68] = LynxAddrMode::Imp;
	_opTable[0x08] = &LynxCpu::PHP;     _addrMode[0x08] = LynxAddrMode::Imp;
	_opTable[0x28] = &LynxCpu::PLP;     _addrMode[0x28] = LynxAddrMode::Imp;
	// 65C02 stack ops
	_opTable[0xda] = &LynxCpu::PHX;     _addrMode[0xda] = LynxAddrMode::Imp;
	_opTable[0xfa] = &LynxCpu::PLX;     _addrMode[0xfa] = LynxAddrMode::Imp;
	_opTable[0x5a] = &LynxCpu::PHY;     _addrMode[0x5a] = LynxAddrMode::Imp;
	_opTable[0x7a] = &LynxCpu::PLY;     _addrMode[0x7a] = LynxAddrMode::Imp;

	// --- INC/DEC ---
	_opTable[0xe6] = &LynxCpu::INC;     _addrMode[0xe6] = LynxAddrMode::Zpg;
	_opTable[0xee] = &LynxCpu::INC;     _addrMode[0xee] = LynxAddrMode::Abs;
	_opTable[0xf6] = &LynxCpu::INC;     _addrMode[0xf6] = LynxAddrMode::ZpgX;
	_opTable[0xfe] = &LynxCpu::INC;     _addrMode[0xfe] = LynxAddrMode::AbsXW;
	_opTable[0x1a] = &LynxCpu::INC_A;   _addrMode[0x1a] = LynxAddrMode::Acc;  // 65C02 INC A
	_opTable[0xc6] = &LynxCpu::DEC;     _addrMode[0xc6] = LynxAddrMode::Zpg;
	_opTable[0xce] = &LynxCpu::DEC;     _addrMode[0xce] = LynxAddrMode::Abs;
	_opTable[0xd6] = &LynxCpu::DEC;     _addrMode[0xd6] = LynxAddrMode::ZpgX;
	_opTable[0xde] = &LynxCpu::DEC;     _addrMode[0xde] = LynxAddrMode::AbsXW;
	_opTable[0x3a] = &LynxCpu::DEC_A;   _addrMode[0x3a] = LynxAddrMode::Acc;  // 65C02 DEC A
	_opTable[0xe8] = &LynxCpu::INX;     _addrMode[0xe8] = LynxAddrMode::Imp;
	_opTable[0xc8] = &LynxCpu::INY;     _addrMode[0xc8] = LynxAddrMode::Imp;
	_opTable[0xca] = &LynxCpu::DEX;     _addrMode[0xca] = LynxAddrMode::Imp;
	_opTable[0x88] = &LynxCpu::DEY;     _addrMode[0x88] = LynxAddrMode::Imp;

	// --- ASL ---
	_opTable[0x0a] = &LynxCpu::ASL_A;   _addrMode[0x0a] = LynxAddrMode::Acc;
	_opTable[0x06] = &LynxCpu::ASL;     _addrMode[0x06] = LynxAddrMode::Zpg;
	_opTable[0x0e] = &LynxCpu::ASL;     _addrMode[0x0e] = LynxAddrMode::Abs;
	_opTable[0x16] = &LynxCpu::ASL;     _addrMode[0x16] = LynxAddrMode::ZpgX;
	_opTable[0x1e] = &LynxCpu::ASL;     _addrMode[0x1e] = LynxAddrMode::AbsXW;

	// --- LSR ---
	_opTable[0x4a] = &LynxCpu::LSR_A;   _addrMode[0x4a] = LynxAddrMode::Acc;
	_opTable[0x46] = &LynxCpu::LSR;     _addrMode[0x46] = LynxAddrMode::Zpg;
	_opTable[0x4e] = &LynxCpu::LSR;     _addrMode[0x4e] = LynxAddrMode::Abs;
	_opTable[0x56] = &LynxCpu::LSR;     _addrMode[0x56] = LynxAddrMode::ZpgX;
	_opTable[0x5e] = &LynxCpu::LSR;     _addrMode[0x5e] = LynxAddrMode::AbsXW;

	// --- ROL ---
	_opTable[0x2a] = &LynxCpu::ROL_A;   _addrMode[0x2a] = LynxAddrMode::Acc;
	_opTable[0x26] = &LynxCpu::ROL;     _addrMode[0x26] = LynxAddrMode::Zpg;
	_opTable[0x2e] = &LynxCpu::ROL;     _addrMode[0x2e] = LynxAddrMode::Abs;
	_opTable[0x36] = &LynxCpu::ROL;     _addrMode[0x36] = LynxAddrMode::ZpgX;
	_opTable[0x3e] = &LynxCpu::ROL;     _addrMode[0x3e] = LynxAddrMode::AbsXW;

	// --- ROR ---
	_opTable[0x6a] = &LynxCpu::ROR_A;   _addrMode[0x6a] = LynxAddrMode::Acc;
	_opTable[0x66] = &LynxCpu::ROR;     _addrMode[0x66] = LynxAddrMode::Zpg;
	_opTable[0x6e] = &LynxCpu::ROR;     _addrMode[0x6e] = LynxAddrMode::Abs;
	_opTable[0x76] = &LynxCpu::ROR;     _addrMode[0x76] = LynxAddrMode::ZpgX;
	_opTable[0x7e] = &LynxCpu::ROR;     _addrMode[0x7e] = LynxAddrMode::AbsXW;

	// --- BIT ---
	_opTable[0x24] = &LynxCpu::BIT;     _addrMode[0x24] = LynxAddrMode::Zpg;
	_opTable[0x2c] = &LynxCpu::BIT;     _addrMode[0x2c] = LynxAddrMode::Abs;
	// 65C02 BIT extensions
	_opTable[0x89] = &LynxCpu::BIT_Imm; _addrMode[0x89] = LynxAddrMode::Imm;
	_opTable[0x34] = &LynxCpu::BIT;     _addrMode[0x34] = LynxAddrMode::ZpgX;
	_opTable[0x3c] = &LynxCpu::BIT;     _addrMode[0x3c] = LynxAddrMode::AbsX;

	// --- TRB/TSB (65C02) ---
	_opTable[0x04] = &LynxCpu::TSB;     _addrMode[0x04] = LynxAddrMode::Zpg;
	_opTable[0x0c] = &LynxCpu::TSB;     _addrMode[0x0c] = LynxAddrMode::Abs;
	_opTable[0x14] = &LynxCpu::TRB;     _addrMode[0x14] = LynxAddrMode::Zpg;
	_opTable[0x1c] = &LynxCpu::TRB;     _addrMode[0x1c] = LynxAddrMode::Abs;

	// --- Branches ---
	_opTable[0x90] = &LynxCpu::BCC;     _addrMode[0x90] = LynxAddrMode::Rel;
	_opTable[0xb0] = &LynxCpu::BCS;     _addrMode[0xb0] = LynxAddrMode::Rel;
	_opTable[0xf0] = &LynxCpu::BEQ;     _addrMode[0xf0] = LynxAddrMode::Rel;
	_opTable[0xd0] = &LynxCpu::BNE;     _addrMode[0xd0] = LynxAddrMode::Rel;
	_opTable[0x30] = &LynxCpu::BMI;     _addrMode[0x30] = LynxAddrMode::Rel;
	_opTable[0x10] = &LynxCpu::BPL;     _addrMode[0x10] = LynxAddrMode::Rel;
	_opTable[0x70] = &LynxCpu::BVS;     _addrMode[0x70] = LynxAddrMode::Rel;
	_opTable[0x50] = &LynxCpu::BVC;     _addrMode[0x50] = LynxAddrMode::Rel;
	_opTable[0x80] = &LynxCpu::BRA;     _addrMode[0x80] = LynxAddrMode::Rel;  // 65C02

	// --- Jumps/Calls ---
	_opTable[0x4c] = &LynxCpu::JMP;     _addrMode[0x4c] = LynxAddrMode::Abs;
	_opTable[0x6c] = &LynxCpu::JMP;     _addrMode[0x6c] = LynxAddrMode::Ind;
	_opTable[0x7c] = &LynxCpu::JMP;     _addrMode[0x7c] = LynxAddrMode::AbsIndX;  // 65C02 JMP (abs,X)
	_opTable[0x20] = &LynxCpu::JSR;     _addrMode[0x20] = LynxAddrMode::Abs;
	_opTable[0x60] = &LynxCpu::RTS;     _addrMode[0x60] = LynxAddrMode::Imp;
	_opTable[0x40] = &LynxCpu::RTI;     _addrMode[0x40] = LynxAddrMode::Imp;

	// --- Flag Set/Clear ---
	_opTable[0x18] = &LynxCpu::CLC;     _addrMode[0x18] = LynxAddrMode::Imp;
	_opTable[0x38] = &LynxCpu::SEC;     _addrMode[0x38] = LynxAddrMode::Imp;
	_opTable[0xd8] = &LynxCpu::CLD;     _addrMode[0xd8] = LynxAddrMode::Imp;
	_opTable[0xf8] = &LynxCpu::SED;     _addrMode[0xf8] = LynxAddrMode::Imp;
	_opTable[0x58] = &LynxCpu::CLI;     _addrMode[0x58] = LynxAddrMode::Imp;
	_opTable[0x78] = &LynxCpu::SEI;     _addrMode[0x78] = LynxAddrMode::Imp;
	_opTable[0xb8] = &LynxCpu::CLV;     _addrMode[0xb8] = LynxAddrMode::Imp;

	// --- NOP ---
	_opTable[0xea] = &LynxCpu::NOP;     _addrMode[0xea] = LynxAddrMode::Imp;

	// --- 65C02: WAI/STP ---
	_opTable[0xcb] = &LynxCpu::WAI;     _addrMode[0xcb] = LynxAddrMode::Imp;
	_opTable[0xdb] = &LynxCpu::STP;     _addrMode[0xdb] = LynxAddrMode::Imp;

	// -------------------------------------------------------------------------
	// Multi-byte NOPs for undefined opcodes
	// Many undefined 65C02 opcodes consume extra operand bytes
	// -------------------------------------------------------------------------
	// 2-byte NOPs (read and discard immediate byte)
	uint8_t twoByteNops[] = {
		0x02, 0x22, 0x42, 0x62, 0x82, 0xc2, 0xe2,
		0x44, 0x54, 0xd4
	};
	for (uint8_t op : twoByteNops) {
		_opTable[op] = &LynxCpu::NOP_Imm;
		_addrMode[op] = LynxAddrMode::Imm;
	}

	// 3-byte NOPs (read and discard absolute address) — $5C, $DC, $FC
	// WDC 65C02 specific cycle counts: $5C = 8 cycles, $DC/$FC = 4 cycles
	_opTable[0x5c] = &LynxCpu::NOP_5C;
	_addrMode[0x5c] = LynxAddrMode::Abs;
	_opTable[0xdc] = &LynxCpu::NOP_DC;
	_addrMode[0xdc] = LynxAddrMode::Abs;
	_opTable[0xfc] = &LynxCpu::NOP_DC;
	_addrMode[0xfc] = LynxAddrMode::Abs;
}
