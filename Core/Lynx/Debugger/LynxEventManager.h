#pragma once
#include "pch.h"
#include <memory>
#include "Debugger/DebugTypes.h"
#include "Debugger/BaseEventManager.h"
#include "Lynx/LynxTypes.h"

enum class DebugEventType;
struct DebugEventInfo;
class LynxConsole;
class LynxCpu;
class LynxMikey;
class Debugger;

struct LynxEventViewerConfig : public BaseEventViewerConfig {
	EventViewerCategoryCfg Irq;
	EventViewerCategoryCfg MarkedBreakpoints;

	EventViewerCategoryCfg MikeyRegisterWrite;
	EventViewerCategoryCfg MikeyRegisterRead;
	EventViewerCategoryCfg SuzyRegisterWrite;
	EventViewerCategoryCfg SuzyRegisterRead;
	EventViewerCategoryCfg AudioRegisterWrite;
	EventViewerCategoryCfg AudioRegisterRead;
	EventViewerCategoryCfg PaletteWrite;
	EventViewerCategoryCfg TimerWrite;
	EventViewerCategoryCfg TimerRead;

	bool ShowPreviousFrameEvents;
};

class LynxEventManager final : public BaseEventManager {
private:
	/// <summary>Display width for event viewer (CpuCyclesPerScanline * 2 for 2x zoom)</summary>
	static constexpr int ScanlineWidth = LynxConstants::CpuCyclesPerScanline * 2;

	LynxEventViewerConfig _config = {};
	LynxConsole* _console = nullptr;
	LynxCpu* _cpu = nullptr;
	LynxMikey* _mikey = nullptr;
	Debugger* _debugger = nullptr;

	uint32_t _scanlineCount = LynxConstants::ScanlineCount;
	std::unique_ptr<uint32_t[]> _ppuBuffer;

protected:
	bool ShowPreviousFrameEvents() override;
	void ConvertScanlineCycleToRowColumn(int32_t& x, int32_t& y) override;
	void DrawScreen(uint32_t* buffer) override;

public:
	LynxEventManager(Debugger* debugger, LynxConsole* console);
	~LynxEventManager();

	void AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId = -1) override;
	void AddEvent(DebugEventType type) override;

	EventViewerCategoryCfg GetEventConfig(DebugEventInfo& evt) override;

	uint32_t TakeEventSnapshot(bool forAutoRefresh) override;
	DebugEventInfo GetEvent(uint16_t y, uint16_t x) override;

	FrameInfo GetDisplayBufferSize() override;
	void SetConfiguration(BaseEventViewerConfig& config) override;
};
