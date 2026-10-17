#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"

class IMemoryHandler {
protected:
	MemoryType _memoryType;
	uint8_t* _directReadPtr = nullptr;
	uint8_t* _directWritePtr = nullptr;
	uint32_t _directMask = 0;

public:
	IMemoryHandler(MemoryType memType) {
		_memoryType = memType;
	}

	virtual ~IMemoryHandler() {}

	virtual uint8_t Read(uint32_t addr) = 0;
	virtual uint8_t Peek(uint32_t addr) = 0;
	virtual void PeekBlock(uint32_t addr, uint8_t* output) = 0;
	virtual void Write(uint32_t addr, uint8_t value) = 0;

	__forceinline MemoryType GetMemoryType() {
		return _memoryType;
	}

	__forceinline bool HasDirectRead() const { return _directReadPtr != nullptr; }
	__forceinline uint8_t DirectRead(uint32_t addr) const { return _directReadPtr[addr & _directMask]; }
	__forceinline bool HasDirectWrite() const { return _directWritePtr != nullptr; }
	__forceinline void DirectWrite(uint32_t addr, uint8_t value) { _directWritePtr[addr & _directMask] = value; }

	virtual AddressInfo GetAbsoluteAddress(uint32_t address) = 0;
};