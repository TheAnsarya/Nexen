#pragma once
#include "pch.h"
#include "SNES/RamHandler.h"

class RomHandler : public RamHandler {
public:
	RomHandler(uint8_t* ram, uint32_t offset, uint32_t size, MemoryType memoryType) : RamHandler(ram, offset, size, memoryType) {
		_directWritePtr = nullptr;
	}

	void Write(uint32_t addr, uint8_t value) override {
	}
};