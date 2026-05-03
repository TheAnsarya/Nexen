#include "pch.h"
#include <algorithm>
#include <cctype>
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/MemoryDumper.h"
#include "Debugger/DebugUtilities.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/FastString.h"
#include "SNES/SnesCpuTypes.h"
#include "SNES/SnesConsole.h"
#include "SNES/BaseCartridge.h"
#include "SNES/Debugger/SnesDisUtils.h"
#include "SNES/Debugger/SpcDisUtils.h"
#include "SNES/Debugger/GsuDisUtils.h"
#include "SNES/Debugger/NecDspDisUtils.h"
#include "SNES/Debugger/Cx4DisUtils.h"
#include "SNES/Debugger/St018DisUtils.h"
#include "Gameboy/Debugger/GameboyDisUtils.h"
#include "NES/Debugger/NesDisUtils.h"
#include "PCE/Debugger/PceDisUtils.h"
#include "SMS/Debugger/SmsDisUtils.h"
#include "GBA/Debugger/GbaDisUtils.h"
#include "WS/Debugger/WsDisUtils.h"
#include "Lynx/Debugger/LynxDisUtils.h"
#include "Atari2600/Debugger/Atari2600DisUtils.h"
#include "ChannelF/Debugger/ChannelFDisUtils.h"
#include "ChannelF/ChannelFTypes.h"
#include "Shared/EmuSettings.h"

namespace {
	uint16_t ReadBe16(const uint8_t* buffer, int offset) {
		return (uint16_t)((buffer[offset] << 8) | buffer[offset + 1]);
	}

	uint32_t ReadBe32(const uint8_t* buffer, int offset) {
		return (uint32_t)((buffer[offset] << 24) | (buffer[offset + 1] << 16) | (buffer[offset + 2] << 8) | buffer[offset + 3]);
	}

