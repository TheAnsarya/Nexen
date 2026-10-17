#pragma once
#include "pch.h"
#include "Debugger/IAssembler.h"
#include "Debugger/LabelManager.h"

class GbaAssembler : public IAssembler {
public:
	GbaAssembler(LabelManager* labelManager) {}

	uint32_t AssembleCode(const string& code, uint32_t startAddress, int16_t* assembledCode) override {
		return 0;
	}
};
