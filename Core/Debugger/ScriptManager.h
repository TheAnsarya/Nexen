#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/ScriptHost.h"
#include "Utilities/SimpleLock.h"
#include "Shared/EventType.h"

class Debugger;
enum class MemoryOperationType;

/// <summary>
/// Manages Lua scripts for debugger automation.
/// </summary>
/// <remarks>
/// Purpose:
/// - Load and execute Lua scripts in debugger
/// - Provide scripting API for automation
/// - Hook into emulator events (frame, scanline, memory access)
/// - Enable advanced debugging workflows
///
/// Script capabilities:
/// - Memory read/write hooks
/// - Breakpoint control
/// - Save state manipulation
/// - Event callbacks (frame end, scanline, etc.)
/// - Emulator control (pause, resume, reset)
///
/// Script lifecycle:
/// 1. LoadScript(): Create ScriptHost, compile Lua
/// 2. Script runs in separate Lua state
/// 3. Callbacks invoked on events (memory access, frame, etc.)
/// 4. RemoveScript(): Clean up and unload
///
/// Memory callbacks:
/// - ProcessMemoryOperation(): Called on every memory access
/// - _isCpuMemoryCallbackEnabled: Enable CPU memory hooks
/// - _isPpuMemoryCallbackEnabled: Enable PPU memory hooks
/// - Template for compile-time optimization (1/2/4 byte access)
///
/// Performance:
/// - __forceinline HasScript() for hot path checks
/// - Template ProcessMemoryOperation<T>() for access width
/// - Scripts only invoked if callbacks registered
/// - Lock-protected script list (_scriptLock)
///
/// Use cases:
/// - Automated testing (script runs game, validates state)
/// - TAS creation (record inputs based on game state)
/// - Cheat finding (scan memory for specific values)
/// - Reverse engineering (log memory access patterns)
/// - Bot programming (AI plays game)
/// </remarks>
class ScriptManager {
private:
	Debugger* _debugger = nullptr;            ///< Main debugger instance
	bool _hasScript = false;                  ///< True if any scripts loaded
	SimpleLock _scriptLock;                   ///< Script list access lock
	int _nextScriptId = 0;                    ///< Next script ID counter
	bool _isCpuMemoryCallbackEnabled = false; ///< True if any script has CPU memory callbacks
	bool _isPpuMemoryCallbackEnabled = false; ///< True if any script has PPU memory callbacks
	vector<unique_ptr<ScriptHost>> _scripts;  ///< Active script instances

	/// <summary>
	/// Refresh memory callback flags based on loaded scripts.
	/// </summary>
	void RefreshMemoryCallbackFlags();

public:
	/// <summary>
	/// Constructor for script manager.
	/// </summary>
	/// <param name="debugger">Main debugger instance</param>
	ScriptManager(Debugger* debugger);

	/// <summary>
	/// Destructor - unload all scripts.
	/// </summary>
	~ScriptManager();

	/// <summary>
	/// Check if any scripts are loaded (hot path).
	/// </summary>
	/// <returns>True if scripts loaded</returns>
	/// <remarks>
	/// Inline for performance - called frequently to check if callbacks needed.
	/// </remarks>
	__forceinline bool HasScript() { return _hasScript; }

	/// <summary>
	/// Load and execute Lua script.
	/// </summary>
	/// <param name="name">Script name</param>
	/// <param name="path">Script file path</param>
	/// <param name="content">Script source code</param>
	/// <param name="scriptId">Script ID (0 for new script, >0 to replace)</param>
	/// <returns>Script ID</returns>
	int32_t LoadScript(string name, string path, string content, int32_t scriptId);

	/// <summary>
	/// Remove and unload script.
	/// </summary>
	/// <param name="scriptId">Script ID to remove</param>
	void RemoveScript(int32_t scriptId);

	/// <summary>
	/// Get script output log.
	/// </summary>
	/// <param name="scriptId">Script ID</param>
	/// <returns>Script log text</returns>
	string GetScriptLog(int32_t scriptId);

	/// <summary>
	/// Process emulator event for scripts.
	/// </summary>
	/// <param name="type">Event type (frame end, scanline, etc.)</param>
	/// <param name="cpuType">CPU type</param>
	void ProcessEvent(EventType type, CpuType cpuType);

	/// <summary>
	/// Enable CPU memory callbacks.
	/// </summary>
	void EnableCpuMemoryCallbacks() { _isCpuMemoryCallbackEnabled = true; }

	/// <summary>
	/// Check if CPU memory callbacks enabled.
	/// </summary>
	/// <returns>True if callbacks enabled</returns>
	bool HasCpuMemoryCallbacks() { return _scripts.size() && _isCpuMemoryCallbackEnabled; }

	/// <summary>
	/// Enable PPU memory callbacks.
	/// </summary>
	void EnablePpuMemoryCallbacks() { _isPpuMemoryCallbackEnabled = true; }

	/// <summary>
	/// Check if PPU memory callbacks enabled.
	/// </summary>
	/// <returns>True if callbacks enabled</returns>
	bool HasPpuMemoryCallbacks() { return _scripts.size() && _isPpuMemoryCallbackEnabled; }

	/// <summary>
	/// Process memory operation for script callbacks.
	/// </summary>
	/// <typeparam name="T">Access type (uint8_t, uint16_t, uint32_t)</typeparam>
	/// <param name="relAddr">Relative address info</param>
	/// <param name="value">Memory value</param>
	/// <param name="type">Operation type (read, write, exec)</param>
	/// <param name="cpuType">CPU type</param>
	/// <param name="processExec">True to process exec callbacks</param>
	/// <remarks>
	/// Inline for performance (called on every memory access).
	/// Invokes script callbacks based on operation type:
	/// - Read: Read, DmaRead, PpuRenderingRead, DummyRead
	/// - Write: Write, DummyWrite, DmaWrite
	/// - Exec: ExecOpCode, ExecOperand (if processExec=true)
	/// </remarks>
	template <typename T>
	__forceinline void ProcessMemoryOperation(AddressInfo relAddr, T& value, MemoryOperationType type, CpuType cpuType, bool processExec) {
		switch (type) {
			case MemoryOperationType::Read:
			case MemoryOperationType::DmaRead:
			case MemoryOperationType::PpuRenderingRead:
			case MemoryOperationType::DummyRead:
				for (unique_ptr<ScriptHost>& script : _scripts) {
					script->CallMemoryCallback(relAddr, value, CallbackType::Read, cpuType);
				}
				break;

			case MemoryOperationType::Write:
			case MemoryOperationType::DummyWrite:
			case MemoryOperationType::DmaWrite:
				for (unique_ptr<ScriptHost>& script : _scripts) {
					script->CallMemoryCallback(relAddr, value, CallbackType::Write, cpuType);
				}
				break;

			case MemoryOperationType::ExecOpCode:
			case MemoryOperationType::ExecOperand:
				if (processExec) {
					for (unique_ptr<ScriptHost>& script : _scripts) {
						script->CallMemoryCallback(relAddr, value, CallbackType::Exec, cpuType);
					}
				}
				break;

			default:
				break;
		}
	}
};