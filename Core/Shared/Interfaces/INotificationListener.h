#pragma once
#include "pch.h"

enum class EmulatorShortcut;

/// <summary>
/// Console notification event types for inter-component communication.
/// </summary>
/// <remarks>
/// Event flow:
/// - Console generates events (game loaded, frame done, etc.)
/// - Emulator broadcasts to registered listeners
/// - Listeners process events (UI updates, debugger sync, etc.)
/// 
/// Listener types:
/// - Debugger: CodeBreak, PpuFrameDone, StateLoaded
/// - UI: GameLoaded, ResolutionChanged, ConfigChanged
/// - Video recording: PpuFrameDone
/// - Network play: GameLoaded, StateLoaded, GameReset
/// </remarks>
enum class ConsoleNotificationType {
	GameLoaded,            ///< ROM loaded successfully (param: GameLoadedEventParams*)
	StateLoaded,           ///< Save state loaded
	GameReset,             ///< Console reset button pressed
	GamePaused,            ///< Emulation paused
	GameResumed,           ///< Emulation resumed
	CodeBreak,             ///< Debugger breakpoint hit
	DebuggerResumed,       ///< Debugger continue execution
	PpuFrameDone,          ///< PPU frame rendering complete
	ResolutionChanged,     ///< Video resolution changed (GB window resizing, SNES mode change)
	ConfigChanged,         ///< Settings modified
	ExecuteShortcut,       ///< Hotkey pressed (param: ExecuteShortcutParams*)
	ReleaseShortcut,       ///< Hotkey released
	EmulationStopped,      ///< Emulation stopped (ROM unloaded)
	BeforeEmulationStop,   ///< About to stop (save battery, cleanup)
	ViewerRefresh,         ///< Debugger memory viewer refresh request
	EventViewerRefresh,    ///< Debugger event viewer refresh
	MissingFirmware,       ///< Required firmware/BIOS file missing
	SufamiTurboFilePrompt, ///< Sufami Turbo ROM selection prompt
	BeforeGameUnload,      ///< About to unload game (cleanup)
	BeforeGameLoad,        ///< About to load game (pre-init)
	GameLoadFailed,        ///< ROM loading failed
	CheatsChanged,         ///< Cheat codes modified
	RequestConfigChange,   ///< Request settings change (param: config)
	RefreshSoftwareRenderer, ///< Software renderer needs refresh
};

/// <summary>
/// Parameters for GameLoaded event.
/// </summary>
struct GameLoadedEventParams {
	bool IsPaused;      ///< True if emulation paused after load
	bool IsPowerCycle;  ///< True if power cycle, false if soft reset
};

/// <summary>
/// Interface for console event notification listeners.
/// </summary>
/// <remarks>
/// Implementers:
/// - Emulator: Main emulation coordinator
/// - Debugger: Debug event processing
/// - GameServer/GameClient: Netplay synchronization
/// - VideoRecorder: Frame capture triggers
/// - NotificationManager: UI notification display
/// 
/// Thread model:
/// - ProcessNotification() called from emulation thread
/// - Listeners must be thread-safe if accessing shared state
/// - Avoid blocking operations (defer to worker threads)
/// </remarks>
class INotificationListener {
public:
	/// <summary>
	/// Process console notification event.
	/// </summary>
	/// <param name="type">Event type</param>
	/// <param name="parameter">Type-specific parameter (may be nullptr)</param>
	/// <remarks>
	/// Parameter types by event:
	/// - GameLoaded: GameLoadedEventParams*
	/// - ExecuteShortcut: ExecuteShortcutParams*
	/// - Others: nullptr or event-specific struct
	/// 
	/// Performance considerations:
	/// - Called frequently (PpuFrameDone = 60 FPS)
	/// - Keep processing lightweight
	/// - Defer heavy work to separate threads
	/// </remarks>
	virtual void ProcessNotification(ConsoleNotificationType type, void* parameter) = 0;
};

/// <summary>
/// Parameters for shortcut execution events.
/// </summary>
struct ExecuteShortcutParams {
	EmulatorShortcut Shortcut;  ///< Shortcut ID (save state, reset, etc.)
	uint32_t Param;             ///< Numeric parameter (e.g., state slot number)
	void* ParamPtr;             ///< Pointer parameter (optional)
};