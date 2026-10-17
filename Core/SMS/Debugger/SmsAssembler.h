#pragma once
#include "pch.h"
#include "Debugger/IAssembler.h"

class LabelManager;
enum class HlRegType;

class SmsAssembler : public IAssembler {
	enum class ParamType {
		None,
		Literal,
		Byte,
		Short,
		Address,
		RelAddress,
		Port,
		IxOffset,
		IyOffset
	};

	struct ParamEntry {
		string Param;
		ParamType Type;
	};

	struct OpCodeEntry {
		uint32_t OpCode;
		int ParamCount;
		ParamEntry Param1;
		ParamEntry Param2;
		ParamEntry Param3;
	};

private:
	unordered_map<string, vector<OpCodeEntry>> _opCodes;
	LabelManager* _labelManager;

	void InitParamEntry(ParamEntry& entry, string param, HlRegType hlRegType);
	[[nodiscard]] bool IsRegisterName(string op);
	void InitAssembler();
	int ReadValue(const string& operand, int min, int max, unordered_map<string, uint16_t>& localLabels, bool firstPass, bool isPortValue = false);
	[[nodiscard]] bool IsMatch(ParamEntry& entry, string operand, uint32_t address, unordered_map<string, uint16_t>& localLabels, bool firstPass);
	void PushOp(uint32_t opCode, vector<int16_t>& output, uint32_t& address);
	void PushByte(uint8_t operand, vector<int16_t>& output, uint32_t& address, bool insertBeforeOpCode = false);
	void PushWord(uint16_t operand, vector<int16_t>& output, uint32_t& address);
	void ProcessOperand(OpCodeEntry& opEntry, ParamEntry& entry, const string& operand, vector<int16_t>& output, uint32_t& address, unordered_map<string, uint16_t>& localLabels, bool firstPass);

	void RunPass(vector<int16_t>& output, const string& code, uint32_t address, int16_t* assembledCode, bool firstPass, unordered_map<string, uint16_t>& localLabels);

public:
	SmsAssembler(LabelManager* labelManager);
	virtual ~SmsAssembler();

	uint32_t AssembleCode(const string& code, uint32_t startAddress, int16_t* assembledCode);
};
