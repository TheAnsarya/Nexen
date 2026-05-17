#include "pch.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Shared/MessageManager.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/Serializer.h"
#include <algorithm>
#include <cstdlib>
#include <functional>

namespace {
	const char* kBranchCondNames[16] = {
		"t", "f", "hi", "ls", "cc", "cs", "ne", "eq",
		"vc", "vs", "pl", "mi", "ge", "lt", "gt", "le"
	};

	uint64_t HashTraceText(uint64_t hash, const string& text) {
		for (uint8_t ch : text) {
			hash ^= ch;
			hash *= 1099511628211ull;
		}
		return hash;
	}

	bool TryReadEnvU32(const char* name, uint32_t minValue, uint32_t maxValue, uint32_t& outValue) {
		const char* raw = getenv(name);
		if (!raw || !*raw) {
			return false;
		}

		char* end = nullptr;
		unsigned long parsed = strtoul(raw, &end, 0);
		if (end == raw || *end != '\0' || parsed < minValue || parsed > maxValue) {
			return false;
		}

		outValue = (uint32_t)parsed;
		return true;
	}

	string FormatM68kEa(uint8_t mode, uint8_t reg, uint8_t sizeBytes, uint32_t extAddr, const std::function<uint16_t(uint32_t)>& readWord, uint32_t& consumedBytes) {
		consumedBytes = 0;
		switch (mode) {
			case 0: return "D" + std::to_string(reg);
			case 1: return "A" + std::to_string(reg);
			case 2: return "(A" + std::to_string(reg) + ")";
			case 3: return "(A" + std::to_string(reg) + ")+";
			case 4: return "-(A" + std::to_string(reg) + ")";
			case 5: {
				int16_t d16 = (int16_t)readWord(extAddr);
				consumedBytes = 2;
				return "$" + HexUtilities::ToHex((uint16_t)d16) + "(A" + std::to_string(reg) + ")";
			}
			case 6: {
				uint16_t ext = readWord(extAddr);
				consumedBytes = 2;
				int8_t d8 = (int8_t)(ext & 0xff);
				uint8_t xn = (ext >> 12) & 7;
				bool isAn = (ext & 0x8000) != 0;
				bool longSize = (ext & 0x0800) != 0;
				return "$" + HexUtilities::ToHex((uint8_t)d8) + "(A" + std::to_string(reg) + "," + (isAn ? "A" : "D") + std::to_string(xn) + "." + (longSize ? "l" : "w") + ")";
			}
			case 7:
				switch (reg) {
					case 0: {
						uint16_t absw = readWord(extAddr);
						consumedBytes = 2;
						return "$" + HexUtilities::ToHex(absw) + ".w";
					}
					case 1: {
						uint32_t absl = ((uint32_t)readWord(extAddr) << 16) | readWord(extAddr + 2);
						consumedBytes = 4;
						return "$" + HexUtilities::ToHex32(absl) + ".l";
					}
					case 2: {
						int16_t d16 = (int16_t)readWord(extAddr);
						consumedBytes = 2;
						uint32_t target = (extAddr + 2 + d16) & 0x00ffffff;
						return "$" + HexUtilities::ToHex24((int32_t)target) + "(pc)";
					}
					case 3: {
						uint16_t ext = readWord(extAddr);
						consumedBytes = 2;
						int8_t d8 = (int8_t)(ext & 0xff);
						uint8_t xn = (ext >> 12) & 7;
						bool isAn = (ext & 0x8000) != 0;
						bool longSize = (ext & 0x0800) != 0;
						return "$" + HexUtilities::ToHex((uint8_t)d8) + "(pc," + (isAn ? "A" : "D") + std::to_string(xn) + "." + (longSize ? "l" : "w") + ")";
					}
					case 4: {
						if (sizeBytes == 4) {
							uint32_t imm = ((uint32_t)readWord(extAddr) << 16) | readWord(extAddr + 2);
							consumedBytes = 4;
							return "#$" + HexUtilities::ToHex32(imm);
						}

						uint16_t immWord = readWord(extAddr);
						consumedBytes = 2;
						if (sizeBytes == 1) {
							return "#$" + HexUtilities::ToHex((uint8_t)immWord);
						}
						return "#$" + HexUtilities::ToHex(immWord);
					}
			}
			break;
		}
		return "ea";
	}

	string DisassembleM68kLine(uint32_t pc, uint16_t opcode, uint16_t operandWordA, uint16_t operandWordB, const std::function<uint16_t(uint32_t)>& readWord) {
		if (opcode == 0x4e71) return "nop";
		if (opcode == 0x4e75) return "rts";
		if (opcode == 0x4e73) return "rte";
		if (opcode == 0x4e77) return "rtr";
		if (opcode == 0x4e70) return "reset";
		if (opcode == 0x4e76) return "trapv";
		if (opcode == 0x4e72) return "stop";
		if ((opcode & 0xfff0) == 0x4e40) {
			return "trap #" + std::to_string(opcode & 0x0f);
		}

		if ((opcode & 0xff00) == 0x6000 || (opcode & 0xff00) == 0x6100 || ((opcode & 0xf000) == 0x6000)) {
			uint8_t cc = (uint8_t)((opcode >> 8) & 0x0f);
			string mnemonic = (cc == 0) ? "bra" : (cc == 1 ? "bsr" : ("b" + string(kBranchCondNames[cc])));
			int32_t disp = 0;
			uint32_t base = (pc + 2) & 0x00ffffff;
			if ((opcode & 0x00ff) == 0) {
				disp = (int16_t)operandWordA;
				base = (base + 2) & 0x00ffffff;
			} else {
				disp = (int8_t)(opcode & 0x00ff);
			}

			uint32_t target = (base + disp) & 0x00ffffff;
			return mnemonic + " $" + HexUtilities::ToHex24((int32_t)target);
		}

		if ((opcode & 0xf100) == 0x7000) {
			uint8_t reg = (uint8_t)((opcode >> 9) & 7);
			int8_t imm = (int8_t)(opcode & 0xff);
			return "moveq #" + std::to_string((int)imm) + ",D" + std::to_string(reg);
		}

		if ((opcode & 0xffc0) == 0x4e80 || (opcode & 0xffc0) == 0x4ec0) {
			uint8_t mode = (uint8_t)((opcode >> 3) & 7);
			uint8_t reg = (uint8_t)(opcode & 7);
			uint32_t used = 0;
			string ea = FormatM68kEa(mode, reg, 4, pc + 2, readWord, used);
			return ((opcode & 0xffc0) == 0x4e80 ? "jsr " : "jmp ") + ea;
		}

		uint8_t top = (uint8_t)(opcode >> 12);
		if (top >= 1 && top <= 3) {
			uint8_t sizeBytes = top == 1 ? 1 : (top == 2 ? 4 : 2);
			uint8_t srcMode = (uint8_t)((opcode >> 3) & 7);
			uint8_t srcReg = (uint8_t)(opcode & 7);
			uint8_t dstMode = (uint8_t)((opcode >> 6) & 7);
			uint8_t dstReg = (uint8_t)((opcode >> 9) & 7);
			uint32_t usedSrc = 0;
			uint32_t usedDst = 0;
			string src = FormatM68kEa(srcMode, srcReg, sizeBytes, pc + 2, readWord, usedSrc);
			string dst = FormatM68kEa(dstMode, dstReg, sizeBytes, pc + 2 + usedSrc, readWord, usedDst);
			char sizeSuffix = sizeBytes == 1 ? 'b' : (sizeBytes == 2 ? 'w' : 'l');
			return string("move.") + sizeSuffix + " " + src + "," + dst;
		}

		return "dc.w $" + HexUtilities::ToHex(opcode) + ",$" + HexUtilities::ToHex(operandWordA) + ",$" + HexUtilities::ToHex(operandWordB);
	}
}

void GenesisM68k::Init(Emulator* emu, GenesisConsole* console, GenesisMemoryManager* memoryManager) {
	_emu = emu;
	_console = console;
	_memoryManager = memoryManager;
	_recentInstructionFlowLogs.clear();
	_recentInstructionFlowLogs.reserve(_recentInstructionFlowCapacity);
}

bool GenesisM68k::CheckAddressError(uint32_t addr, uint8_t size, bool isWrite, const char* sourceTag) {
	if (!_strictAddressErrorChecks || size <= 1) {
		return false;
	}

	if ((addr & 0x01u) == 0u) {
		return false;
	}

	_addressErrorCount++;
	_lastAddressErrorAddr = addr & 0x00ffffffu;
	_lastAddressErrorPc = _state.PC & 0x00ffffffu;
	_lastAddressErrorSize = size;
	_lastAddressErrorWrite = isWrite;
	_lastAddressErrorSource = sourceTag ? sourceTag : "unknown";

	if (_addressErrorCount <= 256 || (_addressErrorCount % 2048) == 0) {
		MessageManager::Log(std::format("[Genesis][M68K] AddressError #{} src={} pc=${:06x} addr=${:06x} size={} write={} sr=${:04x}",
			_addressErrorCount,
			_lastAddressErrorSource,
			_lastAddressErrorPc,
			_lastAddressErrorAddr,
			_lastAddressErrorSize,
			_lastAddressErrorWrite ? 1 : 0,
			_state.SR));
	}

	RaiseException(3);
	return true;
}

// ===== Flag operations =====

void GenesisM68k::SetFlags_Logical(uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	result &= mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);
	SetCcrIf(M68kFlags::Zero, result == 0);
	SetCcrIf(M68kFlags::Negative, IsNeg(result, size));
}

void GenesisM68k::SetFlags_Add(uint32_t src, uint32_t dst, uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);
	src &= mask; dst &= mask; uint32_t res = result & mask;

	bool sm = (src & msb) != 0;
	bool dm = (dst & msb) != 0;
	bool rm = (res & msb) != 0;

	uint32_t carryOut = 0;
	if (size == 0) {
		carryOut = (result >> 8) & 1;
	} else if (size == 1) {
		carryOut = (result >> 16) & 1;
	} else {
		carryOut = (res < src) ? 1u : 0u;
	}
	SetCcrIf(M68kFlags::Carry, carryOut != 0);
	SetCcrIf(M68kFlags::Overflow, (sm && dm && !rm) || (!sm && !dm && rm));
	SetCcrIf(M68kFlags::Zero, res == 0);
	SetCcrIf(M68kFlags::Negative, rm);
	SetCcrIf(M68kFlags::Extend, carryOut != 0);
}

void GenesisM68k::SetFlags_Sub(uint32_t src, uint32_t dst, uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);
	src &= mask; dst &= mask; uint32_t res = result & mask;

	bool sm = (src & msb) != 0;
	bool dm = (dst & msb) != 0;
	bool rm = (res & msb) != 0;

	bool carry = src > dst;
	SetCcrIf(M68kFlags::Carry, carry);
	SetCcrIf(M68kFlags::Overflow, (!sm && dm && !rm) || (sm && !dm && rm));
	SetCcrIf(M68kFlags::Zero, res == 0);
	SetCcrIf(M68kFlags::Negative, rm);
	SetCcrIf(M68kFlags::Extend, carry);
}

void GenesisM68k::SetFlags_Cmp(uint32_t src, uint32_t dst, uint32_t result, uint8_t size) {
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);
	src &= mask; dst &= mask; uint32_t res = result & mask;

	bool sm = (src & msb) != 0;
	bool dm = (dst & msb) != 0;
	bool rm = (res & msb) != 0;

	SetCcrIf(M68kFlags::Carry, src > dst);
	SetCcrIf(M68kFlags::Overflow, (!sm && dm && !rm) || (sm && !dm && rm));
	SetCcrIf(M68kFlags::Zero, res == 0);
	SetCcrIf(M68kFlags::Negative, rm);
}

