#include "pch.h"
#include "ChannelF/Debugger/ChannelFEventManager.h"
#include "ChannelF/ChannelFConsole.h"
#include "ChannelF/ChannelFDefaultVideoFilter.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugBreakHelper.h"
#include "Debugger/BaseEventManager.h"

ChannelFEventManager::ChannelFEventManager(Debugger* debugger, ChannelFConsole* console) {
	_debugger = debugger;
	_console = console;
	_ppuBuffer = std::make_unique<uint16_t[]>(ChannelFConsole::ScreenWidth * ChannelFConsole::ScreenHeight);
	memset(_ppuBuffer.get(), 0, ChannelFConsole::ScreenWidth * ChannelFConsole::ScreenHeight * sizeof(uint16_t));
}

ChannelFEventManager::~ChannelFEventManager() = default;

void ChannelFEventManager::AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId) {
	auto& evt = _debugEvents.emplace_back();
	evt.Type = type;
	evt.Flags = static_cast<uint32_t>(EventFlags::ReadWriteOp);
	evt.Operation = operation;
	evt.Scanline = 0;
	evt.Cycle = 0;
	evt.BreakpointId = breakpointId;
	evt.DmaChannel = -1;
	evt.ProgramCounter = _debugger->GetProgramCounter(CpuType::ChannelF, true);
}

void ChannelFEventManager::AddEvent(DebugEventType type) {
	auto& evt = _debugEvents.emplace_back();
	evt.Type = type;
	evt.Scanline = 0;
	evt.Cycle = 0;
	evt.BreakpointId = -1;
	evt.DmaChannel = -1;
	evt.ProgramCounter = _console->GetCpuState().PC0;
}

DebugEventInfo ChannelFEventManager::GetEvent(uint16_t y, uint16_t x) {
	auto lock = _lock.AcquireSafe();
	for (DebugEventInfo& evt : _sentEvents) {
		if (evt.Cycle == x && evt.Scanline == y) {
			return evt;
		}
	}
	DebugEventInfo empty = {};
	empty.ProgramCounter = 0xffffffff;
	return empty;
}

bool ChannelFEventManager::ShowPreviousFrameEvents() {
	return _config.ShowPreviousFrameEvents;
}

void ChannelFEventManager::SetConfiguration(BaseEventViewerConfig& config) {
	_config = (ChannelFEventViewerConfig&)config;
}

EventViewerCategoryCfg ChannelFEventManager::GetEventConfig(DebugEventInfo& evt) {
	switch (evt.Type) {
		default:
			return {};
		case DebugEventType::Breakpoint:
			return _config.MarkedBreakpoints;
		case DebugEventType::Register: {
			bool isWrite = evt.Operation.Type == MemoryOperationType::Write;
			return isWrite ? _config.IoWrite : _config.IoRead;
		}
	}
}

void ChannelFEventManager::ConvertScanlineCycleToRowColumn(int32_t& x, int32_t& y) {
	y *= 2;
	x *= 2;
}

uint32_t ChannelFEventManager::TakeEventSnapshot(bool forAutoRefresh) {
	DebugBreakHelper breakHelper(_debugger);
	auto lock = _lock.AcquireSafe();

	constexpr uint32_t pixelCount = ChannelFConsole::ScreenWidth * ChannelFConsole::ScreenHeight;
	_console->DebugRenderFrame();

	_snapshotCurrentFrame = _debugEvents;
	_snapshotPrevFrame = _prevDebugEvents;
	_snapshotScanline = 0;
	_snapshotCycle = 0;
	_forAutoRefresh = forAutoRefresh;
	return ChannelFConsole::ScreenHeight;
}

FrameInfo ChannelFEventManager::GetDisplayBufferSize() {
	FrameInfo size;
	size.Width = ScanlineWidth;
	size.Height = ChannelFConsole::ScreenHeight * 2;
	return size;
}

void ChannelFEventManager::DrawScreen(uint32_t* buffer) {
	uint16_t* src = _ppuBuffer.get();
	constexpr uint32_t w = ChannelFConsole::ScreenWidth;
	constexpr uint32_t h = ChannelFConsole::ScreenHeight;

	for (uint32_t y = 0; y < h * 2; y++) {
		for (uint32_t x = 0; x < w * 2; x++) {
			int srcOffset = (y >> 1) * w + (x >> 1);
			uint16_t paletteIndex = src[srcOffset] & 0x03;
			uint32_t color = ChannelFDefaultVideoFilter::ChannelFPalette[paletteIndex];
			buffer[y * ScanlineWidth + x] = color;
		}
	}
}