	string ToMnemonic(const char* mnemonic, bool useLowerCase) {
		string result = mnemonic;
		if (!useLowerCase) {
			std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return (char)std::toupper(c); });
		}
		return result;
	}

	string RegName(char regType, uint8_t regIndex, bool useLowerCase) {
		char prefix = useLowerCase ? (char)std::tolower((unsigned char)regType) : regType;
		return std::format("{}{}", prefix, regIndex);
	}

	int GetGenesisEaExtensionSize(uint8_t mode, uint8_t reg, uint8_t operandSize) {
		switch (mode) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
				return 0;
			case 5:
			case 6:
				return 2;
			case 7:
				switch (reg) {
					case 0: return 2;
					case 1: return 4;
					case 2: return 2;
					case 3: return 2;
					case 4: return operandSize >= 4 ? 4 : 2;
					default: return 0;
				}
			default:
				return 0;
		}
	}

	string FormatGenesisEa(const uint8_t* byteCode, uint8_t opSize, uint8_t mode, uint8_t reg, uint8_t operandSize, uint32_t memoryAddr, int& extensionOffset, bool useLowerCase) {
		auto readExt16 = [&]() -> uint16_t {
			if (extensionOffset + 1 >= opSize) {
				return 0;
			}
			uint16_t value = ReadBe16(byteCode, extensionOffset);
			extensionOffset += 2;
			return value;
		};

		auto readExt32 = [&]() -> uint32_t {
			if (extensionOffset + 3 >= opSize) {
				return 0;
			}
			uint32_t value = ReadBe32(byteCode, extensionOffset);
			extensionOffset += 4;
			return value;
		};

		switch (mode) {
			case 0: return RegName('D', reg, useLowerCase);
			case 1: return RegName('A', reg, useLowerCase);
			case 2: return std::format("({})", RegName('A', reg, useLowerCase));
			case 3: return std::format("({})+", RegName('A', reg, useLowerCase));
			case 4: return std::format("-({})", RegName('A', reg, useLowerCase));
			case 5: {
				int16_t disp = (int16_t)readExt16();
				return std::format("({:+d},{})", disp, RegName('A', reg, useLowerCase));
			}
			case 6: {
				int16_t disp = (int8_t)(readExt16() & 0x00FF);
				return std::format("({:+d},{})", disp, RegName('A', reg, useLowerCase));
			}
			case 7:
				switch (reg) {
					case 0: return std::format("${:04x}", readExt16());
					case 1: return std::format("${:08x}", readExt32());
					case 2: {
						int16_t disp = (int16_t)readExt16();
						return std::format("({:+d},{})", disp, useLowerCase ? "pc" : "PC");
					}
					case 3: {
						int16_t disp = (int8_t)(readExt16() & 0x00FF);
						return std::format("({:+d},{})", disp, useLowerCase ? "pc" : "PC");
					}
					case 4: {
						if (operandSize >= 4) {
							return std::format("#${:08x}", readExt32());
						}
						return std::format("#${:04x}", readExt16());
					}
				}
				break;
		}

		return std::format("${:06x}", memoryAddr);
	}

	uint8_t GetGenesisOpSize(uint32_t cpuAddress, MemoryType memType, MemoryDumper* memoryDumper) {
		uint8_t hi = memoryDumper->GetMemoryValue(memType, cpuAddress);
		uint8_t lo = memoryDumper->GetMemoryValue(memType, cpuAddress + 1);
		uint16_t opCode = (uint16_t)((hi << 8) | lo);

		uint8_t opClass = (opCode >> 12) & 0x0F;
		if (opClass >= 1 && opClass <= 3) {
			uint8_t srcMode = (opCode >> 3) & 0x07;
			uint8_t srcReg = opCode & 0x07;
			uint8_t dstMode = (opCode >> 6) & 0x07;
			uint8_t dstReg = (opCode >> 9) & 0x07;

			uint8_t operandSize = opClass == 1 ? 1 : (opClass == 2 ? 4 : 2);
			int size = 2 + GetGenesisEaExtensionSize(srcMode, srcReg, operandSize) + GetGenesisEaExtensionSize(dstMode, dstReg, operandSize);
			return (uint8_t)std::clamp(size, 2, 8);
		}

		if (opClass == 6) {
			uint8_t disp8 = (uint8_t)(opCode & 0x00FF);
			return disp8 == 0 ? 4 : 2;
		}

		if (opCode == 0x4e72) {
			return 4;
		}

		if ((opCode & 0xFFF8) == 0x4e50) {
			return 4;
		}

		if ((opCode & 0xFFC0) == 0x4e80 || (opCode & 0xFFC0) == 0x4ec0 || (opCode & 0xF1C0) == 0x41c0 || (opCode & 0xFFC0) == 0x4840) {
			uint8_t mode = (opCode >> 3) & 0x07;
			uint8_t reg = opCode & 0x07;
			int size = 2 + GetGenesisEaExtensionSize(mode, reg, 4);
			return (uint8_t)std::clamp(size, 2, 8);
		}

		return 2;
	}
}

DisassemblyInfo::DisassemblyInfo() {
}

DisassemblyInfo::DisassemblyInfo(uint32_t cpuAddress, uint8_t cpuFlags, CpuType cpuType, MemoryType memType, MemoryDumper* memoryDumper) {
	Initialize(cpuAddress, cpuFlags, cpuType, memType, memoryDumper);
}

void DisassemblyInfo::Initialize(uint32_t cpuAddress, uint8_t cpuFlags, CpuType cpuType, MemoryType memType, MemoryDumper* memoryDumper) {
	_cpuType = cpuType;
	_flags = cpuFlags;

	_byteCode[0] = memoryDumper->GetMemoryValue(memType, cpuAddress);

	_opSize = GetOpSize(_byteCode[0], _flags, _cpuType, cpuAddress, memType, memoryDumper);

	for (int i = 1; i < _opSize && i < 8; i++) {
		_byteCode[i] = memoryDumper->GetMemoryValue(memType, cpuAddress + i);
	}

	_initialized = true;
}

bool DisassemblyInfo::IsInitialized() {
	return _initialized;
}

bool DisassemblyInfo::IsValid(uint8_t cpuFlags) {
	return _flags == cpuFlags;
}

void DisassemblyInfo::Reset() {
	_initialized = false;
}