// ===== Condition testing =====

bool GenesisM68k::TestCondition(uint8_t cc) {
	bool C = GetCcr(M68kFlags::Carry);
	bool V = GetCcr(M68kFlags::Overflow);
	bool Z = GetCcr(M68kFlags::Zero);
	bool N = GetCcr(M68kFlags::Negative);

	switch (cc) {
		case 0x0: return true;           // T (true)
		case 0x1: return false;          // F (false)
		case 0x2: return !C && !Z;       // HI (higher)
		case 0x3: return C || Z;         // LS (lower or same)
		case 0x4: return !C;             // CC (carry clear)
		case 0x5: return C;              // CS (carry set)
		case 0x6: return !Z;             // NE (not equal)
		case 0x7: return Z;              // EQ (equal)
		case 0x8: return !V;             // VC (overflow clear)
		case 0x9: return V;              // VS (overflow set)
		case 0xa: return !N;             // PL (plus)
		case 0xb: return N;              // MI (minus)
		case 0xc: return N == V;         // GE (greater or equal)
		case 0xd: return N != V;         // LT (less than)
		case 0xe: return !Z && (N == V); // GT (greater than)
		case 0xf: return Z || (N != V);  // LE (less or equal)
		default: return false;
	}
}

// ===== Effective address resolution =====

uint32_t GenesisM68k::GetEffectiveAddress(uint8_t mode, uint8_t reg, uint8_t size) {
	switch (mode) {
		case 0: // Dn - data register direct (no address)
			return 0;
		case 1: // An - address register direct (no address)
			return 0;
		case 2: // (An) - address register indirect
			return _state.A[reg];
		case 3: // (An)+ - postincrement
		{
			uint32_t addr = _state.A[reg];
			uint32_t inc = (size == 0) ? 1 : (size == 1 ? 2 : 4);
			if (size == 0 && reg == 7) inc = 2; // SP always word-aligned
			_state.A[reg] += inc;
			return addr;
		}
		case 4: // -(An) - predecrement
		{
			uint32_t dec = (size == 0) ? 1 : (size == 1 ? 2 : 4);
			if (size == 0 && reg == 7) dec = 2;
			_state.A[reg] -= dec;
			return _state.A[reg];
		}
		case 5: // (d16, An) - displacement
		{
			int16_t disp = (int16_t)FetchWord();
			return _state.A[reg] + disp;
		}
		case 6: // (d8, An, Xn) - index
		{
			uint16_t ext = FetchWord();
			uint8_t xreg = (ext >> 12) & 0x0f;
			bool isLong = (ext >> 11) & 1;
			int8_t disp = (int8_t)(ext & 0xff);
			int32_t xval;
			if (xreg < 8) {
				xval = isLong ? (int32_t)_state.D[xreg] : (int16_t)(uint16_t)_state.D[xreg];
			} else {
				xval = isLong ? (int32_t)_state.A[xreg - 8] : (int16_t)(uint16_t)_state.A[xreg - 8];
			}
			return _state.A[reg] + disp + xval;
		}
		case 7: // Absolute/PC modes
			switch (reg) {
				case 0: // (xxx).W - absolute short
					return (int16_t)FetchWord();
				case 1: // (xxx).L - absolute long
					return FetchLong();
				case 2: // (d16, PC) - PC displacement
				{
					uint32_t pc = _state.PC;
					int16_t disp = (int16_t)FetchWord();
					return pc + disp;
				}
				case 3: // (d8, PC, Xn) - PC index
				{
					uint32_t pc = _state.PC;
					uint16_t ext = FetchWord();
					uint8_t xreg = (ext >> 12) & 0x0f;
					bool isLong = (ext >> 11) & 1;
					int8_t disp = (int8_t)(ext & 0xff);
					int32_t xval;
					if (xreg < 8) {
						xval = isLong ? (int32_t)_state.D[xreg] : (int16_t)(uint16_t)_state.D[xreg];
					} else {
						xval = isLong ? (int32_t)_state.A[xreg - 8] : (int16_t)(uint16_t)_state.A[xreg - 8];
					}
					return pc + disp + xval;
				}
				case 4: // #imm - immediate
					return 0; // Handled specially
				default:
					return 0;
			}
		default:
			return 0;
	}
}

uint32_t GenesisM68k::ReadEa(uint8_t mode, uint8_t reg, uint8_t size) {
	uint32_t mask = SizeMask(size);
	switch (mode) {
		case 0: // Dn
			return _state.D[reg] & mask;
		case 1: // An
			return _state.A[reg] & mask;
		case 7:
			if (reg == 4) { // #imm
				if (size == 2) return FetchLong();
				return FetchWord() & mask;
			}
			[[fallthrough]];
		default:
		{
			uint32_t addr = GetEffectiveAddress(mode, reg, size);
			if (size == 0) return Read8(addr);
			if (size == 1) return Read16(addr);
			return Read32(addr);
		}
	}
}

void GenesisM68k::WriteEa(uint8_t mode, uint8_t reg, uint8_t size, uint32_t value) {
	uint32_t mask = SizeMask(size);
	switch (mode) {
		case 0: // Dn
			_state.D[reg] = (_state.D[reg] & ~mask) | (value & mask);
			return;
		case 1: // An
			if (size == 1) {
				_state.A[reg] = (int16_t)(uint16_t)(value & 0xffff); // Sign-extend word to long
			} else {
				_state.A[reg] = value;
			}
			return;
		default:
		{
			uint32_t addr = GetEffectiveAddress(mode, reg, size);
			if (size == 0) Write8(addr, (uint8_t)value);
			else if (size == 1) Write16(addr, (uint16_t)value);
			else Write32(addr, value);
			return;
		}
	}
}

// ===== Exception handling =====

void GenesisM68k::RaiseException(uint8_t vector) {
	// Switch to supervisor mode, push PC and SR
	uint16_t oldSR = _state.SR;
	uint32_t faultPc = _state.PC;
	_state.SR |= M68kFlags::Supervisor;
	_state.SR &= ~M68kFlags::Trace;

	// Swap stacks if needed
	if (!(oldSR & M68kFlags::Supervisor)) {
		_state.USP = _state.A[7];
		_state.A[7] = _state.SSP;
	}

	Push32(_state.PC);
	Push16(oldSR);

	// Read vector
	uint32_t vectorAddr = Read32(vector * 4);
	static uint64_t exceptionCount = 0;
	exceptionCount++;
	if (exceptionCount <= 128 || vector <= 16 || vectorAddr == 0 || (exceptionCount % 2048) == 0) {
		MessageManager::Log(std::format("[Genesis][M68K] Exception #{} vector={} pc=${:06x} newpc=${:06x} oldsr=${:04x}", exceptionCount, vector, faultPc & 0xffffff, vectorAddr & 0xffffff, oldSR));
	}
	_state.PC = vectorAddr;
	_state.Stopped = false;
	AddCycles(34);
}

void GenesisM68k::ProcessInterrupt(uint8_t level) {
	uint16_t oldSR = _state.SR;
	uint32_t oldPc = _state.PC;
	uint16_t vdpStatusBeforeAck = 0;
	if (_console && _console->GetVdp()) {
		vdpStatusBeforeAck = _console->GetVdp()->GetState().StatusRegister;
	}
	_state.SR = (_state.SR & ~M68kFlags::IntMask) | ((uint16_t)level << 8);
	_state.SR |= M68kFlags::Supervisor;
	_state.SR &= ~M68kFlags::Trace;

	if (!(oldSR & M68kFlags::Supervisor)) {
		_state.USP = _state.A[7];
		_state.A[7] = _state.SSP;
	}

	Push32(_state.PC);
	Push16(oldSR);

	// Genesis interrupt vectors: level 2=external, 4=HBlank, 6=VBlank
	uint8_t vector = 24 + level;
	_state.PC = Read32(vector * 4);
	if (_console && _console->GetVdp()) {
		_console->GetVdp()->AcknowledgeInterrupt(level);
	}
	uint16_t vdpStatusAfterAck = vdpStatusBeforeAck;
	if (_console && _console->GetVdp()) {
		vdpStatusAfterAck = _console->GetVdp()->GetState().StatusRegister;
	}
	static uint64_t irqCount = 0;
	irqCount++;
	if (irqCount <= 128 || (irqCount % 4096) == 0) {
		MessageManager::Log(std::format("[Genesis][M68K] IRQ #{} level={} vector={} pc=${:06x}->${:06x} sr=${:04x} vdpBefore=${:04x} vdpAfter=${:04x} pendingAfter={}",
			irqCount,
			level,
			vector,
			oldPc & 0xffffff,
			_state.PC & 0xffffff,
			oldSR,
			vdpStatusBeforeAck,
			vdpStatusAfterAck,
			(vdpStatusAfterAck & VdpStatus::VIntPending) ? 1 : 0));
	}
	_emu->ProcessInterrupt<CpuType::Genesis>(oldPc, _state.PC, false);
	_state.Stopped = false;
	AddCycles(44);
}

void GenesisM68k::SetInterrupt(uint8_t level) {
	// Latch and process at the next instruction boundary inside Exec().
	uint8_t previousPending = _pendingInterruptLevel;
	if (level > _pendingInterruptLevel) {
		_pendingInterruptLevel = level;
	}

	static uint64_t irqLatchCount = 0;
	irqLatchCount++;
	if (irqLatchCount <= 256 || (irqLatchCount % 4096) == 0) {
		uint16_t vdpStatus = 0;
		if (_console && _console->GetVdp()) {
			vdpStatus = _console->GetVdp()->GetState().StatusRegister;
		}
		MessageManager::Log(std::format("[Genesis][M68K] SetInterrupt #{} reqLevel={} prevPending={} newPending={} intMask={} pc=${:06x} sr=${:04x} vdp=${:04x}",
			irqLatchCount,
			level,
			previousPending,
			_pendingInterruptLevel,
			GetIntMask(),
			_state.PC & 0xffffff,
			_state.SR,
			vdpStatus));
	}
}

// ===== Main execution =====

