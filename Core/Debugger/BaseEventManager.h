#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Shared/SettingTypes.h"
#include "Utilities/SimpleLock.h"
#include "SNES/DmaControllerTypes.h"

/// <summary>
/// Event flags for DebugEventInfo.
/// </summary>
enum class EventFlags {
	PreviousFrame = 1 << 0,      ///< Event from previous frame
	RegFirstWrite = 1 << 1,      ///< First write to double-write register
	RegSecondWrite = 1 << 2,     ///< Second write to double-write register
	WithTargetMemory = 1 << 3,   ///< Event has target memory info (DMA/etc)
	SmsVdpPaletteWrite = 1 << 4, ///< SMS VDP palette write
	ReadWriteOp = 1 << 5,        ///< Memory read/write operation
};

/// <summary>
/// Debug event information for event viewer.
/// </summary>
/// <remarks>
/// Events are recorded during emulation and displayed in event viewer.
/// Includes timing (scanline/cycle), memory operations, DMA transfers.
/// </remarks>
struct DebugEventInfo {
	MemoryOperationInfo Operation;    ///< Memory operation details
	DebugEventType Type;              ///< Event type (register, NMI, IRQ, etc)
	uint32_t ProgramCounter;          ///< CPU PC when event occurred
	int16_t Scanline;                 ///< Scanline when event occurred
	uint16_t Cycle;                   ///< Cycle when event occurred
	int16_t BreakpointId = -1;        ///< Breakpoint ID if triggered
	int8_t DmaChannel = -1;           ///< DMA channel number (-1 if N/A)
	DmaChannelConfig DmaChannelInfo;  ///< DMA channel configuration
	uint32_t Flags;                   ///< EventFlags bitfield
	int32_t RegisterId = -1;          ///< Register ID for register writes
	MemoryOperationInfo TargetMemory; ///< Target memory for DMA/etc
	uint32_t Color = 0;               ///< Display color in event viewer
};

/// <summary>
/// Event viewer category configuration.
/// </summary>
struct EventViewerCategoryCfg {
	bool Visible;   ///< True if category is visible
	uint32_t Color; ///< ARGB color for category
};

/// <summary>
/// Base event viewer configuration (platform-specific classes extend).
/// </summary>
struct BaseEventViewerConfig {
};

/// <summary>
/// Base class for platform-specific event managers (event viewer tool).
/// </summary>
/// <remarks>
/// Architecture:
/// - Records emulation events (register writes, NMI/IRQ, DMA, etc)
/// - Displays events on scanline/cycle grid
/// - Platform-specific classes (NesEventManager, SnesEventManager, etc)
///
/// Event recording:
/// - AddEvent(): Record events during emulation
/// - Events include timing (scanline/cycle), memory ops, DMA info
/// - Two-frame buffer: current frame + previous frame
///
/// Event snapshots:
/// - TakeEventSnapshot(): Create snapshot for rendering
/// - Auto-refresh mode: snapshot at specific scanline/cycle
/// - Manual mode: snapshot on request
///
/// Event rendering:
/// - GetDisplayBuffer(): Render events to ARGB bitmap
/// - DrawEvents(): Draw dots/lines at scanline/cycle positions
/// - Platform-specific coordinate conversion (NTSC/PAL timing)
///
/// Filtering:
/// - FilterEvents(): Apply visibility filters
/// - GetEventConfig(): Get category color/visibility
///
/// Use cases:
/// - Debug register writes (when/where PPU registers changed)
/// - Visualize interrupt timing (NMI/IRQ scanline/cycle)
/// - Analyze DMA transfers (timing and data flow)
/// </remarks>
class BaseEventManager {
protected:
	vector<DebugEventInfo> _debugEvents;     ///< Current frame events
	vector<DebugEventInfo> _prevDebugEvents; ///< Previous frame events
	vector<DebugEventInfo> _sentEvents;      ///< Events sent to client