void DisassemblyInfo::GetDisassembly(string& out, uint32_t memoryAddr, LabelManager* labelManager, EmuSettings* settings) {
	switch (_cpuType) {
		case CpuType::Sa1:
		case CpuType::Snes:
			SnesDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;

		case CpuType::Spc:
			SpcDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::NecDsp:
			NecDspDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Gsu:
			GsuDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Cx4:
			Cx4DisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::St018:
			GbaDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Gameboy:
			GameboyDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Nes:
			NesDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Pce:
			PceDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Sms:
			SmsDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Gba:
			GbaDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Ws:
			WsDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Lynx:
			LynxDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::Atari2600:
			Atari2600DisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;
		case CpuType::ChannelF:
			ChannelFDisUtils::GetDisassembly(*this, out, memoryAddr, labelManager, settings);
			break;

		case CpuType::Genesis: {
			bool useLowerCase = settings->GetDebugConfig().UseLowerCaseDisassembly;
			uint16_t opCode = ReadBe16(_byteCode, 0);

			auto addBranch = [&](const char* mnemonic) {
				int32_t displacement = (int8_t)(opCode & 0x00FF);
				int32_t baseAddress = (int32_t)memoryAddr + 2;
				if ((opCode & 0x00FF) == 0 && _opSize >= 4) {
					displacement = (int16_t)ReadBe16(_byteCode, 2);
				}
				uint32_t target = (uint32_t)((baseAddress + displacement) & 0x00ffffff);
				out += std::format("{} ${:06x}", ToMnemonic(mnemonic, useLowerCase), target);
			};

			if (opCode == 0x4e71) {
				out += ToMnemonic("nop", useLowerCase);
				break;
			} else if (opCode == 0x4e75) {
				out += ToMnemonic("rts", useLowerCase);
				break;
			} else if (opCode == 0x4e73) {
				out += ToMnemonic("rte", useLowerCase);
				break;
			} else if (opCode == 0x4e77) {
				out += ToMnemonic("rtr", useLowerCase);
				break;
			} else if (opCode == 0x4e70) {
				out += ToMnemonic("reset", useLowerCase);
				break;
			} else if (opCode == 0x4e72 && _opSize >= 4) {
				out += std::format("{} #${:04x}", ToMnemonic("stop", useLowerCase), ReadBe16(_byteCode, 2));
				break;
			} else if ((opCode & 0xF100) == 0x7000) {
				uint8_t dstReg = (opCode >> 9) & 0x07;
				int8_t imm = (int8_t)(opCode & 0x00FF);
				out += std::format("{} #{:+d},{}", ToMnemonic("moveq", useLowerCase), imm, RegName('D', dstReg, useLowerCase));
				break;
			} else if ((opCode & 0xF000) == 0x6000) {
				static const char* branchMnemonics[16] = {"bra", "bsr", "bhi", "bls", "bcc", "bcs", "bne", "beq", "bvc", "bvs", "bpl", "bmi", "bge", "blt", "bgt", "ble"};
				addBranch(branchMnemonics[(opCode >> 8) & 0x0F]);
				break;
			} else if ((opCode & 0xFFF8) == 0x4e50 && _opSize >= 4) {
				uint8_t reg = (opCode & 0x0007);
				out += std::format("{} {},#${:04x}", ToMnemonic("link", useLowerCase), RegName('A', reg, useLowerCase), ReadBe16(_byteCode, 2));
				break;
			} else if ((opCode & 0xFFF8) == 0x4e58) {
				out += std::format("{} {}", ToMnemonic("unlk", useLowerCase), RegName('A', (uint8_t)(opCode & 0x0007), useLowerCase));
				break;
			} else if ((opCode & 0xFFC0) == 0x4e80 || (opCode & 0xFFC0) == 0x4ec0 || (opCode & 0xF1C0) == 0x41c0 || (opCode & 0xFFC0) == 0x4840) {
				uint8_t mode = (opCode >> 3) & 0x07;
				uint8_t reg = opCode & 0x07;
				int offset = 2;
				string ea = FormatGenesisEa(_byteCode, _opSize, mode, reg, 4, memoryAddr, offset, useLowerCase);

				if ((opCode & 0xFFC0) == 0x4e80) {
					out += std::format("{} {}", ToMnemonic("jsr", useLowerCase), ea);
				} else if ((opCode & 0xFFC0) == 0x4ec0) {
					out += std::format("{} {}", ToMnemonic("jmp", useLowerCase), ea);
				} else if ((opCode & 0xF1C0) == 0x41c0) {
					uint8_t dstReg = (opCode >> 9) & 0x07;
					out += std::format("{}.l {},{}", ToMnemonic("lea", useLowerCase), ea, RegName('A', dstReg, useLowerCase));
				} else {
					out += std::format("{} {}", ToMnemonic("pea", useLowerCase), ea);
				}
				break;
			} else if (((opCode >> 12) & 0x0F) >= 1 && ((opCode >> 12) & 0x0F) <= 3) {
				uint8_t opClass = (opCode >> 12) & 0x0F;
				uint8_t srcMode = (opCode >> 3) & 0x07;
				uint8_t srcReg = opCode & 0x07;
				uint8_t dstMode = (opCode >> 6) & 0x07;
				uint8_t dstReg = (opCode >> 9) & 0x07;
				char sizeSuffix = opClass == 1 ? 'b' : (opClass == 2 ? 'l' : 'w');
				uint8_t operandSize = opClass == 1 ? 1 : (opClass == 2 ? 4 : 2);

				int srcOffset = 2;
				string src = FormatGenesisEa(_byteCode, _opSize, srcMode, srcReg, operandSize, memoryAddr, srcOffset, useLowerCase);
				int dstOffset = srcOffset;
				string dst = FormatGenesisEa(_byteCode, _opSize, dstMode, dstReg, operandSize, memoryAddr, dstOffset, useLowerCase);

				if (dstMode == 1) {
					out += std::format("{}.{} {},{}", ToMnemonic("movea", useLowerCase), sizeSuffix == 'b' ? 'w' : sizeSuffix, src, dst);
				} else {
					out += std::format("{}.{} {},{}", ToMnemonic("move", useLowerCase), sizeSuffix, src, dst);
				}
				break;
			}

			out += std::format("{} ${:04x}", ToMnemonic("dc.w", useLowerCase), opCode);
			break;
		}

		[[unlikely]] default:
			throw std::runtime_error("GetDisassembly - Unsupported CPU type");
	}
}