void GenesisM68k::Exec() {
	_execCallCount++;
	if (!_memoryManager) {
		_dispatchFaultCount++;
		_lastDispatchFaultSummary = "missing_memory_manager";
		_lastDispatchBoundarySummary = "missing_memory_manager";
		return;
	}

	if (_pendingInterruptLevel > 0 && (_pendingInterruptLevel > GetIntMask() || _pendingInterruptLevel == 7)) {
		ProcessInterrupt(_pendingInterruptLevel);
		_pendingInterruptLevel = 0;
	}

	if (_state.Stopped) {
		AddCycles(4);
		return;
	}

#ifndef DUMMYCPU
	_emu->ProcessInstruction<CpuType::Genesis>();
#endif

	uint32_t rawPc = _state.PC;
	uint32_t prevPc = rawPc & 0x00ffffff;
	if (rawPc != prevPc) {
		_dispatchGuardHitCount++;
		_dispatchFaultCount++;
		_lastDispatchFaultSummary = std::format("pc_normalized raw=${:08x} normalized=${:06x}", rawPc, prevPc);
		_state.PC = prevPc;
	}
	if ((prevPc & 1) != 0) {
		_dispatchGuardHitCount++;
		_dispatchFaultCount++;
		uint32_t alignedPc = prevPc & 0xfffffe;
		_lastDispatchFaultSummary = std::format("odd_pc_alignment pc=${:06x} aligned=${:06x}", prevPc, alignedPc);
		prevPc = alignedPc;
		_state.PC = alignedPc;
		AddCycles(4);
	}

	if (_memoryManager) {
		_memoryManager->TraceCpuEarlyProbe(prevPc, _state);
	}

	uint16_t srBefore = _state.SR;
	uint32_t d0Before = _state.D[0];
	uint32_t a0Before = _state.A[0];
	uint32_t a7Before = _state.A[7];
	bool stoppedBefore = _state.Stopped;
	uint64_t cyclesBefore = _state.CycleCount;
	_lastFetchProgramCounter = prevPc;
	_lastFetchPreviewWordA = PeekWord(prevPc);
	_lastFetchPreviewWordB = PeekWord(prevPc + 2);
	_lastDispatchBoundarySummary = std::format("exec={} pc=${:06x} sr=${:04x} preview=${:04x}:${:04x} pendingIrq={} mask={} guardHits={}",
		_execCallCount,
		prevPc,
		srBefore,
		_lastFetchPreviewWordA,
		_lastFetchPreviewWordB,
		_pendingInterruptLevel,
		GetIntMask(),
		_dispatchGuardHitCount);
	uint16_t opcode = FetchOpcode();
	_lastFetchOpcode = opcode;
	uint16_t operandWordA = 0;
	uint16_t operandWordB = 0;
	if (_instructionTraceEnabled) {
		operandWordA = PeekWord(_state.PC);
		operandWordB = PeekWord(_state.PC + 2);
	}
	MaybeLogInstructionFlow(prevPc, opcode, operandWordA, operandWordB, cyclesBefore, cyclesBefore, srBefore, _state.SR);

	if ((opcode == 0x0000 && _lastFetchPreviewWordA == 0x0000 && _lastFetchPreviewWordB == 0x0000)
		|| (opcode == 0xffff && _lastFetchPreviewWordA == 0xffff && _lastFetchPreviewWordB == 0xffff)) {
		_decodeFaultCount++;
		if (_decodeFaultCount <= 128 || (_decodeFaultCount % 2048) == 0) {
			MessageManager::Log(std::format("[Genesis][M68K] DecodeBoundaryFault #{} pc=${:06x} op=${:04x} preview=${:04x}:${:04x} sr=${:04x}",
				_decodeFaultCount,
				prevPc,
				opcode,
				_lastFetchPreviewWordA,
				_lastFetchPreviewWordB,
				srBefore));
		}
	}

	ExecuteInstruction(opcode);
	uint64_t cyclesAfter = _state.CycleCount;
	bool forcedCycleFloorApplied = false;
	if (_memoryManager) {
		_memoryManager->UpdateExecutionHeartbeat(prevPc, _state.CycleCount);
	}

	if (cyclesAfter == cyclesBefore) {
		AddCycles(4);
		cyclesAfter = _state.CycleCount;
		_forcedCycleFloorCount++;
		forcedCycleFloorApplied = true;
		static uint64_t zeroCycleInstructionCount = 0;
		zeroCycleInstructionCount++;
		if (zeroCycleInstructionCount <= 512 || (zeroCycleInstructionCount % 8192) == 0) {
			MessageManager::Log(std::format("[Genesis][M68K] Zero-cycle instruction #{} pc=${:06x} opcode=${:04x} sr=${:04x} stopped={}",
				zeroCycleInstructionCount,
				prevPc,
				opcode,
				_state.SR,
				_state.Stopped ? 1 : 0));
		}
	}

	_lastDispatchBoundarySummary = std::format("exec={} pc=${:06x}->${:06x} op=${:04x} sr=${:04x}->${:04x} preview=${:04x}:${:04x} delta={} guards={} decodeFaults={} dispatchFaults={}",
		_execCallCount,
		prevPc,
		_state.PC & 0x00ffffff,
		opcode,
		srBefore,
		_state.SR,
		_lastFetchPreviewWordA,
		_lastFetchPreviewWordB,
		cyclesAfter > cyclesBefore ? (uint32_t)(cyclesAfter - cyclesBefore) : 0,
		_dispatchGuardHitCount,
		_decodeFaultCount,
		_dispatchFaultCount);
	MaybeLogInstructionFlow(prevPc, opcode, operandWordA, operandWordB, cyclesBefore, cyclesAfter, srBefore, _state.SR);

	if (_state.PC == prevPc && opcode == _lastRunOpcode) {
		_samePcRunLength++;
	} else {
		_samePcRunLength = 0;
	}
	_lastRunPc = prevPc;
	_lastRunOpcode = opcode;

	if (_samePcRunLength >= 200000) {
		ForceClockAdvance(32);
		_samePcRunLength = 0;
	}

	if (_instructionTraceEnabled) {
		RecordInstructionTrace(
			prevPc,
			_state.PC & 0x00ffffff,
			opcode,
			operandWordA,
			operandWordB,
			srBefore,
			_state.SR,
			cyclesBefore,
			cyclesAfter,
			d0Before,
			_state.D[0],
			a0Before,
			_state.A[0],
			a7Before,
			_state.A[7],
			forcedCycleFloorApplied,
			stoppedBefore,
			_state.Stopped);
	}

	if (!_firstDispatchCaptured) {
		_firstDispatchCaptured = true;
		_firstDispatchEntry = {};
		_firstDispatchEntry.Sequence = _instructionTraceSequence;
		_firstDispatchEntry.ProgramCounterBefore = prevPc;
		_firstDispatchEntry.ProgramCounterAfter = _state.PC & 0x00ffffff;
		_firstDispatchEntry.Opcode = opcode;
		_firstDispatchEntry.OperandWordA = operandWordA;
		_firstDispatchEntry.OperandWordB = operandWordB;
		_firstDispatchEntry.StatusRegisterBefore = srBefore;
		_firstDispatchEntry.StatusRegisterAfter = _state.SR;
		_firstDispatchEntry.CycleCountBefore = cyclesBefore;
		_firstDispatchEntry.CycleCountAfter = cyclesAfter;
		_firstDispatchEntry.InstructionCycleDelta = cyclesAfter > cyclesBefore ? (uint32_t)(cyclesAfter - cyclesBefore) : 0;
	}

	uint32_t nextPc = _state.PC & 0x00ffffff;
	if (prevPc != 0 && nextPc == 0) {
		static uint64_t pcZeroTransitionCount = 0;
		pcZeroTransitionCount++;
		if (pcZeroTransitionCount <= 256 || (pcZeroTransitionCount % 4096) == 0) {
			MessageManager::Log(std::format("[Genesis][M68K] PC->0 transition #{} prevPc=${:06x} opcode=${:04x} sr=${:04x} cycles={} stopped={}",
				pcZeroTransitionCount,
				prevPc,
				opcode,
				_state.SR,
				_state.CycleCount,
				_state.Stopped ? 1 : 0));
		}
	}
}

uint16_t GenesisM68k::PeekWord(uint32_t addr) const {
	if (!_memoryManager) {
		return 0;
	}
	uint8_t hi = _memoryManager->Peek8ForTrace(addr & 0x00ffffff);
	uint8_t lo = _memoryManager->Peek8ForTrace((addr + 1) & 0x00ffffff);
	return (uint16_t)((hi << 8) | lo);
}

void GenesisM68k::LoadInstructionFlowLogConfig() {
	if (_instructionFlowConfigLoaded) {
		return;
	}

	_instructionFlowConfigLoaded = true;
	const char* enabledRaw = getenv("NEXEN_GENESIS_TRACE_EXEC_FLOW");
	if (enabledRaw && (*enabledRaw == '1' || *enabledRaw == 'y' || *enabledRaw == 'Y' || *enabledRaw == 't' || *enabledRaw == 'T')) {
		_instructionFlowLogEnabled = true;
	}

	uint32_t limit = 0;
	if (TryReadEnvU32("NEXEN_GENESIS_TRACE_EXEC_LIMIT", 64, 5000000, limit)) {
		_instructionFlowLogLimit = limit;
	}

	uint32_t stride = 0;
	if (TryReadEnvU32("NEXEN_GENESIS_TRACE_EXEC_STRIDE", 1, 1000000, stride)) {
		_instructionFlowLogStride = stride;
	}

	uint32_t ringCapacity = 0;
	if (TryReadEnvU32("NEXEN_GENESIS_TRACE_EXEC_RING", 8, 1024, ringCapacity)) {
		_recentInstructionFlowCapacity = ringCapacity;
	}

	_recentInstructionFlowLogs.clear();
	_recentInstructionFlowLogs.reserve(_recentInstructionFlowCapacity);
}

void GenesisM68k::MaybeLogInstructionFlow(uint32_t prePc, uint16_t opcode, uint16_t operandWordA, uint16_t operandWordB, uint64_t cyclesBefore, uint64_t cyclesAfter, uint16_t srBefore, uint16_t srAfter) {
	LoadInstructionFlowLogConfig();
	if (!_instructionFlowLogEnabled) {
		return;
	}

	if (_instructionFlowLogCount >= _instructionFlowLogLimit) {
		_instructionFlowLogSkipped++;
		return;
	}

	if ((_instructionFlowLogCount % _instructionFlowLogStride) != 0) {
		_instructionFlowLogSkipped++;
		_instructionFlowLogCount++;
		return;
	}

	auto readWord = [this](uint32_t addr) {
		return PeekWord(addr);
	};

	string disasm = DisassembleM68kLine(prePc, opcode, operandWordA, operandWordB, readWord);
	string line = std::format("[Genesis][M68K][FLOW] seq={} pc=${:06x} op=${:04x} a=${:04x} b=${:04x} sr=${:04x}->${:04x} cyc={}->{} delta={} route={} disasm={}",
		_instructionFlowLogCount,
		prePc,
		opcode,
		operandWordA,
		operandWordB,
		srBefore,
		srAfter,
		cyclesBefore,
		cyclesAfter,
		cyclesAfter > cyclesBefore ? (uint32_t)(cyclesAfter - cyclesBefore) : 0,
		_lastDecodeRouteSummary.empty() ? "none" : _lastDecodeRouteSummary,
		disasm);

	_lastInstructionFlowLogLine = line;
	MessageManager::Log(line);
	if (_recentInstructionFlowLogs.size() >= _recentInstructionFlowCapacity && !_recentInstructionFlowLogs.empty()) {
		_recentInstructionFlowLogs.erase(_recentInstructionFlowLogs.begin());
	}
	_recentInstructionFlowLogs.push_back(line);
	_instructionFlowLogCount++;
}

void GenesisM68k::ArmAggressiveFlowTrace(uint32_t limit, uint32_t stride, uint32_t ringCapacity) {
	LoadInstructionFlowLogConfig();
	_instructionFlowLogEnabled = true;
	_instructionFlowLogLimit = std::clamp<uint32_t>(limit, 256u, 5000000u);
	_instructionFlowLogStride = std::clamp<uint32_t>(stride, 1u, 1000000u);
	_recentInstructionFlowCapacity = std::clamp<uint32_t>(ringCapacity, 16u, 1024u);
	if (_recentInstructionFlowLogs.capacity() < _recentInstructionFlowCapacity) {
		_recentInstructionFlowLogs.reserve(_recentInstructionFlowCapacity);
	}
}