	vector<DebugEventInfo> _snapshotCurrentFrame; ///< Snapshot of current frame
	vector<DebugEventInfo> _snapshotPrevFrame;    ///< Snapshot of previous frame
	int16_t _snapshotScanline = -1;               ///< Snapshot scanline
	int16_t _snapshotScanlineOffset = 0;          ///< Scanline offset adjustment
	uint16_t _snapshotCycle = 0;                  ///< Snapshot cycle
	bool _forAutoRefresh = false;                 ///< True if auto-refresh mode
	SimpleLock _lock;                             ///< Thread-safety lock

	/// <summary>
	/// Check if previous frame events should be shown (platform-specific).
	/// </summary>
	virtual bool ShowPreviousFrameEvents() = 0;

	/// <summary>
	/// Apply visibility filters to events.
	/// </summary>
	void FilterEvents();

	/// <summary>
	/// Draw a single dot at position with color.
	/// </summary>
	void DrawDot(uint32_t x, uint32_t y, uint32_t color, bool drawBackground, uint32_t* buffer);

	/// <summary>
	/// Get scanline offset for rendering adjustment (platform-specific).
	/// </summary>
	virtual int GetScanlineOffset() { return 0; }

	/// <summary>
	/// Draw horizontal line in buffer.
	/// </summary>
	void DrawLine(uint32_t* buffer, FrameInfo size, uint32_t color, uint32_t row);

	/// <summary>
	/// Draw all events to buffer.
	/// </summary>
	void DrawEvents(uint32_t* buffer, FrameInfo size);

	/// <summary>
	/// Convert scanline/cycle to row/column (platform-specific timing).
	/// </summary>
	virtual void ConvertScanlineCycleToRowColumn(int32_t& x, int32_t& y) = 0;

	/// <summary>
	/// Draw platform-specific screen overlay (platform-specific).
	/// </summary>
	virtual void DrawScreen(uint32_t* buffer) = 0;

	/// <summary>
	/// Draw single event to buffer.
	/// </summary>
	void DrawEvent(DebugEventInfo& evt, bool drawBackground, uint32_t* buffer);

public:
	virtual ~BaseEventManager() {}

	/// <summary>
	/// Set event viewer configuration (platform-specific).
	/// </summary>
	virtual void SetConfiguration(BaseEventViewerConfig& config) = 0;

	/// <summary>
	/// Add event with memory operation.
	/// </summary>
	virtual void AddEvent(DebugEventType type, MemoryOperationInfo& operation, int32_t breakpointId = -1) = 0;

	/// <summary>
	/// Add event without memory operation.
	/// </summary>
	virtual void AddEvent(DebugEventType type) = 0;

	/// <summary>
	/// Get events for transmission to client.
	/// </summary>
	void GetEvents(DebugEventInfo* eventArray, uint32_t& maxEventCount);

	/// <summary>
	/// Get event count.
	/// </summary>
	uint32_t GetEventCount();

	/// <summary>
	/// Clear current frame events.
	/// </summary>
	virtual void ClearFrameEvents();

	/// <summary>
	/// Get event category configuration (visibility/color).
	/// </summary>
	virtual EventViewerCategoryCfg GetEventConfig(DebugEventInfo& evt) = 0;

	/// <summary>
	/// Take snapshot of current events for rendering.
	/// </summary>
	/// <returns>Event count in snapshot</returns>
	virtual uint32_t TakeEventSnapshot(bool forAutoRefresh) = 0;

	/// <summary>
	/// Get display buffer size (platform-specific).
	/// </summary>
	virtual FrameInfo GetDisplayBufferSize() = 0;

	/// <summary>
	/// Get event at scanline/cycle position.
	/// </summary>
	virtual DebugEventInfo GetEvent(uint16_t scanline, uint16_t cycle) = 0;

	/// <summary>
	/// Render events to ARGB display buffer.
	/// </summary>
	void GetDisplayBuffer(uint32_t* buffer, uint32_t bufferSize);
};