EffectiveAddressInfo DisassemblyInfo::GetEffectiveAddress(Debugger* debugger, void* cpuState, CpuType cpuType) {
	switch (_cpuType) {
		case CpuType::Sa1:
		case CpuType::Snes:
			return SnesDisUtils::GetEffectiveAddress(*this, (SnesConsole*)debugger->GetConsole(), *(SnesCpuState*)cpuState, cpuType);

		case CpuType::Spc:
			return SpcDisUtils::GetEffectiveAddress(*this, (SnesConsole*)debugger->GetConsole(), *(SpcState*)cpuState);
		case CpuType::Gsu:
			return GsuDisUtils::GetEffectiveAddress(*this, (SnesConsole*)debugger->GetConsole(), *(GsuState*)cpuState);
		case CpuType::Cx4:
			return Cx4DisUtils::GetEffectiveAddress(*this, *(Cx4State*)cpuState, debugger->GetMemoryDumper());
		case CpuType::St018:
			return St018DisUtils::GetEffectiveAddress(*this, (SnesConsole*)debugger->GetConsole(), *(ArmV3CpuState*)cpuState);

		case CpuType::NecDsp:
			return {};

		case CpuType::Gameboy: {
			if (debugger->GetMainCpuType() == CpuType::Snes) {
				Gameboy* gb = ((SnesConsole*)debugger->GetConsole())->GetCartridge()->GetGameboy();
				return GameboyDisUtils::GetEffectiveAddress(*this, gb, *(GbCpuState*)cpuState);
			} else {
				return GameboyDisUtils::GetEffectiveAddress(*this, (Gameboy*)debugger->GetConsole(), *(GbCpuState*)cpuState);
			}
		}

		case CpuType::Nes:
			return NesDisUtils::GetEffectiveAddress(*this, *(NesCpuState*)cpuState, debugger->GetMemoryDumper());
		case CpuType::Pce:
			return PceDisUtils::GetEffectiveAddress(*this, (PceConsole*)debugger->GetConsole(), *(PceCpuState*)cpuState);
		case CpuType::Sms:
			return SmsDisUtils::GetEffectiveAddress(*this, (SmsConsole*)debugger->GetConsole(), *(SmsCpuState*)cpuState);
		case CpuType::Gba:
			return GbaDisUtils::GetEffectiveAddress(*this, (GbaConsole*)debugger->GetConsole(), *(GbaCpuState*)cpuState);
		case CpuType::Ws:
			return WsDisUtils::GetEffectiveAddress(*this, (WsConsole*)debugger->GetConsole(), *(WsCpuState*)cpuState);
		case CpuType::Lynx:
			return LynxDisUtils::GetEffectiveAddress(*this, (LynxConsole*)debugger->GetConsole(), *(LynxCpuState*)cpuState);
		case CpuType::Atari2600:
			return Atari2600DisUtils::GetEffectiveAddress(*this, *(Atari2600CpuState*)cpuState, debugger->GetMemoryDumper());
		case CpuType::ChannelF:
			return ChannelFDisUtils::GetEffectiveAddress(*this, *(ChannelFCpuState*)cpuState, debugger->GetMemoryDumper());
		case CpuType::Genesis:
			return {};
	}

	[[unlikely]] throw std::runtime_error("GetEffectiveAddress - Unsupported CPU type");
}

