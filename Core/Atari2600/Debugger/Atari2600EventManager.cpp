#include "pch.h"
#include "Atari2600/Debugger/Atari2600EventManager.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600DefaultVideoFilter.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/Debugger.h"
#include "Debugger/DebugBreakHelper.h"
#include "Debugger/BaseEventManager.h"

Atari2600EventManager::Atari2600EventManager(Debugger* debugger, Atari2600Console* console) {
	_debugger = debugger;
	_console = console;
	_ppuBuffer = std::make_unique<uint16_t[]>(Atari2600Console::ScreenWidth * Atari2600Console::ScreenHeight);
	memset(_ppuBuffer.get(), 0, Atari2600Console::ScreenWidth * Atari2600Console::ScreenHeight * sizeof(uint16_t));
}

Atari2600EventManager::~Atari2600EventManager() = default;

void Atari2600EventManager::AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId) {
	auto& evt = _debugEvents.emplace_back();
	evt.Type = type;
	evt.Flags = static_cast<uint32_t>(EventFlags::ReadWriteOp);
	evt.Operation = operation;
	evt.Scanline = _console->GetCurrentScanline();
	evt.Cycle = static_cast<uint16_t>(_console->GetCurrentColorClock());
	evt.BreakpointId = breakpointId;
	evt.DmaChannel = -1;
	evt.ProgramCounter = _debugger->GetProgramCounter(CpuType::Atari2600, true);
}

void Atari2600EventManager::AddEvent(DebugEventType type) {
	auto& evt = _debugEvents.emplace_back();
	evt.Type = type;
	evt.Scanline = _console->GetCurrentScanline();
	evt.Cycle = static_cast<uint16_t>(_console->GetCurrentColorClock());
	evt.BreakpointId = -1;
	evt.DmaChannel = -1;
	evt.ProgramCounter = _console->GetCpuState().PC;
}

DebugEventInfo Atari2600EventManager::GetEvent(uint16_t y, uint16_t x) {
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

bool Atari2600EventManager::ShowPreviousFrameEvents() {
	return _config.ShowPreviousFrameEvents;
}

void Atari2600EventManager::SetConfiguration(BaseEventViewerConfig& config) {
	_config = (Atari2600EventViewerConfig&)config;
}

EventViewerCategoryCfg Atari2600EventManager::GetEventConfig(DebugEventInfo& evt) {
	switch (evt.Type) {
		default:
			return {};
		case DebugEventType::Breakpoint:
			return _config.MarkedBreakpoints;
		case DebugEventType::Irq:
			return _config.Irq;
		case DebugEventType::Register: {
			bool isWrite = evt.Operation.Type == MemoryOperationType::Write;
			uint16_t addr = static_cast<uint16_t>(evt.Operation.Address) & 0x1FFF;

			// TIA registers: $0000-$007F (mirrored)
			if ((addr & 0x1080) == 0x0000) {
				return isWrite ? _config.TiaWrite : _config.TiaRead;
			}

			// RIOT registers: $0280-$0297
			if ((addr & 0x1080) == 0x0080 && (addr & 0x0200) != 0) {
				return isWrite ? _config.RiotWrite : _config.RiotRead;
			}

			return {};
		}
	}
}

void Atari2600EventManager::ConvertScanlineCycleToRowColumn(int32_t& x, int32_t& y) {
	y *= 2;
	x *= 2;
}

uint32_t Atari2600EventManager::TakeEventSnapshot(bool forAutoRefresh) {
	DebugBreakHelper breakHelper(_debugger);
	auto lock = _lock.AcquireSafe();

	constexpr uint32_t pixelCount = Atari2600Console::ScreenWidth * Atari2600Console::ScreenHeight;
	uint32_t* frameBuffer = _console->GetFrameBuffer();
	if (frameBuffer) {
		memcpy(_ppuBuffer.get(), frameBuffer, pixelCount * sizeof(uint16_t));
	}

	_snapshotCurrentFrame = _debugEvents;
	_snapshotPrevFrame = _prevDebugEvents;
	_snapshotScanline = _console->GetCurrentScanline();
	_snapshotCycle = static_cast<uint16_t>(_console->GetCurrentColorClock());
	_forAutoRefresh = forAutoRefresh;
	_scanlineCount = 262;
	return _scanlineCount;
}

FrameInfo Atari2600EventManager::GetDisplayBufferSize() {
	FrameInfo size;
	size.Width = ScanlineWidth;
	size.Height = _scanlineCount * 2;
	return size;
}

void Atari2600EventManager::DrawScreen(uint32_t* buffer) {
	uint16_t* src = _ppuBuffer.get();
	constexpr uint32_t visibleWidth = Atari2600Console::ScreenWidth;
	constexpr uint32_t visibleHeight = Atari2600Console::ScreenHeight;

	for (uint32_t y = 0; y < visibleHeight * 2 && y < _scanlineCount * 2; y++) {
		for (uint32_t x = 0; x < visibleWidth * 2; x++) {
			int srcOffset = (y >> 1) * visibleWidth + (x >> 1);
			uint16_t paletteIndex = src[srcOffset] & 0x7f;
			uint32_t color = Atari2600DefaultVideoFilter::NtscPalette[paletteIndex];
			buffer[y * ScanlineWidth + x + HblankWidth * 2] = 0xff000000 | color;
		}
	}
}
