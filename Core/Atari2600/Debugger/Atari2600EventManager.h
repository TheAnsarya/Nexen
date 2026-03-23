#pragma once
#include "pch.h"
#include <memory>
#include "Debugger/DebugTypes.h"
#include "Debugger/BaseEventManager.h"

enum class DebugEventType;
struct DebugEventInfo;
class Atari2600Console;
class Debugger;

struct Atari2600EventViewerConfig : public BaseEventViewerConfig {
	EventViewerCategoryCfg Irq;
	EventViewerCategoryCfg MarkedBreakpoints;

	EventViewerCategoryCfg TiaWrite;
	EventViewerCategoryCfg TiaRead;
	EventViewerCategoryCfg RiotWrite;
	EventViewerCategoryCfg RiotRead;

	bool ShowPreviousFrameEvents;
};

class Atari2600EventManager final : public BaseEventManager {
private:
	// 228 color clocks per scanline, 2x for display zoom
	static constexpr int ScanlineWidth = 228 * 2;
	// HBLANK is 68 color clocks before the visible area begins
	static constexpr int HblankWidth = 68;

	Atari2600EventViewerConfig _config = {};
	Atari2600Console* _console = nullptr;
	Debugger* _debugger = nullptr;

	uint32_t _scanlineCount = 262;
	std::unique_ptr<uint16_t[]> _ppuBuffer;

protected:
	bool ShowPreviousFrameEvents() override;
	void ConvertScanlineCycleToRowColumn(int32_t& x, int32_t& y) override;
	void DrawScreen(uint32_t* buffer) override;

public:
	Atari2600EventManager(Debugger* debugger, Atari2600Console* console);
	~Atari2600EventManager();

	void AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId = -1) override;
	void AddEvent(DebugEventType type) override;

	EventViewerCategoryCfg GetEventConfig(DebugEventInfo& evt) override;

	uint32_t TakeEventSnapshot(bool forAutoRefresh) override;
	DebugEventInfo GetEvent(uint16_t y, uint16_t x) override;

	FrameInfo GetDisplayBufferSize() override;
	void SetConfiguration(BaseEventViewerConfig& config) override;
};