CpuType DisassemblyInfo::GetCpuType() {
	return _cpuType;
}

uint8_t DisassemblyInfo::GetOpCode() {
	return _byteCode[0];
}

template <CpuType type>
uint32_t DisassemblyInfo::GetFullOpCode() {
	switch (type) {
		default:
			return _byteCode[0];
		case CpuType::NecDsp:
			return _byteCode[0] | (_byteCode[1] << 8) | (_byteCode[2] << 16);
		case CpuType::Cx4:
			return _byteCode[1];
		case CpuType::St018:
			return _byteCode[0] | (_byteCode[1] << 8) | (_opSize == 4 ? ((_byteCode[2] << 16) | (_byteCode[3] << 24)) : 0);
		case CpuType::Gba:
			return _byteCode[0] | (_byteCode[1] << 8) | (_opSize == 4 ? ((_byteCode[2] << 16) | (_byteCode[3] << 24)) : 0);
		case CpuType::Ws:
			return WsDisUtils::GetFullOpCode(*this);
	}
}

uint8_t DisassemblyInfo::GetOpSize() {
	return _opSize;
}

uint8_t DisassemblyInfo::GetFlags() {
	return _flags;
}

uint8_t* DisassemblyInfo::GetByteCode() {
	return _byteCode;
}

void DisassemblyInfo::GetByteCode(uint8_t copyBuffer[8]) {
	memcpy(copyBuffer, _byteCode, _opSize);
}

void DisassemblyInfo::GetByteCode(string& out) {
	FastString str;
	for (int i = 0; i < _opSize; i++) {
		str.WriteAll('$', HexUtilities::ToHex(_byteCode[i]));
		if (i < _opSize - 1) {
			str.Write(' ');
		}
	}
	out += str.ToString();
}

uint8_t DisassemblyInfo::GetOpSize(uint32_t opCode, uint8_t flags, CpuType type, uint32_t cpuAddress, MemoryType memType, MemoryDumper* memoryDumper) {
	switch (type) {
		case CpuType::Snes:
			return SnesDisUtils::GetOpSize(opCode, flags);
		case CpuType::Spc:
			return SpcDisUtils::GetOpSize(opCode);
		case CpuType::NecDsp:
			return NecDspDisUtils::GetOpSize();
		case CpuType::Sa1:
			return SnesDisUtils::GetOpSize(opCode, flags);
		case CpuType::Gsu:
			return GsuDisUtils::GetOpSize(opCode);
		case CpuType::Cx4:
			return Cx4DisUtils::GetOpSize();
		case CpuType::St018:
			return GbaDisUtils::GetOpSize(opCode, flags);
		case CpuType::Gameboy:
			return GameboyDisUtils::GetOpSize(opCode);
		case CpuType::Nes:
			return NesDisUtils::GetOpSize(opCode);
		case CpuType::Pce:
			return PceDisUtils::GetOpSize(opCode);
		case CpuType::Sms:
			return SmsDisUtils::GetOpSize(opCode, cpuAddress, memType, memoryDumper);
		case CpuType::Gba:
			return GbaDisUtils::GetOpSize(opCode, flags);
		case CpuType::Ws:
			return WsDisUtils::GetOpSize(cpuAddress, memType, memoryDumper);
		case CpuType::Lynx:
			return LynxDisUtils::GetOpSize(opCode);
		case CpuType::Atari2600:
			return Atari2600DisUtils::GetOpSize(opCode);
		case CpuType::ChannelF:
			return ChannelFDisUtils::GetOpSize(opCode);
		case CpuType::Genesis:
			return GetGenesisOpSize(cpuAddress, memType, memoryDumper);
	}

	[[unlikely]] throw std::runtime_error("GetOpSize - Unsupported CPU type");
}

