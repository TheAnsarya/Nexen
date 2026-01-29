#pragma once
#include "pch.h"

class Emulator;
class DebuggerRequest;
class DebugBreakHelper;

/// <summary>
/// RAII lock for safe emulator state access.
/// Prevents race conditions during emulation thread operations.
/// </summary>
/// <remarks>
/// Use cases:
/// 1. UI thread accessing emulator state
/// 2. Debugger operations
/// 3. Save state operations
/// 4. Cheats/patches
/// 
/// Locking behavior:
/// - Constructor acquires emulator lock (blocks if emulating)
/// - Destructor releases lock (allows emulation to continue)
/// - Optional debugger lock support (prevents debugger from releasing)
/// 
/// Example:
/// <code>
/// {
///     EmulatorLock lock(&emu, false);
///     // Safe to access emulator state here
///     emu->GetConsole()->DoSomething();
/// } // Lock released, emulation resumes
/// </code>
/// 
/// Thread safety: Ensures serialized access to emulator state.
/// WARNING: Never hold lock across message pumps or UI operations (potential deadlock).
/// </remarks>
class EmulatorLock {
private:
	Emulator* _emu = nullptr;               ///< Emulator instance
	unique_ptr<DebuggerRequest> _debugger;  ///< Optional debugger lock
	unique_ptr<DebugBreakHelper> _breakHelper;  ///< Optional debugger break lock

public:
	/// <summary>
	/// Acquire emulator lock (blocks until emulation pauses).
	/// </summary>
	/// <param name="emulator">Emulator instance to lock</param>
	/// <param name="allowDebuggerLock">Allow debugger to break during lock if true</param>
	EmulatorLock(Emulator* emulator, bool allowDebuggerLock);
	
	/// <summary>
	/// Release emulator lock (allows emulation to resume).
	/// </summary>
	~EmulatorLock();
};