void GenesisM68k::RecordInstructionTrace(uint32_t programCounterBefore, uint32_t programCounterAfter, uint16_t opcode, uint16_t operandWordA, uint16_t operandWordB, uint16_t statusRegisterBefore, uint16_t statusRegisterAfter, uint64_t cycleCountBefore, uint64_t cycleCountAfter, uint32_t d0Before, uint32_t d0After, uint32_t a0Before, uint32_t a0After, uint32_t a7Before, uint32_t a7After, bool forcedCycleFloor, bool stoppedBefore, bool stoppedAfter) {
	if (!_instructionTraceEnabled || _instructionTraceCapacity == 0) {
		return;
	}

	if (_instructionTraceEntries.size() != _instructionTraceCapacity) {
		_instructionTraceEntries.assign(_instructionTraceCapacity, GenesisInstructionTraceEntry{});
		_instructionTraceWriteIndex = 0;
		_instructionTraceWrapped = false;
	}

	GenesisInstructionTraceEntry entry = {};
	entry.Sequence = ++_instructionTraceSequence;
	entry.ProgramCounterBefore = programCounterBefore & 0x00ffffff;
	entry.ProgramCounterAfter = programCounterAfter & 0x00ffffff;
	entry.Opcode = opcode;
	entry.OperandWordA = operandWordA;
	entry.OperandWordB = operandWordB;
	entry.StatusRegisterBefore = statusRegisterBefore;
	entry.StatusRegisterAfter = statusRegisterAfter;
	entry.CycleCountBefore = cycleCountBefore;
	entry.CycleCountAfter = cycleCountAfter;
	entry.D0Before = d0Before;
	entry.D0After = d0After;
	entry.A0Before = a0Before;
	entry.A0After = a0After;
	entry.A7Before = a7Before;
	entry.A7After = a7After;
	entry.InstructionCycleDelta = cycleCountAfter > cycleCountBefore ? (uint32_t)(cycleCountAfter - cycleCountBefore) : 0;
	entry.ForcedCycleFloor = forcedCycleFloor ? 1 : 0;
	entry.InterruptLatched = _pendingInterruptLevel;
	entry.StoppedBefore = stoppedBefore ? 1 : 0;
	entry.StoppedAfter = stoppedAfter ? 1 : 0;

	_instructionTraceEntries[_instructionTraceWriteIndex] = entry;
	_instructionTraceWriteIndex++;
	if (_instructionTraceWriteIndex >= _instructionTraceCapacity) {
		_instructionTraceWriteIndex = 0;
		_instructionTraceWrapped = true;
	}
}

void GenesisM68k::SetInstructionTraceCapacity(uint32_t capacity) {
	_instructionTraceCapacity = std::max<uint32_t>(capacity, 256);
	_instructionTraceEntries.assign(_instructionTraceCapacity, GenesisInstructionTraceEntry{});
	_instructionTraceWriteIndex = 0;
	_instructionTraceWrapped = false;
	_instructionTraceSequence = 0;
}

void GenesisM68k::ClearInstructionTrace() {
	_instructionTraceEntries.clear();
	_instructionTraceWriteIndex = 0;
	_instructionTraceWrapped = false;
	_instructionTraceSequence = 0;
}

vector<GenesisInstructionTraceEntry> GenesisM68k::GetInstructionTraceSnapshot() const {
	vector<GenesisInstructionTraceEntry> snapshot = {};
	if (_instructionTraceEntries.empty() || _instructionTraceCapacity == 0) {
		return snapshot;
	}

	uint32_t start = _instructionTraceWrapped ? _instructionTraceWriteIndex : 0;
	uint32_t count = _instructionTraceWrapped ? _instructionTraceCapacity : _instructionTraceWriteIndex;
	snapshot.reserve(count);
	for (uint32_t i = 0; i < count; i++) {
		snapshot.push_back(_instructionTraceEntries[(start + i) % _instructionTraceCapacity]);
	}
	return snapshot;
}

string GenesisM68k::BuildInstructionTraceDigest() const {
	auto snapshot = GetInstructionTraceSnapshot();
	uint64_t hash = 1469598103934665603ull;
	for (const GenesisInstructionTraceEntry& entry : snapshot) {
		hash ^= entry.Sequence;
		hash *= 1099511628211ull;
		hash ^= ((uint64_t)entry.ProgramCounterBefore << 32) | entry.ProgramCounterAfter;
		hash *= 1099511628211ull;
		hash ^= ((uint64_t)entry.Opcode << 48) | ((uint64_t)entry.OperandWordA << 24) | entry.OperandWordB;
		hash *= 1099511628211ull;
		hash ^= ((uint64_t)entry.StatusRegisterBefore << 16) | entry.StatusRegisterAfter;
		hash *= 1099511628211ull;
		hash ^= ((uint64_t)entry.ForcedCycleFloor << 8) | entry.InterruptLatched;
		hash *= 1099511628211ull;
		hash ^= entry.InstructionCycleDelta;
		hash *= 1099511628211ull;
	}
	return std::format("{:016x}", hash);
}

string GenesisM68k::BuildInstructionTraceWindow(uint32_t maxLines) const {
	auto snapshot = GetInstructionTraceSnapshot();
	if (snapshot.empty()) {
		return "trace-window=empty";
	}

	uint32_t clampedLines = std::clamp<uint32_t>(maxLines, 1u, 32u);
	uint32_t startIndex = 0;
	if (snapshot.size() > clampedLines) {
		startIndex = (uint32_t)snapshot.size() - clampedLines;
	}

	string window = std::format("trace-window count={} lines={}", snapshot.size(), snapshot.size() - startIndex);
	for (uint32_t i = startIndex; i < (uint32_t)snapshot.size(); i++) {
		const GenesisInstructionTraceEntry& entry = snapshot[i];
		window += std::format(" | #{:05} pc=${:06x} op=${:04x} a=${:04x} b=${:04x} next=${:06x} sr=${:04x}->{:04x} dcy={} d0=${:08x}->{:08x} a7=${:08x}->{:08x}",
			entry.Sequence,
			entry.ProgramCounterBefore,
			entry.Opcode,
			entry.OperandWordA,
			entry.OperandWordB,
			entry.ProgramCounterAfter,
			entry.StatusRegisterBefore,
			entry.StatusRegisterAfter,
			entry.InstructionCycleDelta,
			entry.D0Before,
			entry.D0After,
			entry.A7Before,
			entry.A7After);
	}

	return window;
}

string GenesisM68k::BuildExecutionStallSummary() const {
	auto snapshot = GetInstructionTraceSnapshot();
	if (snapshot.empty()) {
		return "trace=empty";
	}

	const GenesisInstructionTraceEntry& lastEntry = snapshot.back();
	uint32_t forcedEntries = 0;
	for (const GenesisInstructionTraceEntry& entry : snapshot) {
		forcedEntries += entry.ForcedCycleFloor != 0 ? 1u : 0u;
	}

	string summary = std::format(
		"traceCount={} digest={} forcedCycleFloors={} forcedClockAdvances={} lastPc=${:06x} lastOpcode=${:04x} lastDelta={} lastSr=${:04x}",
		snapshot.size(),
		BuildInstructionTraceDigest(),
		forcedEntries,
		_forcedClockAdvanceCount,
		lastEntry.ProgramCounterBefore,
		lastEntry.Opcode,
		lastEntry.InstructionCycleDelta,
		lastEntry.StatusRegisterAfter);

	if (snapshot.size() >= 4) {
		uint64_t loopHash = 1469598103934665603ull;
		for (size_t i = snapshot.size() - 4; i < snapshot.size(); i++) {
			const GenesisInstructionTraceEntry& entry = snapshot[i];
			string row = std::format("{:06x}:{:04x}:{:06x}:{}", entry.ProgramCounterBefore, entry.Opcode, entry.ProgramCounterAfter, entry.InstructionCycleDelta);
			loopHash = HashTraceText(loopHash, row);
		}
		summary += std::format(" loopSig={:016x}", loopHash);
	}

	return summary;
}

string GenesisM68k::BuildAddressErrorSummary() const {
	if (_addressErrorCount == 0) {
		return "address-errors=0";
	}

	return std::format("address-errors={} lastSrc={} lastPc=${:06x} lastAddr=${:06x} lastSize={} lastWrite={}",
		_addressErrorCount,
		_lastAddressErrorSource.empty() ? "unknown" : _lastAddressErrorSource,
		_lastAddressErrorPc,
		_lastAddressErrorAddr,
		_lastAddressErrorSize,
		_lastAddressErrorWrite ? 1 : 0);
}

string GenesisM68k::BuildCrashProbeSummary() const {
	string firstDispatchSummary = "none";
	if (_firstDispatchCaptured) {
		firstDispatchSummary = std::format(
			"pc=${:06x} opcode=${:04x} next=${:06x} delta={} sr=${:04x}",
			_firstDispatchEntry.ProgramCounterBefore,
			_firstDispatchEntry.Opcode,
			_firstDispatchEntry.ProgramCounterAfter,
			_firstDispatchEntry.InstructionCycleDelta,
			_firstDispatchEntry.StatusRegisterAfter);
	}

	return std::format(
		"resetCount={} vectorSp=${:08x} vectorPc=${:06x} firstDispatch={} dispatchFaults={} lastDispatchFault={} guardHits={} decodeFaults={} lastFetchPc=${:06x} lastOpcode=${:04x} decodeRoute={} boundary={} flow={} forcedCycleFloors={} forcedClockAdvances={} {}",
		_resetProbeCount,
		_lastResetVectorSp,
		_lastResetVectorPc,
		firstDispatchSummary,
		_dispatchFaultCount,
		_lastDispatchFaultSummary.empty() ? "none" : _lastDispatchFaultSummary,
		_dispatchGuardHitCount,
		_decodeFaultCount,
		_lastFetchProgramCounter,
		_lastFetchOpcode,
		_lastDecodeRouteSummary.empty() ? "none" : _lastDecodeRouteSummary,
		_lastDispatchBoundarySummary.empty() ? "none" : _lastDispatchBoundarySummary,
		BuildInstructionFlowSummary(),
		_forcedCycleFloorCount,
		_forcedClockAdvanceCount,
		BuildAddressErrorSummary());
}

string GenesisM68k::BuildDispatchBoundaryProbeSummary() const {
	return std::format(
		"execCalls={} guardHits={} decodeFaults={} dispatchFaults={} fetchPc=${:06x} fetchOpcode=${:04x} decodeGroup={} decodeSubOp={} decodeMode={} decodeReg={} decodeRoute={} preview=${:04x}:{:04x} boundary={} flow={}",
		_execCallCount,
		_dispatchGuardHitCount,
		_decodeFaultCount,
		_dispatchFaultCount,
		_lastFetchProgramCounter,
		_lastFetchOpcode,
		_lastDecodedGroup,
		_lastDecodedSubOp,
		_lastDecodedMode,
		_lastDecodedReg,
		_lastDecodeRouteSummary.empty() ? "none" : _lastDecodeRouteSummary,
		_lastFetchPreviewWordA,
		_lastFetchPreviewWordB,
		_lastDispatchBoundarySummary.empty() ? "none" : _lastDispatchBoundarySummary,
		BuildInstructionFlowSummary());
}

string GenesisM68k::BuildInstructionFlowSummary() const {
	string lastLine = _lastInstructionFlowLogLine.empty() ? "none" : _lastInstructionFlowLogLine;
	if (lastLine.size() > 240) {
		lastLine = lastLine.substr(0, 240);
	}

	return std::format("enabled={} logged={} skipped={} stride={} limit={} ring={} last={}",
		_instructionFlowLogEnabled ? 1 : 0,
		_instructionFlowLogCount,
		_instructionFlowLogSkipped,
		_instructionFlowLogStride,
		_instructionFlowLogLimit,
		_recentInstructionFlowLogs.size(),
		lastLine);
}

void GenesisM68k::ForceClockAdvance(uint32_t cycles) {
	if (!_memoryManager || cycles == 0) {
		return;
	}
	_memoryManager->Exec(cycles);
	_state.CycleCount += cycles;
	_forcedClockAdvanceCount++;
}