bool DisassemblyInfo::IsJumpToSub() {
	switch (_cpuType) {
		case CpuType::Snes:
			return SnesDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Spc:
			return SpcDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::NecDsp:
			return NecDspDisUtils::IsJumpToSub(GetFullOpCode<CpuType::NecDsp>());
		case CpuType::Sa1:
			return SnesDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Gsu:
			return false; // GSU has no JSR op codes
		case CpuType::Cx4:
			return Cx4DisUtils::IsJumpToSub(GetFullOpCode<CpuType::Cx4>());
		case CpuType::St018:
			return GbaDisUtils::IsJumpToSub(GetFullOpCode<CpuType::St018>(), _flags);
		case CpuType::Gameboy:
			return GameboyDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Nes:
			return NesDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Pce:
			return PceDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Sms:
			return SmsDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Gba:
			return GbaDisUtils::IsJumpToSub(GetFullOpCode<CpuType::Gba>(), _flags);
		case CpuType::Ws:
			return WsDisUtils::IsJumpToSub(GetFullOpCode<CpuType::Ws>());
		case CpuType::Lynx:
			return LynxDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Atari2600:
			return Atari2600DisUtils::IsJumpToSub(GetOpCode());
		case CpuType::ChannelF:
			return ChannelFDisUtils::IsJumpToSub(GetOpCode());
		case CpuType::Genesis:
			return (ReadBe16(_byteCode, 0) & 0xFFC0) == 0x4e80 || (ReadBe16(_byteCode, 0) & 0xFF00) == 0x6100;
	}

	[[unlikely]] throw std::runtime_error("IsJumpToSub - Unsupported CPU type");
}

bool DisassemblyInfo::IsReturnInstruction() {
	switch (_cpuType) {
		case CpuType::Snes:
			return SnesDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::Spc:
			return SpcDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::NecDsp:
			return NecDspDisUtils::IsReturnInstruction(GetFullOpCode<CpuType::NecDsp>());
		case CpuType::Sa1:
			return SnesDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::Gsu:
			return false; // GSU has no RTS/RTI op codes
		case CpuType::Cx4:
			return Cx4DisUtils::IsReturnInstruction(GetFullOpCode<CpuType::Cx4>());
		case CpuType::St018:
			return GbaDisUtils::IsReturnInstruction(GetFullOpCode<CpuType::St018>(), _flags);
		case CpuType::Gameboy:
			return GameboyDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::Nes:
			return NesDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::Pce:
			return PceDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::Sms:
			return SmsDisUtils::IsReturnInstruction(_byteCode[0] | (_byteCode[1] << 8));
		case CpuType::Gba:
			return GbaDisUtils::IsReturnInstruction(GetFullOpCode<CpuType::Gba>(), _flags);
		case CpuType::Ws:
			return WsDisUtils::IsReturnInstruction(GetFullOpCode<CpuType::Ws>());
		case CpuType::Lynx:
			return LynxDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::Atari2600:
			return Atari2600DisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::ChannelF:
			return ChannelFDisUtils::IsReturnInstruction(GetOpCode());
		case CpuType::Genesis:
			return ReadBe16(_byteCode, 0) == 0x4e75 || ReadBe16(_byteCode, 0) == 0x4e73 || ReadBe16(_byteCode, 0) == 0x4e77;
	}

	[[unlikely]] throw std::runtime_error("IsReturnInstruction - Unsupported CPU type");
}

bool DisassemblyInfo::CanDisassembleNextOp() {
	if (IsUnconditionalJump()) {
		return false;
	}

	switch (_cpuType) {
		case CpuType::Snes:
			return SnesDisUtils::CanDisassembleNextOp(GetOpCode());
		case CpuType::Sa1:
			return SnesDisUtils::CanDisassembleNextOp(GetOpCode());
		case CpuType::Gsu:
			return GsuDisUtils::CanDisassembleNextOp(GetOpCode());
		case CpuType::Cx4:
			return Cx4DisUtils::CanDisassembleNextOp(GetByteCode()[1]);
		default:
			return true;
	}
}

