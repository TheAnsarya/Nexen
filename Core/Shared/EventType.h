#pragma once

/// <summary>
/// Event types for notification callbacks and script hooks.
/// Used by NotificationManager to dispatch events to Lua scripts and UI.
/// </summary>
enum class EventType {
	/// <summary>Non-maskable interrupt triggered (NMI - typically VBlank on NES/SNES)</summary>
	Nmi,

	/// <summary>Interrupt request triggered (IRQ - hardware interrupt)</summary>
	Irq,

	/// <summary>Frame rendering started (beginning of new video frame)</summary>
	StartFrame,

	/// <summary>Frame rendering ended (VBlank period starts)</summary>
	/// <remarks>Common hook point for frame-by-frame processing</remarks>
	EndFrame,

	/// <summary>System reset (console power cycled or reset button pressed)</summary>
	Reset,

	/// <summary>Lua script execution completed</summary>
	ScriptEnded,

	/// <summary>Controller input polled by emulated game</summary>
	/// <remarks>Used for input recording and TAS (tool-assisted speedrun) tools</remarks>
	InputPolled,

	/// <summary>Save state loaded from file</summary>
	StateLoaded,

	/// <summary>Save state saved to file</summary>
	StateSaved,

	/// <summary>Code breakpoint hit in debugger</summary>
	CodeBreak,

	/// <summary>Sentinel value for loop iteration (not a real event type)</summary>
	LastValue
};