void GenesisM68k::Reset(bool softReset) {
	static uint64_t resetCount = 0;
	resetCount++;
	_resetProbeCount++;
	if (!softReset) {
		memset(&_state, 0, sizeof(_state));
		_firstDispatchCaptured = false;
		_firstDispatchEntry = {};
		_dispatchFaultCount = 0;
		_lastDispatchFaultSummary.clear();
		_addressErrorCount = 0;
		_lastAddressErrorAddr = 0;
		_lastAddressErrorPc = 0;
		_lastAddressErrorSize = 0;
		_lastAddressErrorWrite = false;
		_lastAddressErrorSource.clear();
	}

	_state.SR = 0x2700; // Supervisor mode, all interrupts masked
	_state.Stopped = false;
	_pendingInterruptLevel = 0;

	// Read initial SSP and PC from vector table
	_state.SSP = Read32(0x000000);
	_state.A[7] = _state.SSP;
	_state.PC = Read32(0x000004);
	_lastResetVectorSp = _state.SSP;
	_lastResetVectorPc = _state.PC & 0x00ffffff;
	if ((_state.PC & 0x00ffffff) == 0) {
		_dispatchFaultCount++;
		_lastDispatchFaultSummary = "reset_vector_pc_zero";
	}
	if (resetCount <= 8 || (resetCount % 256) == 0) {
		MessageManager::Log(std::format("[Genesis][M68K] Reset #{} soft={} ssp=${:08x} pc=${:06x}", resetCount, softReset ? 1 : 0, _state.SSP, _state.PC));
		MessageManager::Log(std::format("[Genesis][M68K] ResetCrashProbe {}", BuildCrashProbeSummary()));
	}
}

// ===== Instruction decoder =====
// Decodes the 68000 instruction set by examining opcode bit fields

void GenesisM68k::ExecuteInstruction(uint16_t opcode) {
	uint8_t group = (opcode >> 12) & 0x0f;
	uint8_t subOp = (opcode >> 9) & 0x07;
	uint8_t mode = (opcode >> 3) & 0x07;
	uint8_t reg = opcode & 0x07;
	_lastDecodedGroup = group;
	_lastDecodedSubOp = subOp;
	_lastDecodedMode = mode;
	_lastDecodedReg = reg;
	_decodeGroupHitCount[group]++;
	_lastDecodeRouteSummary = std::format("route=entry g={} sub={} mode={} reg={} hits={}", group, subOp, mode, reg, _decodeGroupHitCount[group]);
	auto setRoute = [&](const char* routeTag) {
		_lastDecodeRouteSummary = std::format("route={} g={} sub={} mode={} reg={} hits={}", routeTag, group, subOp, mode, reg, _decodeGroupHitCount[group]);
	};

	switch (group) {
		case 0x0: // Bit manipulation / MOVEP / Immediate
		{
			if ((opcode & 0x0100) && !(opcode & 0x0038)) {
				setRoute("g0-dynbit-dn");
				// Dynamic bit operations on Dn
				uint8_t bitOp = (opcode >> 6) & 3;
				switch (bitOp) {
					case 0: Op_BTST(opcode); break;
					case 1: Op_BCHG(opcode); break;
					case 2: Op_BCLR(opcode); break;
					case 3: Op_BSET(opcode); break;
				}
			} else if (opcode & 0x0100) {
				setRoute("g0-dynbit-mem");
				// Dynamic bit ops on memory
				uint8_t bitOp = (opcode >> 6) & 3;
				switch (bitOp) {
					case 0: Op_BTST(opcode); break;
					case 1: Op_BCHG(opcode); break;
					case 2: Op_BCLR(opcode); break;
					case 3: Op_BSET(opcode); break;
				}
			} else {
				setRoute("g0-imm-subswitch");
				uint8_t subOp = (opcode >> 9) & 7;
				switch (subOp) {
					case 0: // ORI
						if ((opcode & 0x3f) == 0x3c) Op_ORI_SR(opcode);
						else Op_ORI(opcode);
						break;
					case 1: // ANDI
						if ((opcode & 0x3f) == 0x3c) Op_ANDI_SR(opcode);
						else Op_ANDI(opcode);
						break;
					case 2: Op_SUBI(opcode); break;
					case 3: Op_ADDI(opcode); break;
					case 4: // Static bit operations
					{
						uint8_t bitOp = (opcode >> 6) & 3;
						switch (bitOp) {
							case 0: Op_BTST(opcode); break;
							case 1: Op_BCHG(opcode); break;
							case 2: Op_BCLR(opcode); break;
							case 3: Op_BSET(opcode); break;
						}
						break;
					}
					case 5: // EORI
						if ((opcode & 0x3f) == 0x3c) Op_EORI_SR(opcode);
						else Op_EORI(opcode);
						break;
					case 6: Op_CMPI(opcode); break;
					default: Op_ILLEGAL(opcode); break;
				}
			}
			break;
		}

		case 0x1: // MOVE.B
		case 0x2: // MOVE.L
		case 0x3: // MOVE.W
		{
			setRoute("g1to3-move");
			uint8_t dstMode = (opcode >> 6) & 7;
			if (dstMode == 1 && group != 1) {
				Op_MOVEA(opcode);
			} else {
				Op_MOVE(opcode);
			}
			break;
		}

		case 0x4: // Miscellaneous
		{
			setRoute("g4-misc");
			if ((opcode & 0xffc0) == 0x46c0) { Op_MOVE_SR(opcode); break; } // MOVE to SR
			if ((opcode & 0xffc0) == 0x44c0) { Op_MOVE_SR(opcode); break; } // MOVE to CCR
			if ((opcode & 0xffc0) == 0x40c0) { Op_MOVE_SR(opcode); break; } // MOVE from SR
			if ((opcode & 0xfff0) == 0x4e60) { Op_MOVE_USP(opcode); break; }
			if ((opcode & 0xfff8) == 0x4e50) { Op_LINK(opcode); break; }
			if ((opcode & 0xfff8) == 0x4e58) { Op_UNLK(opcode); break; }
			if ((opcode & 0xfff8) == 0x4840) { Op_SWAP(opcode); break; }
			if ((opcode & 0xfff8) == 0x4880) { Op_EXT(opcode); break; }  // EXT.W
			if ((opcode & 0xfff8) == 0x48c0) { Op_EXT(opcode); break; }  // EXT.L
			if (opcode == 0x4e75) { Op_RTS(opcode); break; }
			if (opcode == 0x4e73) { Op_RTE(opcode); break; }
			if (opcode == 0x4e77) { Op_RTR(opcode); break; }
			if (opcode == 0x4e71) { Op_NOP(opcode); break; }
			if (opcode == 0x4e70) { Op_RESET(opcode); break; }
			if (opcode == 0x4e72) { Op_STOP(opcode); break; }
			if ((opcode & 0xfff0) == 0x4e40) { Op_TRAP(opcode); break; }
			if ((opcode & 0xffc0) == 0x4ec0) { Op_JMP(opcode); break; }
			if ((opcode & 0xffc0) == 0x4e80) { Op_JSR(opcode); break; }
			if ((opcode & 0xffc0) == 0x4800) { // NBCD - not implementing now
				Op_ILLEGAL(opcode); break;
			}
			if ((opcode & 0xfb80) == 0x4880) { Op_MOVEM(opcode); break; }
			if ((opcode & 0xffc0) == 0x4ac0) { Op_TAS(opcode); break; }
			if ((opcode & 0xff00) == 0x4a00) { Op_TST(opcode); break; }

			uint8_t subOp = (opcode >> 6) & 3;
			switch (subOp) {
				case 0: case 1: case 2:
				{
					uint8_t op2 = (opcode >> 8) & 0x0f;
					if (op2 == 0x02 || op2 == 0x06 || op2 == 0x0a) {
						Op_CLR(opcode);
					} else if (op2 == 0x04 || op2 == 0x08 || op2 == 0x0c) {
						Op_NEG(opcode);
					} else if ((opcode & 0xffc0) == 0x4ac0) {
						Op_TAS(opcode);
					} else {
						Op_ILLEGAL(opcode);
					}
					break;
				}
				default:
					if ((opcode & 0xffc0) == 0x4ec0) Op_JMP(opcode);
					else if ((opcode & 0xffc0) == 0x4e80) Op_JSR(opcode);
					else if ((opcode & 0xffc0) == 0x41c0 || (opcode & 0xf1c0) == 0x41c0) Op_LEA(opcode);
					else Op_ILLEGAL(opcode);
					break;
			}
			break;
		}

		case 0x5: // ADDQ/SUBQ/Scc/DBcc
		{
			setRoute("g5-addq-subq-scc");
			uint8_t size = (opcode >> 6) & 3;
			if (size == 3) {
				// Scc / DBcc
				uint8_t mode = (opcode >> 3) & 7;
				if (mode == 1) Op_DBcc(opcode);
				else Op_Scc(opcode);
			} else {
				if (opcode & 0x0100) Op_SUBQ(opcode);
				else Op_ADDQ(opcode);
			}
			break;
		}

		case 0x6: // Bcc/BSR/BRA
		{
			setRoute("g6-branch");
			uint8_t cc = (opcode >> 8) & 0x0f;
			if (cc == 0) Op_BRA(opcode);
			else if (cc == 1) Op_BSR(opcode);
			else Op_Bcc(opcode);
			break;
		}

		case 0x7: // MOVEQ
			setRoute("g7-moveq");
			Op_MOVEQ(opcode);
			break;

		case 0x8: // OR/DIV
		{
			setRoute("g8-or-div");
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3) Op_DIVU(opcode);
			else if (opMode == 7) Op_DIVS(opcode);
			else Op_OR(opcode);
			break;
		}

		case 0x9: // SUB/SUBA
		{
			setRoute("g9-sub");
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3 || opMode == 7) Op_SUBA(opcode);
			else Op_SUB(opcode);
			break;
		}

		case 0xb: // CMP/CMPA/EOR
		{
			setRoute("gb-cmp-eor");
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3 || opMode == 7) Op_CMPA(opcode);
			else if (opMode >= 4 && opMode <= 6) Op_EOR(opcode);
			else Op_CMP(opcode);
			break;
		}

		case 0xc: // AND/MUL/EXG
		{
			setRoute("gc-and-mul-exg");
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3) Op_MULU(opcode);
			else if (opMode == 7) Op_MULS(opcode);
			else if ((opcode & 0xf130) == 0xc100) Op_EXG(opcode);
			else Op_AND(opcode);
			break;
		}

		case 0xd: // ADD/ADDA
		{
			setRoute("gd-add");
			uint8_t opMode = (opcode >> 6) & 7;
			if (opMode == 3 || opMode == 7) Op_ADDA(opcode);
			else Op_ADD(opcode);
			break;
		}

		case 0xe: // Shift/Rotate
		{
			setRoute("ge-shift-rotate");
			uint8_t type = (opcode >> 3) & 3;
			switch (type & 3) {
				case 0: Op_ASd(opcode); break;
				case 1: Op_LSd(opcode); break;
				case 2: Op_ROXd(opcode); break;
				case 3: Op_ROd(opcode); break;
			}
			break;
		}

		case 0xa: // Line-A exception
			setRoute("ga-linea-exception");
			_state.PC -= 2;
			RaiseException(10);
			break;

		case 0xf: // Line-F exception
			setRoute("gf-linef-exception");
			_state.PC -= 2;
			RaiseException(11);
			break;

		default:
			setRoute("g?-illegal");
			Op_ILLEGAL(opcode);
			break;
	}
}

// ===== Data Movement Instructions =====

