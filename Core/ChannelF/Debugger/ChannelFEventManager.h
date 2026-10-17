#pragma once
#include "pch.h"
#include <memory>
#include "Debugger/DebugTypes.h"
#include "Debugger/BaseEventManager.h"

enum class DebugEventType;
struct DebugEventInfo;
class ChannelFConsole;
class Debugger;

struct ChannelFEventViewerConfig : public BaseEventViewerConfig {
	EventViewerCategoryCfg MarkedBreakpoints;
	EventViewerCategoryCfg IoRead;
	EventViewerCategoryCfg IoWrite;
	bool ShowPreviousFrameEvents;
};

class ChannelFEventManager final : public BaseEventManager {
private:
	static constexpr int ScanlineWidth = 128 * 2;

	ChannelFEventViewerConfig _config = {};
	ChannelFConsole* _console = nullptr;
	Debugger* _debugger = nullptr;
	std::unique_ptr<uint16_t[]> _ppuBuffer;

protected:
	bool ShowPreviousFrameEvents() override;
	void ConvertScanlineCycleToRowColumn(int32_t& x, int32_t& y) override;
	void DrawScreen(uint32_t* buffer) override;

public:
	ChannelFEventManager(Debugger* debugger, ChannelFConsole* console);
	~ChannelFEventManager();

	void AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId = -1) override;
	void AddEvent(DebugEventType type) override;

	EventViewerCategoryCfg GetEventConfig(DebugEventInfo& evt) override;

	uint32_t TakeEventSnapshot(bool forAutoRefresh) override;
	DebugEventInfo GetEvent(uint16_t y, uint16_t x) override;

	FrameInfo GetDisplayBufferSize() override;
	void SetConfiguration(BaseEventViewerConfig& config) override;
};
