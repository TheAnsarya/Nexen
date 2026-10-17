#include "pch.h"
#include "Lynx/Debugger/DummyLynxCpu.h"

#define DUMMYCPU
#define LynxCpu DummyLynxCpu
#include "Lynx/LynxCpu.cpp"
#undef LynxCpu
#undef DUMMYCPU

DummyLynxCpu::DummyLynxCpu(Emulator* emu, LynxMemoryManager* memoryManager) {
	_emu = emu;
	_console = nullptr;
	_memoryManager = memoryManager;

	InitOpTable();
}

uint8_t DummyLynxCpu::MemoryRead(uint16_t addr, MemoryOperationType type) {
	uint8_t value = _memoryManager->DebugRead(addr);
	LogMemoryOperation(addr, value, type);
	return value;
}

void DummyLynxCpu::MemoryWrite(uint16_t addr, uint8_t value, MemoryOperationType type) {
	LogMemoryOperation(addr, value, type);
}

void DummyLynxCpu::SetDummyState(LynxCpuState& state) {
	_state = state;
	_memOpCounter = 0;
}

uint32_t DummyLynxCpu::GetOperationCount() {
	return _memOpCounter;
}

void DummyLynxCpu::LogMemoryOperation(uint32_t addr, uint8_t value, MemoryOperationType type) {
	if (_memOpCounter >= 10) { return; }
	_memOperations[_memOpCounter] = { addr, static_cast<int32_t>(value), type, MemoryType::LynxMemory };
	_memOpCounter++;
}

MemoryOperationInfo DummyLynxCpu::GetOperationInfo(uint32_t index) {
	return _memOperations[index];
}

void DummyLynxCpu::Serialize(Serializer& s) {
	// No-op: DummyCpu is not serialized
}