void GenesisM68k::Op_MOVE(uint16_t opcode) {
	uint8_t size;
	switch ((opcode >> 12) & 3) {
		case 1: size = 0; break; // .B
		case 3: size = 1; break; // .W
		case 2: size = 2; break; // .L
		default: size = 1; break;
	}

	uint8_t srcMode = (opcode >> 3) & 7;
	uint8_t srcReg = opcode & 7;
	uint8_t dstMode = (opcode >> 6) & 7;
	uint8_t dstReg = (opcode >> 9) & 7;

	uint32_t data = ReadEa(srcMode, srcReg, size);
	SetFlags_Logical(data, size);
	WriteEa(dstMode, dstReg, size, data);
	AddCycles(4);
}

void GenesisM68k::Op_MOVEA(uint16_t opcode) {
	uint8_t size = ((opcode >> 12) & 3) == 3 ? 1 : 2;
	uint8_t srcMode = (opcode >> 3) & 7;
	uint8_t srcReg = opcode & 7;
	uint8_t dstReg = (opcode >> 9) & 7;

	uint32_t data = ReadEa(srcMode, srcReg, size);
	if (size == 1) data = (int16_t)(uint16_t)data; // Sign extend word
	_state.A[dstReg] = data;
	AddCycles(4);
}

void GenesisM68k::Op_MOVEQ(uint16_t opcode) {
	uint8_t dstReg = (opcode >> 9) & 7;
	int8_t data = (int8_t)(opcode & 0xff);
	_state.D[dstReg] = (int32_t)data; // Sign extend
	SetFlags_Logical(_state.D[dstReg], 2);
	AddCycles(4);
}

void GenesisM68k::Op_MOVEM(uint16_t opcode) {
	uint8_t size = (opcode & 0x0040) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint16_t regList = FetchWord();
	bool toMemory = !(opcode & 0x0400);

	if (toMemory) {
		if (mode == 4) { // Predecrement: register order is reversed
			for (int i = 15; i >= 0; i--) {
				if (regList & (1 << i)) {
					uint32_t val = (i < 8) ? _state.D[i] : _state.A[i - 8];
					if (size == 1) {
						_state.A[reg] -= 2;
						Write16(_state.A[reg], (uint16_t)val);
					} else {
						_state.A[reg] -= 4;
						Write32(_state.A[reg], val);
					}
				}
			}
		} else {
			uint32_t addr;
			if (mode == 3) {
				// (An)+ uses the current address and updates An after the full transfer.
				addr = _state.A[reg];
			} else {
				addr = GetEffectiveAddress(mode, reg, size);
			}
			for (int i = 0; i < 16; i++) {
				if (regList & (1 << i)) {
					uint32_t val = (i < 8) ? _state.D[i] : _state.A[i - 8];
					if (size == 1) {
						Write16(addr, (uint16_t)val);
						addr += 2;
					} else {
						Write32(addr, val);
						addr += 4;
					}
				}
			}
			if (mode == 3) {
				_state.A[reg] = addr;
			}
		}
	} else { // To registers
		uint32_t addr;
		if (mode == 3) { // Postincrement
			addr = _state.A[reg];
		} else {
			addr = GetEffectiveAddress(mode, reg, size);
		}
		for (int i = 0; i < 16; i++) {
			if (regList & (1 << i)) {
				if (i < 8) {
					_state.D[i] = (size == 1) ? (int16_t)(int32_t)Read16(addr) : Read32(addr);
				} else {
					_state.A[i - 8] = (size == 1) ? (int16_t)(int32_t)Read16(addr) : Read32(addr);
				}
				addr += (size == 1) ? 2 : 4;
			}
		}
		if (mode == 3) {
			_state.A[reg] = addr;
		}
	}
	AddCycles(4);
}

void GenesisM68k::Op_LEA(uint16_t opcode) {
	uint8_t dstReg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	_state.A[dstReg] = GetEffectiveAddress(mode, reg, 2);
	AddCycles(4);
}

void GenesisM68k::Op_PEA(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t addr = GetEffectiveAddress(mode, reg, 2);
	Push32(addr);
	AddCycles(4);
}

void GenesisM68k::Op_CLR(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	WriteEa(mode, reg, size, 0);
	_state.SR &= ~(M68kFlags::Carry | M68kFlags::Overflow | M68kFlags::Negative);
	_state.SR |= M68kFlags::Zero;
	AddCycles(4);
}

void GenesisM68k::Op_EXG(uint16_t opcode) {
	uint8_t rx = (opcode >> 9) & 7;
	uint8_t ry = opcode & 7;
	uint8_t opMode = (opcode >> 3) & 0x1f;

	if (opMode == 0x08) { // Data <-> Data
		std::swap(_state.D[rx], _state.D[ry]);
	} else if (opMode == 0x09) { // Address <-> Address
		std::swap(_state.A[rx], _state.A[ry]);
	} else if (opMode == 0x11) { // Data <-> Address
		std::swap(_state.D[rx], _state.A[ry]);
	}
	AddCycles(6);
}

void GenesisM68k::Op_SWAP(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	uint32_t val = _state.D[reg];
	_state.D[reg] = (val >> 16) | (val << 16);
	SetFlags_Logical(_state.D[reg], 2);
	AddCycles(4);
}

void GenesisM68k::Op_LINK(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	int16_t disp = (int16_t)FetchWord();
	Push32(_state.A[reg]);
	_state.A[reg] = SP();
	SP() += disp;
	AddCycles(16);
}

void GenesisM68k::Op_UNLK(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	SP() = _state.A[reg];
	_state.A[reg] = Pop32();
	AddCycles(12);
}

// ===== Arithmetic =====

void GenesisM68k::Op_ADD(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);

	uint32_t mask = SizeMask(size);
	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size);
		uint32_t dst = _state.D[reg] & mask;
		uint64_t result = (uint64_t)src + (uint64_t)dst;
		SetFlags_Add(src, dst, (uint32_t)result, size);
		_state.D[reg] = (_state.D[reg] & ~mask) | ((uint32_t)result & mask);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t src = _state.D[reg] & mask;
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint64_t result = (uint64_t)src + (uint64_t)dst;
		SetFlags_Add(src, dst, (uint32_t)result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, (uint32_t)result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_ADDA(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode & 0x0100) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int32_t src = (size == 1) ? (int16_t)(uint16_t)ReadEa(mode, eaReg, size) : (int32_t)ReadEa(mode, eaReg, size);
	_state.A[reg] += src;
	AddCycles(size == 2 ? 6 : 8);
}

void GenesisM68k::Op_ADDI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t dst;
	if (mode <= 1) {
		dst = ReadEa(mode, reg, size);
		uint64_t result = (uint64_t)(dst & mask) + (uint64_t)(imm & mask);
		SetFlags_Add(imm, dst, (uint32_t)result, size);
		WriteEa(mode, reg, size, (uint32_t)result);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, reg, size);
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint64_t result = (uint64_t)(dst & mask) + (uint64_t)(imm & mask);
		SetFlags_Add(imm, dst, (uint32_t)result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, (uint32_t)result);
	}
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_ADDQ(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t data = (opcode >> 9) & 7;
	if (data == 0) data = 8;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 1) { // Address register — no flags affected
		_state.A[reg] += data;
	} else {
		uint32_t mask = SizeMask(size);
		if (mode == 0) {
			uint32_t dst = ReadEa(mode, reg, size) & mask;
			uint64_t result = (uint64_t)dst + data;
			SetFlags_Add(data, dst, (uint32_t)result, size);
			WriteEa(mode, reg, size, (uint32_t)result);
		} else {
			uint32_t addr = GetEffectiveAddress(mode, reg, size);
			uint32_t dst;
			if (size == 0) dst = Read8(addr);
			else if (size == 1) dst = Read16(addr);
			else dst = Read32(addr);
			dst &= mask;
			uint64_t result = (uint64_t)dst + data;
			SetFlags_Add(data, dst, (uint32_t)result, size);
			if (size == 0) Write8(addr, (uint8_t)result);
			else if (size == 1) Write16(addr, (uint16_t)result);
			else Write32(addr, (uint32_t)result);
		}
	}
	AddCycles(4);
}

void GenesisM68k::Op_SUB(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);
	uint32_t mask = SizeMask(size);

	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size) & mask;
		uint32_t dst = _state.D[reg] & mask;
		uint32_t result = dst - src;
		SetFlags_Sub(src, dst, result, size);
		_state.D[reg] = (_state.D[reg] & ~mask) | (result & mask);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t src = _state.D[reg] & mask;
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst - src;
		SetFlags_Sub(src, dst, result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_SUBA(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode & 0x0100) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int32_t src = (size == 1) ? (int16_t)(uint16_t)ReadEa(mode, eaReg, size) : (int32_t)ReadEa(mode, eaReg, size);
	_state.A[reg] -= src;
	AddCycles(size == 2 ? 6 : 8);
}

void GenesisM68k::Op_SUBI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode <= 1) {
		uint32_t dst = ReadEa(mode, reg, size) & mask;
		uint32_t result = dst - imm;
		SetFlags_Sub(imm, dst, result, size);
		WriteEa(mode, reg, size, result);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, reg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		dst &= mask;
		uint32_t result = dst - imm;
		SetFlags_Sub(imm, dst, result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_SUBQ(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t data = (opcode >> 9) & 7;
	if (data == 0) data = 8;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 1) {
		_state.A[reg] -= data;
	} else {
		uint32_t mask = SizeMask(size);
		if (mode == 0) {
			uint32_t dst = ReadEa(mode, reg, size) & mask;
			uint32_t result = dst - data;
			SetFlags_Sub(data, dst, result, size);
			WriteEa(mode, reg, size, result);
		} else {
			uint32_t addr = GetEffectiveAddress(mode, reg, size);
			uint32_t dst;
			if (size == 0) dst = Read8(addr);
			else if (size == 1) dst = Read16(addr);
			else dst = Read32(addr);
			dst &= mask;
			uint32_t result = dst - data;
			SetFlags_Sub(data, dst, result, size);
			if (size == 0) Write8(addr, (uint8_t)result);
			else if (size == 1) Write16(addr, (uint16_t)result);
			else Write32(addr, result);
		}
	}
	AddCycles(4);
}

void GenesisM68k::Op_CMP(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	uint32_t mask = SizeMask(size);

	uint32_t src = ReadEa(mode, eaReg, size) & mask;
	uint32_t dst = _state.D[reg] & mask;
	uint32_t result = dst - src;
	SetFlags_Cmp(src, dst, result, size);
	AddCycles(4);
}

void GenesisM68k::Op_CMPA(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode & 0x0100) ? 2 : 1;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int32_t src = (size == 1) ? (int16_t)(uint16_t)ReadEa(mode, eaReg, size) : (int32_t)ReadEa(mode, eaReg, size);
	int32_t dst = (int32_t)_state.A[reg];
	uint32_t result = (uint32_t)(dst - src);
	SetFlags_Cmp((uint32_t)src, (uint32_t)dst, result, 2);
	AddCycles(6);
}

void GenesisM68k::Op_CMPI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = dst - imm;
	SetFlags_Cmp(imm, dst, result, size);
	AddCycles(size == 2 ? 12 : 8);
}

void GenesisM68k::Op_MULU(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	uint16_t src = (uint16_t)ReadEa(mode, eaReg, 1);
	uint16_t dst = (uint16_t)_state.D[reg];
	uint32_t result = (uint32_t)src * (uint32_t)dst;
	_state.D[reg] = result;
	SetFlags_Logical(result, 2);
	AddCycles(70); // Worst case
}

void GenesisM68k::Op_MULS(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int16_t src = (int16_t)(uint16_t)ReadEa(mode, eaReg, 1);
	int16_t dst = (int16_t)(uint16_t)_state.D[reg];
	int32_t result = (int32_t)src * (int32_t)dst;
	_state.D[reg] = (uint32_t)result;
	SetFlags_Logical((uint32_t)result, 2);
	AddCycles(70);
}

