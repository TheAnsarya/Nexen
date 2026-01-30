#pragma once
#include "pch.h"

class Debugger;
class Emulator;

/// <summary>
/// RAII guard for safe debugger access.
/// Ensures debugger pointer remains valid during usage and cleans up on scope exit.
/// </summary>
/// <remarks>
/// The debugger can be deleted asynchronously (e.g., when loading a new game).
/// This class uses shared_ptr to keep the debugger alive during the request.
///
/// Usage pattern:
/// <code>
/// DebuggerRequest req(emulator);
/// if (Debugger* dbg = req.GetDebugger()) {
///     // Debugger is guaranteed valid within this scope
///     dbg->Step();
/// }
/// // Debugger automatically released on scope exit
/// </code>
///
/// Thread safety: Copying the request shares ownership of the debugger.
/// </remarks>
class DebuggerRequest {
private:
	shared_ptr<Debugger> _debugger; ///< Shared debugger ownership (keeps alive)
	Emulator* _emu = nullptr;       ///< Owning emulator instance

public:
	/// <summary>
	/// Create debugger request for emulator instance.
	/// </summary>
	/// <param name="emu">Emulator to get debugger from</param>
	/// <remarks>
	/// Acquires shared ownership of debugger if one exists.
	/// If no debugger, GetDebugger() will return nullptr.
	/// </remarks>
	DebuggerRequest(Emulator* emu);

	/// <summary>Copy constructor - shares debugger ownership</summary>
	DebuggerRequest(const DebuggerRequest& copy);

	/// <summary>Destructor - releases debugger reference</summary>
	~DebuggerRequest();

	/// <summary>
	/// Get raw debugger pointer (valid only during request lifetime).
	/// </summary>
	/// <returns>Debugger pointer, or nullptr if no debugger active</returns>
	/// <remarks>
	/// Returned pointer is only valid while this DebuggerRequest exists.
	/// Always check for nullptr before use.
	/// </remarks>
	Debugger* GetDebugger() { return _debugger.get(); }
};