bool DisassemblyInfo::IsUnconditionalJump() {
	switch (_cpuType) {
		case CpuType::Snes:
			return SnesDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Spc:
			return SpcDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::NecDsp:
			return NecDspDisUtils::IsUnconditionalJump(GetFullOpCode<CpuType::NecDsp>());
		case CpuType::Sa1:
			return SnesDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Gsu:
			return GsuDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Cx4:
			return Cx4DisUtils::IsUnconditionalJump(GetFullOpCode<CpuType::Cx4>());
		case CpuType::St018:
			return GbaDisUtils::IsUnconditionalJump(GetFullOpCode<CpuType::St018>(), _flags);
		case CpuType::Gameboy:
			return GameboyDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Nes:
			return NesDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Pce:
			return PceDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Sms:
			return SmsDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Gba:
			return GbaDisUtils::IsUnconditionalJump(GetFullOpCode<CpuType::Gba>(), _flags);
		case CpuType::Ws:
			return WsDisUtils::IsUnconditionalJump(GetFullOpCode<CpuType::Ws>());
		case CpuType::Lynx:
			return LynxDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Atari2600:
			return Atari2600DisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::ChannelF:
			return ChannelFDisUtils::IsUnconditionalJump(GetOpCode());
		case CpuType::Genesis:
			return (ReadBe16(_byteCode, 0) & 0xFFC0) == 0x4ec0 || (ReadBe16(_byteCode, 0) & 0xFF00) == 0x6000;
	}

	[[unlikely]] throw std::runtime_error("IsUnconditionalJump - Unsupported CPU type");
}

bool DisassemblyInfo::IsJump() {
	if (IsUnconditionalJump()) {
		return true;
	}

	// Check for conditional jumps
	switch (_cpuType) {
		case CpuType::Snes:
			return SnesDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Spc:
			return SpcDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::NecDsp:
			return NecDspDisUtils::IsConditionalJump(GetFullOpCode<CpuType::NecDsp>());
		case CpuType::Sa1:
			return SnesDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Gsu:
			return GsuDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Cx4:
			return Cx4DisUtils::IsConditionalJump(GetFullOpCode<CpuType::Cx4>(), GetByteCode()[0]);
		case CpuType::St018:
			return GbaDisUtils::IsConditionalJump(GetFullOpCode<CpuType::St018>(), GetByteCode()[0]);
		case CpuType::Gameboy:
			return GameboyDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Nes:
			return NesDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Pce:
			return PceDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Sms:
			return SmsDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Gba:
			return GbaDisUtils::IsConditionalJump(GetFullOpCode<CpuType::Gba>(), _flags);
		case CpuType::Ws:
			return WsDisUtils::IsConditionalJump(GetFullOpCode<CpuType::Ws>());
		case CpuType::Lynx:
			return LynxDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Atari2600:
			return Atari2600DisUtils::IsConditionalJump(GetOpCode());
		case CpuType::ChannelF:
			return ChannelFDisUtils::IsConditionalJump(GetOpCode());
		case CpuType::Genesis:
			return (ReadBe16(_byteCode, 0) & 0xF000) == 0x6000 && (ReadBe16(_byteCode, 0) & 0x0F00) != 0x0000;
	}

	[[unlikely]] throw std::runtime_error("IsJump - Unsupported CPU type");
}

void DisassemblyInfo::UpdateCpuFlags(uint8_t& cpuFlags) {
	switch (_cpuType) {
		case CpuType::Snes:
			SnesDisUtils::UpdateCpuFlags(GetOpCode(), GetByteCode(), cpuFlags);
			break;
		case CpuType::Sa1:
			SnesDisUtils::UpdateCpuFlags(GetOpCode(), GetByteCode(), cpuFlags);
			break;
		case CpuType::Gsu:
			GsuDisUtils::UpdateCpuFlags(GetOpCode(), cpuFlags);
			break;
		default:
			break;
	}
}

uint32_t DisassemblyInfo::GetMemoryValue(EffectiveAddressInfo effectiveAddress, MemoryDumper* memoryDumper, MemoryType memType) {
	MemoryType effectiveMemType = effectiveAddress.Type == MemoryType::None ? memType : effectiveAddress.Type;
	switch (effectiveAddress.ValueSize) {
		default:
		case 1:
			return memoryDumper->GetMemoryValue(effectiveMemType, effectiveAddress.Address);
		case 2:
			return memoryDumper->GetMemoryValue16(effectiveMemType, effectiveAddress.Address);
		case 4:
			return memoryDumper->GetMemoryValue32(effectiveMemType, effectiveAddress.Address);
	}
}