void GenesisM68k::Op_DIVU(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	uint16_t divisor = (uint16_t)ReadEa(mode, eaReg, 1);
	if (divisor == 0) {
		RaiseException(5); // Division by zero
		return;
	}
	uint32_t dividend = _state.D[reg];
	uint32_t quotient = dividend / divisor;
	uint16_t remainder = (uint16_t)(dividend % divisor);

	if (quotient > 0xffff) {
		SetCcr(M68kFlags::Overflow);
		ClearCcr(M68kFlags::Carry);
	} else {
		_state.D[reg] = ((uint32_t)remainder << 16) | (quotient & 0xffff);
		ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);
		SetCcrIf(M68kFlags::Zero, (quotient & 0xffff) == 0);
		SetCcrIf(M68kFlags::Negative, (quotient & 0x8000) != 0);
	}
	AddCycles(140); // Worst case
}

void GenesisM68k::Op_DIVS(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;

	int16_t divisor = (int16_t)(uint16_t)ReadEa(mode, eaReg, 1);
	if (divisor == 0) {
		RaiseException(5);
		return;
	}
	int32_t dividend = (int32_t)_state.D[reg];
	int32_t quotient = dividend / divisor;
	int16_t remainder = (int16_t)(dividend % divisor);

	if (quotient < -32768 || quotient > 32767) {
		SetCcr(M68kFlags::Overflow);
		ClearCcr(M68kFlags::Carry);
	} else {
		_state.D[reg] = ((uint32_t)(uint16_t)remainder << 16) | ((uint16_t)quotient & 0xffff);
		ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);
		SetCcrIf(M68kFlags::Zero, (quotient & 0xffff) == 0);
		SetCcrIf(M68kFlags::Negative, (quotient & 0x8000) != 0);
	}
	AddCycles(158);
}

void GenesisM68k::Op_NEG(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t mask = SizeMask(size);

	uint32_t dst = ReadEa(mode, reg, size) & mask;
	uint32_t result = (uint32_t)(-(int32_t)dst) & mask;
	SetFlags_Sub(dst, 0, result, size);
	WriteEa(mode, reg, size, result);
	AddCycles(4);
}

void GenesisM68k::Op_EXT(uint16_t opcode) {
	uint8_t reg = opcode & 7;
	if (opcode & 0x0040) { // EXT.L
		_state.D[reg] = (int16_t)(uint16_t)_state.D[reg];
		SetFlags_Logical(_state.D[reg], 2);
	} else { // EXT.W
		_state.D[reg] = (_state.D[reg] & 0xffff0000) | ((uint16_t)(int8_t)_state.D[reg]);
		SetFlags_Logical(_state.D[reg], 1);
	}
	AddCycles(4);
}

void GenesisM68k::Op_TST(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	if (size == 3) {
		Op_ILLEGAL(opcode);
		return;
	}
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t data = ReadEa(mode, reg, size);
	SetFlags_Logical(data, size);
	AddCycles(4);
}

void GenesisM68k::Op_TAS(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	// TAS operates on data-alterable destinations only.
	if (mode == 1 || (mode == 7 && reg >= 2)) {
		Op_ILLEGAL(opcode);
		return;
	}

	uint8_t data = 0;
	if (mode == 0) {
		data = (uint8_t)(_state.D[reg] & 0xff);
		SetFlags_Logical(data, 0);
		_state.D[reg] = (_state.D[reg] & 0xffffff00) | (uint8_t)(data | 0x80);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, reg, 0);
		data = Read8(addr);
		SetFlags_Logical(data, 0);
		Write8(addr, (uint8_t)(data | 0x80));
	}
	AddCycles(4);
}

// ===== Logic =====

void GenesisM68k::Op_AND(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);
	uint32_t mask = SizeMask(size);

	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size) & mask;
		uint32_t result = (_state.D[reg] & mask) & src;
		_state.D[reg] = (_state.D[reg] & ~mask) | result;
		SetFlags_Logical(result, size);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst & (_state.D[reg] & mask);
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_ANDI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode <= 1) {
		uint32_t dst = ReadEa(mode, reg, size) & mask;
		uint32_t result = dst & imm;
		SetFlags_Logical(result, size);
		WriteEa(mode, reg, size, result);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, reg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		dst &= mask;
		uint32_t result = dst & imm;
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(size == 2 ? 14 : 8);
}

void GenesisM68k::Op_OR(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	bool toReg = !((opcode >> 8) & 1);
	uint32_t mask = SizeMask(size);

	if (toReg) {
		uint32_t src = ReadEa(mode, eaReg, size) & mask;
		uint32_t result = (_state.D[reg] & mask) | src;
		_state.D[reg] = (_state.D[reg] & ~mask) | result;
		SetFlags_Logical(result, size);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst | (_state.D[reg] & mask);
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_ORI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode <= 1) {
		uint32_t dst = ReadEa(mode, reg, size) & mask;
		uint32_t result = dst | imm;
		SetFlags_Logical(result, size);
		WriteEa(mode, reg, size, result);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, reg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		dst &= mask;
		uint32_t result = dst | imm;
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_EOR(uint16_t opcode) {
	uint8_t reg = (opcode >> 9) & 7;
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t eaReg = opcode & 7;
	uint32_t mask = SizeMask(size);

	uint32_t src = _state.D[reg] & mask;
	if (mode == 0) {
		uint32_t result = (_state.D[eaReg] & mask) ^ src;
		_state.D[eaReg] = (_state.D[eaReg] & ~mask) | result;
		SetFlags_Logical(result, size);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, eaReg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		uint32_t result = dst ^ src;
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

void GenesisM68k::Op_EORI(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t imm = (size == 2) ? FetchLong() : (FetchWord() & mask);
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode <= 1) {
		uint32_t dst = ReadEa(mode, reg, size) & mask;
		uint32_t result = dst ^ imm;
		SetFlags_Logical(result, size);
		WriteEa(mode, reg, size, result);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, reg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		dst &= mask;
		uint32_t result = dst ^ imm;
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(size == 2 ? 16 : 8);
}

void GenesisM68k::Op_NOT(uint16_t opcode) {
	uint8_t size = (opcode >> 6) & 3;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t mask = SizeMask(size);

	if (mode <= 1) {
		uint32_t dst = ReadEa(mode, reg, size) & mask;
		uint32_t result = (~dst) & mask;
		SetFlags_Logical(result, size);
		WriteEa(mode, reg, size, result);
	} else {
		uint32_t addr = GetEffectiveAddress(mode, reg, size);
		uint32_t dst;
		if (size == 0) dst = Read8(addr);
		else if (size == 1) dst = Read16(addr);
		else dst = Read32(addr);
		dst &= mask;
		uint32_t result = (~dst) & mask;
		SetFlags_Logical(result, size);
		if (size == 0) Write8(addr, (uint8_t)result);
		else if (size == 1) Write16(addr, (uint16_t)result);
		else Write32(addr, result);
	}
	AddCycles(4);
}

// ===== Shifts / Rotates =====

void GenesisM68k::Op_ASd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3f;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		if (left) {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, (data & msb) != 0);
			uint32_t old = data;
			data = (data << 1) & mask;
			if ((old ^ data) & msb) SetCcr(M68kFlags::Overflow);
		} else {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, data & 1);
			uint32_t sign = data & msb;
			data = (data >> 1) | sign;
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	if (count == 0) ClearCcr(M68kFlags::Carry);
	AddCycles(6 + 2 * count);
}

void GenesisM68k::Op_LSd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3f;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		if (left) {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, (data & msb) != 0);
			data = (data << 1) & mask;
		} else {
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, data & 1);
			data >>= 1;
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	if (count == 0) ClearCcr(M68kFlags::Carry);
	AddCycles(6 + 2 * count);
}

void GenesisM68k::Op_ROd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3f;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Carry | M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		if (left) {
			bool bit = (data & msb) != 0;
			data = ((data << 1) | (bit ? 1 : 0)) & mask;
			SetCcrIf(M68kFlags::Carry, bit);
		} else {
			bool bit = (data & 1) != 0;
			data = (data >> 1) | (bit ? msb : 0);
			SetCcrIf(M68kFlags::Carry, bit);
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	if (count == 0) ClearCcr(M68kFlags::Carry);
	AddCycles(6 + 2 * count);
}

void GenesisM68k::Op_ROXd(uint16_t opcode) {
	bool left = (opcode >> 8) & 1;
	uint8_t size = (opcode >> 6) & 3;
	uint32_t mask = SizeMask(size);
	uint32_t msb = SignBit(size);

	uint8_t reg = opcode & 7;
	uint8_t count;
	if (opcode & 0x20) {
		count = _state.D[(opcode >> 9) & 7] & 0x3f;
	} else {
		count = (opcode >> 9) & 7;
		if (count == 0) count = 8;
	}

	uint32_t data = _state.D[reg] & mask;
	ClearCcr(M68kFlags::Overflow);

	for (uint8_t i = 0; i < count; i++) {
		bool x = GetCcr(M68kFlags::Extend);
		if (left) {
			bool bit = (data & msb) != 0;
			data = ((data << 1) | (x ? 1 : 0)) & mask;
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, bit);
		} else {
			bool bit = (data & 1) != 0;
			data = (data >> 1) | (x ? msb : 0);
			SetCcrIf(M68kFlags::Carry | M68kFlags::Extend, bit);
		}
	}

	_state.D[reg] = (_state.D[reg] & ~mask) | (data & mask);
	SetCcrIf(M68kFlags::Zero, (data & mask) == 0);
	SetCcrIf(M68kFlags::Negative, (data & msb) != 0);
	AddCycles(6 + 2 * count);
}

// ===== Bit operations =====

void GenesisM68k::Op_BTST(uint16_t opcode) {
	bool dynamicBit = (opcode & 0x0100) != 0;
	uint8_t bitNum;
	uint32_t btstPc = (_state.PC - 2) & 0x00ffffff;
	if (dynamicBit) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
	} else {
		bitNum &= 7;
		uint8_t data = (uint8_t)ReadEa(mode, reg, 0);
		static uint64_t btstMemCount = 0;
		btstMemCount++;
		if ((btstMemCount <= 128 || (btstMemCount % 4096) == 0) && bitNum == 3) {
			MessageManager::Log(std::format("[Genesis][M68K] BTST.mem #{} pc=${:06x} op=${:04x} mode={} reg={} bit={} data=${:02x} z={}",
				btstMemCount,
				btstPc,
				opcode,
				mode,
				reg,
				bitNum,
				data,
				((data & (1 << bitNum)) == 0) ? 1 : 0));
		}
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
	}

	int cycles = 0;
	if (mode == 0) {
		cycles = dynamicBit ? 6 : 10;
	} else {
		cycles = dynamicBit ? 8 : 12;
		switch (mode) {
			case 5: // (d16,An)
			case 6: // (d8,An,Xn)
				cycles += 4;
				break;
			case 7:
				switch (reg) {
					case 0: // (xxx).W
					case 2: // (d16,PC)
					case 3: // (d8,PC,Xn)
						cycles += 4;
						break;
					case 1: // (xxx).L
						cycles += 8;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	AddCycles(cycles);
}

void GenesisM68k::Op_BSET(uint16_t opcode) {
	uint8_t bitNum;
	if (opcode & 0x0100) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
		_state.D[reg] |= (1u << bitNum);
	} else {
		bitNum &= 7;
		uint32_t addr = GetEffectiveAddress(mode, reg, 0);
		uint8_t data = Read8(addr);
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
		Write8(addr, data | (1 << bitNum));
	}
	AddCycles(8);
}

void GenesisM68k::Op_BCLR(uint16_t opcode) {
	uint8_t bitNum;
	if (opcode & 0x0100) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
		_state.D[reg] &= ~(1u << bitNum);
	} else {
		bitNum &= 7;
		uint32_t addr = GetEffectiveAddress(mode, reg, 0);
		uint8_t data = Read8(addr);
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
		Write8(addr, data & ~(1 << bitNum));
	}
	AddCycles(8);
}

