#pragma once
#include "pch.h"
#include "Shared/RewindManager.h"

class Emulator;
class IDebugger;

/// <summary>
/// Cached save state entry for step-back.
/// </summary>
struct StepBackCacheEntry {
	stringstream SaveState; ///< Serialized emulator state
	uint64_t Clock;         ///< Master clock at this state
};

/// <summary>
/// Step-back configuration (cycles per scanline/frame).
/// </summary>
struct StepBackConfig {
	uint64_t CurrentCycle;      ///< Current cycle count
	uint32_t CyclesPerScanline; ///< Cycles per scanline
	uint32_t CyclesPerFrame;    ///< Cycles per frame
};

/// <summary>
/// Step-back granularity.
/// </summary>
enum class StepBackType {
	Instruction, ///< Step back one instruction
	Scanline,    ///< Step back one scanline
	Frame        ///< Step back one frame
};

/// <summary>
/// Step-back debugger functionality (rewind with save states).
/// </summary>
/// <remarks>
/// Architecture:
/// - Uses RewindManager's rewind buffer to step backwards
/// - Caches save states for precise instruction-level stepping
/// - Handles NES sprite DMA timing (512-cycle limit)
///
/// Step-back types:
/// - Instruction: Step back 1 instruction (uses state cache)
/// - Scanline: Step back 1 scanline
/// - Frame: Step back 1 frame
///
/// State caching:
/// - Saves states at regular intervals
/// - Default clock limit: 600 cycles (avoids NES sprite DMA ~512 cycles)
/// - Retry mechanism if target not found
///
/// Rewind integration:
/// - CheckStepBack(): Called each instruction to detect target
/// - Loads state from RewindManager if needed
/// - Falls back to RewindManager if cache misses
///
/// Performance:
/// - State cache reduces rewind overhead
/// - Configurable clock limit balances memory vs accuracy
///
/// Use cases:
/// - Step backwards through execution
/// - Debug race conditions (go back to before issue)
/// - Reverse-execute to find root cause
/// </remarks>
class StepBackManager {
private:
	/// <summary>
	/// Default to 600 clocks to avoid retry when NES sprite DMA occurs (~512 cycles).
	/// </summary>
	static constexpr uint64_t DefaultClockLimit = 600;

	Emulator* _emu = nullptr;                ///< Emulator instance
	RewindManager* _rewindManager = nullptr; ///< Rewind manager
	IDebugger* _debugger = nullptr;          ///< Debugger instance

	vector<StepBackCacheEntry> _cache;                              ///< Cached save states
	uint64_t _targetClock = 0;                                      ///< Target clock to step back to
	uint64_t _prevClock = 0;                                        ///< Previous clock value
	bool _active = false;                                           ///< True if step-back in progress
	bool _allowRetry = false;                                       ///< True to retry if target missed
	uint64_t _stateClockLimit = StepBackManager::DefaultClockLimit; ///< State save interval

public:
	/// <summary>
	/// Constructor for step-back manager.
	/// </summary>
	StepBackManager(Emulator* emu, IDebugger* debugger);

	/// <summary>
	/// Initiate step-back operation.
	/// </summary>
	/// <param name="type">Step-back type (instruction/scanline/frame)</param>
	void StepBack(StepBackType type);

	/// <summary>
	/// Check if target clock reached (called each instruction).
	/// </summary>
	/// <returns>True if step-back complete</returns>
	bool CheckStepBack();

	/// <summary>
	/// Reset state cache.
	/// </summary>
	void ResetCache() { _cache.clear(); }

	/// <summary>
	/// Check if rewinding in progress.
	/// </summary>
	bool IsRewinding() { return _active || _rewindManager->IsRewinding(); }
};