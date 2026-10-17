#include "pch.h"
#include "Lynx/Debugger/LynxEventManager.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxTypes.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugBreakHelper.h"
#include "Debugger/BaseEventManager.h"

LynxEventManager::LynxEventManager(Debugger* debugger, LynxConsole* console) {
	_debugger = debugger;
	_console = console;
	_cpu = console->GetCpu();
	_mikey = console->GetMikey();

	// Lynx framebuffer is 32-bit ARGB, 160x102
	_ppuBuffer = std::make_unique<uint32_t[]>(LynxConstants::PixelCount);
	memset(_ppuBuffer.get(), 0, LynxConstants::PixelCount * sizeof(uint32_t));
}

LynxEventManager::~LynxEventManager() = default;

void LynxEventManager::AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId) {
	auto& evt = _debugEvents.emplace_back();
	evt.Type = type;
	evt.Flags = static_cast<uint32_t>(EventFlags::ReadWriteOp);
	evt.Operation = operation;
	evt.Scanline = _mikey->GetState().CurrentScanline;
	evt.Cycle = static_cast<uint16_t>(_cpu->GetCycleCount() % LynxConstants::CpuCyclesPerScanline);
	evt.BreakpointId = breakpointId;
	evt.DmaChannel = -1;
	evt.ProgramCounter = _debugger->GetProgramCounter(CpuType::Lynx, true);
}

void LynxEventManager::AddEvent(DebugEventType type) {
	auto& evt = _debugEvents.emplace_back();
	evt.Type = type;
	evt.Scanline = _mikey->GetState().CurrentScanline;
	evt.Cycle = static_cast<uint16_t>(_cpu->GetCycleCount() % LynxConstants::CpuCyclesPerScanline);
	evt.BreakpointId = -1;
	evt.DmaChannel = -1;
	evt.ProgramCounter = _cpu->GetState().PC;
}

DebugEventInfo LynxEventManager::GetEvent(uint16_t y, uint16_t x) {
	auto lock = _lock.AcquireSafe();

	uint16_t cycle = x / 2;
	uint16_t scanline = y / 2;

	for (DebugEventInfo& evt : _sentEvents) {
		if (evt.Cycle == cycle && evt.Scanline == scanline) {
			return evt;
		}
	}

	for (int i = static_cast<int>(_sentEvents.size()) - 1; i >= 0; i--) {
		DebugEventInfo& evt = _sentEvents[i];
		if (std::abs(static_cast<int>(evt.Cycle) - static_cast<int>(cycle)) <= 1 && std::abs(static_cast<int>(evt.Scanline) - static_cast<int>(scanline)) <= 1) {
			return evt;
		}
	}

	DebugEventInfo empty = {};
	empty.ProgramCounter = 0xFFFFFFFF;
	return empty;
}

bool LynxEventManager::ShowPreviousFrameEvents() {
	return _config.ShowPreviousFrameEvents;
}

void LynxEventManager::SetConfiguration(BaseEventViewerConfig& config) {
	_config = (LynxEventViewerConfig&)config;
}

EventViewerCategoryCfg LynxEventManager::GetEventConfig(DebugEventInfo& evt) {
	switch (evt.Type) {
		default:
			return {};
		case DebugEventType::Breakpoint:
			return _config.MarkedBreakpoints;
		case DebugEventType::Irq:
			return _config.Irq;
		case DebugEventType::Register: {
			bool isWrite = evt.Operation.Type == MemoryOperationType::Write;
			uint16_t addr = static_cast<uint16_t>(evt.Operation.Address);

			// Mikey registers: $FD00-$FDFF
			if (addr >= LynxConstants::MikeyBase && addr <= LynxConstants::MikeyEnd) {
				uint8_t reg = addr & 0xFF;

				// Timer registers: $FD00-$FD1F
				if (reg < 0x20) {
					return isWrite ? _config.TimerWrite : _config.TimerRead;
				}
				// Audio registers: $FD20-$FD4F
				if (reg < 0x50) {
					return isWrite ? _config.AudioRegisterWrite : _config.AudioRegisterRead;
				}
				// Palette: $FDA0-$FDBF
				if (reg >= 0xa0 && reg <= 0xbf) {
					return isWrite ? _config.PaletteWrite : EventViewerCategoryCfg{};
				}
				// Other Mikey registers
				return isWrite ? _config.MikeyRegisterWrite : _config.MikeyRegisterRead;
			}

			// Suzy registers: $FC00-$FCFF
			if (addr >= LynxConstants::SuzyBase && addr <= LynxConstants::SuzyEnd) {
				return isWrite ? _config.SuzyRegisterWrite : _config.SuzyRegisterRead;
			}

			return {};
		}
	}
}

void LynxEventManager::ConvertScanlineCycleToRowColumn(int32_t& x, int32_t& y) {
	y *= 2;
	x *= 2;
}

uint32_t LynxEventManager::TakeEventSnapshot(bool forAutoRefresh) {
	DebugBreakHelper breakHelper(_debugger);
	auto lock = _lock.AcquireSafe();

	uint16_t scanline = _mikey->GetState().CurrentScanline;

	// Copy the Lynx framebuffer (already 32-bit ARGB)
	uint32_t* fb = _console->GetFrameBuffer();
	if (fb) {
		memcpy(_ppuBuffer.get(), fb, LynxConstants::PixelCount * sizeof(uint32_t));
	}

	_snapshotCurrentFrame = _debugEvents;
	_snapshotPrevFrame = _prevDebugEvents;
	_snapshotScanline = scanline;
	_snapshotCycle = static_cast<uint16_t>(_cpu->GetCycleCount() % LynxConstants::CpuCyclesPerScanline);
	_forAutoRefresh = forAutoRefresh;
	_scanlineCount = LynxConstants::ScanlineCount;
	return _scanlineCount;
}

FrameInfo LynxEventManager::GetDisplayBufferSize() {
	FrameInfo size;
	size.Width = ScanlineWidth;
	size.Height = _scanlineCount * 2;
	return size;
}

void LynxEventManager::DrawScreen(uint32_t* buffer) {
	uint32_t* src = _ppuBuffer.get();
	uint32_t visibleWidth = LynxConstants::ScreenWidth;
	uint32_t visibleHeight = LynxConstants::ScreenHeight;

	for (uint32_t y = 0; y < visibleHeight * 2; y++) {
		for (uint32_t x = 0; x < visibleWidth * 2; x++) {
			int srcOffset = (y >> 1) * visibleWidth + (x >> 1);
			buffer[y * ScanlineWidth + x] = src[srcOffset];
		}
	}
}