void GenesisM68k::Op_BCHG(uint16_t opcode) {
	uint8_t bitNum;
	if (opcode & 0x0100) {
		bitNum = _state.D[(opcode >> 9) & 7];
	} else {
		bitNum = (uint8_t)FetchWord();
	}

	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if (mode == 0) {
		bitNum &= 31;
		SetCcrIf(M68kFlags::Zero, !(_state.D[reg] & (1u << bitNum)));
		_state.D[reg] ^= (1u << bitNum);
	} else {
		bitNum &= 7;
		uint32_t addr = GetEffectiveAddress(mode, reg, 0);
		uint8_t data = Read8(addr);
		SetCcrIf(M68kFlags::Zero, !(data & (1 << bitNum)));
		Write8(addr, data ^ (1 << bitNum));
	}
	AddCycles(8);
}

// ===== Branch =====

void GenesisM68k::Op_BRA(uint16_t opcode) {
	int8_t disp8 = (int8_t)(opcode & 0xff);
	uint32_t basePc = _state.PC;
	int32_t offset;
	uint32_t targetBase;
	if (disp8 == 0) {
		offset = (int16_t)FetchWord();
		targetBase = basePc;
	} else {
		offset = disp8;
		targetBase = _state.PC;
	}
	_state.PC = targetBase + offset;
	AddCycles(10);
}

void GenesisM68k::Op_BSR(uint16_t opcode) {
	int8_t disp8 = (int8_t)(opcode & 0xff);
	uint32_t basePc = _state.PC;
	int32_t offset;
	uint32_t targetBase;
	if (disp8 == 0) {
		offset = (int16_t)FetchWord();
		targetBase = basePc;
	} else {
		offset = disp8;
		targetBase = _state.PC;
	}
	uint32_t returnPC = _state.PC;
	Push32(returnPC);
	_state.PC = targetBase + offset;
	AddCycles(18);
}

void GenesisM68k::Op_Bcc(uint16_t opcode) {
	uint8_t cc = (opcode >> 8) & 0x0f;
	int8_t disp8 = (int8_t)(opcode & 0xff);
	uint32_t basePc = _state.PC;
	int32_t offset;
	uint32_t targetBase;

	if (disp8 == 0) {
		offset = (int16_t)FetchWord();
		targetBase = basePc;
	} else {
		offset = disp8;
		targetBase = _state.PC;
	}

	if (TestCondition(cc)) {
		_state.PC = targetBase + offset;
		AddCycles(10);
	} else {
		AddCycles(disp8 == 0 ? 12 : 8);
	}
}

void GenesisM68k::Op_DBcc(uint16_t opcode) {
	uint8_t cc = (opcode >> 8) & 0x0f;
	uint8_t reg = opcode & 7;
	uint32_t basePc = _state.PC;
	int16_t disp = (int16_t)FetchWord();

	if (!TestCondition(cc)) {
		int16_t counter = (int16_t)(uint16_t)_state.D[reg];
		counter--;
		_state.D[reg] = (_state.D[reg] & 0xffff0000) | (uint16_t)counter;
		if (counter != -1) {
			_state.PC = basePc + disp;
			AddCycles(10);
		} else {
			AddCycles(14);
		}
	} else {
		AddCycles(12);
	}
}

void GenesisM68k::Op_Scc(uint16_t opcode) {
	uint8_t cc = (opcode >> 8) & 0x0f;
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	uint8_t val = TestCondition(cc) ? 0xff : 0x00;
	WriteEa(mode, reg, 0, val);
	AddCycles(mode == 0 ? (TestCondition(cc) ? 6 : 4) : 8);
}

void GenesisM68k::Op_JMP(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	_state.PC = GetEffectiveAddress(mode, reg, 2);
	AddCycles(8);
}

void GenesisM68k::Op_JSR(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;
	uint32_t addr = GetEffectiveAddress(mode, reg, 2);
	Push32(_state.PC);
	_state.PC = addr;
	AddCycles(16);
}

void GenesisM68k::Op_RTS(uint16_t opcode) {
	_state.PC = Pop32();
	AddCycles(16);
}

void GenesisM68k::Op_RTE(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8); // Privilege violation
		return;
	}
	uint16_t newSR = Pop16();
	_state.PC = Pop32();

	bool wasSupervisor = IsSupervisor();
	_state.SR = newSR;
	if (wasSupervisor && !IsSupervisor()) {
		_state.SSP = _state.A[7];
		_state.A[7] = _state.USP;
	}
	AddCycles(20);
}

void GenesisM68k::Op_RTR(uint16_t opcode) {
	uint16_t ccr = Pop16();
	_state.SR = (_state.SR & 0xff00) | (ccr & 0x001f);
	_state.PC = Pop32();
	AddCycles(20);
}

// ===== System =====

void GenesisM68k::Op_NOP(uint16_t opcode) {
	AddCycles(4);
}

void GenesisM68k::Op_TRAP(uint16_t opcode) {
	uint8_t vector = 32 + (opcode & 0x0f);
	RaiseException(vector);
}

void GenesisM68k::Op_RESET(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8);
		return;
	}
	// Reset external devices only (not CPU itself)
	AddCycles(132);
}

void GenesisM68k::Op_STOP(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8);
		return;
	}
	uint16_t newSR = FetchWord();
	_state.SR = newSR;
	_state.Stopped = true;
	AddCycles(4);
}

void GenesisM68k::Op_MOVE_SR(uint16_t opcode) {
	uint8_t mode = (opcode >> 3) & 7;
	uint8_t reg = opcode & 7;

	if ((opcode & 0xffc0) == 0x40c0) {
		// MOVE from SR
		WriteEa(mode, reg, 1, _state.SR);
		AddCycles(6);
	} else if ((opcode & 0xffc0) == 0x44c0) {
		// MOVE to CCR
		uint16_t data = (uint16_t)ReadEa(mode, reg, 1);
		_state.SR = (_state.SR & 0xff00) | (data & 0x001f);
		AddCycles(12);
	} else if ((opcode & 0xffc0) == 0x46c0) {
		// MOVE to SR
		if (!IsSupervisor()) {
			RaiseException(8);
			return;
		}
		uint16_t data = (uint16_t)ReadEa(mode, reg, 1);
		bool wasSupervisor = IsSupervisor();
		_state.SR = data;
		if (wasSupervisor && !IsSupervisor()) {
			_state.SSP = _state.A[7];
			_state.A[7] = _state.USP;
		}
		AddCycles(12);
	}
}

void GenesisM68k::Op_MOVE_USP(uint16_t opcode) {
	if (!IsSupervisor()) {
		RaiseException(8);
		return;
	}
	uint8_t reg = opcode & 7;
	if (opcode & 0x08) {
		// MOVE USP, An
		_state.A[reg] = _state.USP;
	} else {
		// MOVE An, USP
		_state.USP = _state.A[reg];
	}
	AddCycles(4);
}

void GenesisM68k::Op_ANDI_SR(uint16_t opcode) {
	if ((opcode & 0xff) == 0x3c) {
		// ANDI to CCR
		uint16_t imm = FetchWord();
		_state.SR &= (0xff00 | (imm & 0x001f));
	} else {
		if (!IsSupervisor()) { RaiseException(8); return; }
		uint16_t imm = FetchWord();
		_state.SR &= imm;
	}
	AddCycles(20);
}

void GenesisM68k::Op_ORI_SR(uint16_t opcode) {
	if ((opcode & 0xff) == 0x3c) {
		uint16_t imm = FetchWord();
		_state.SR |= (imm & 0x001f);
	} else {
		if (!IsSupervisor()) { RaiseException(8); return; }
		uint16_t imm = FetchWord();
		_state.SR |= imm;
	}
	AddCycles(20);
}

void GenesisM68k::Op_EORI_SR(uint16_t opcode) {
	if ((opcode & 0xff) == 0x3c) {
		uint16_t imm = FetchWord();
		_state.SR ^= (imm & 0x001f);
	} else {
		if (!IsSupervisor()) { RaiseException(8); return; }
		uint16_t imm = FetchWord();
		_state.SR ^= imm;
	}
	AddCycles(20);
}

void GenesisM68k::Op_ILLEGAL(uint16_t opcode) {
	static uint64_t illegalCount = 0;
	illegalCount++;
	if (illegalCount <= 16 || (illegalCount % 1024) == 0) {
		MessageManager::Log(std::format("[Genesis][M68K] Illegal opcode #{} op=${:04x} pc=${:06x}", illegalCount, opcode, _state.PC - 2));
	}
	_state.PC -= 2; // Back up to illegal opcode
	RaiseException(4); // Illegal instruction
}

// ===== Serialization =====

void GenesisM68k::Serialize(Serializer& s) {
	for (int i = 0; i < 8; i++) {
		SVI(_state.D[i]);
		SVI(_state.A[i]);
	}
	SV(_state.PC);
	SV(_state.USP);
	SV(_state.SSP);
	SV(_state.SR);
	SV(_state.CycleCount);
	SV(_state.Stopped);
	SV(_pendingInterruptLevel);
	SV(_forcedCycleFloorCount);
	SV(_forcedClockAdvanceCount);
	SV(_resetProbeCount);
	SV(_lastResetVectorSp);
	SV(_lastResetVectorPc);
	SV(_firstDispatchCaptured);
	SV(_dispatchFaultCount);
	SV(_dispatchGuardHitCount);
	SV(_decodeFaultCount);
	SV(_execCallCount);
	SV(_lastFetchProgramCounter);
	SV(_lastFetchOpcode);
	SV(_lastFetchPreviewWordA);
	SV(_lastFetchPreviewWordB);
	SV(_lastDispatchFaultSummary);
	SV(_lastDispatchBoundarySummary);
	SV(_lastDecodedGroup);
	SV(_lastDecodedSubOp);
	SV(_lastDecodedMode);
	SV(_lastDecodedReg);
	SVArray(_decodeGroupHitCount, 16);
	SV(_lastDecodeRouteSummary);
	SV(_instructionFlowConfigLoaded);
	SV(_instructionFlowLogEnabled);
	SV(_instructionFlowLogLimit);
	SV(_instructionFlowLogStride);
	SV(_instructionFlowLogCount);
	SV(_instructionFlowLogSkipped);
	SV(_lastInstructionFlowLogLine);
	SV(_strictAddressErrorChecks);
	SV(_addressErrorCount);
	SV(_lastAddressErrorAddr);
	SV(_lastAddressErrorPc);
	SV(_lastAddressErrorSize);
	SV(_lastAddressErrorWrite);
	SV(_lastAddressErrorSource);
	SV(_recentInstructionFlowCapacity);
	uint32_t flowLineCount = (uint32_t)_recentInstructionFlowLogs.size();
	SV(flowLineCount);
	if (!s.IsSaving()) {
		_recentInstructionFlowLogs.clear();
		_recentInstructionFlowLogs.reserve(_recentInstructionFlowCapacity);
	}

	for (uint32_t i = 0; i < flowLineCount; i++) {
		string flowLine = s.IsSaving() ? _recentInstructionFlowLogs[i] : string{};
		SV(flowLine);
		if (!s.IsSaving()) {
			_recentInstructionFlowLogs.push_back(flowLine);
		}
	}
}
