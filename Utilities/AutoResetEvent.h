#pragma once
#include "pch.h"

#include <condition_variable>
#include <mutex>

/// <summary>
/// Synchronization primitive that resets automatically after a thread is released.
/// Similar to Windows AutoResetEvent - wakes one waiting thread then resets to non-signaled.
/// </summary>
/// <remarks>
/// Thread synchronization pattern:
/// - Thread A calls Wait() and blocks
/// - Thread B calls Signal()
/// - Thread A wakes up and continues
/// - Event automatically resets to non-signaled state
/// 
/// Use cases:
/// - Producer-consumer queues
/// - Thread wakeup notifications
/// - Frame synchronization between emulation and rendering threads
/// 
/// Implementation uses std::condition_variable + std::mutex.
/// Automatically resets after waking one thread (unlike ManualResetEvent).
/// </remarks>
class AutoResetEvent {
private:
	std::condition_variable _signal; ///< Condition variable for thread wakeup
	std::mutex _mutex;               ///< Mutex protecting signaled state
	bool _signaled;                  ///< Current signal state (true = signaled)

public:
	/// <summary>
	/// Construct AutoResetEvent in non-signaled state.
	/// </summary>
	AutoResetEvent();
	
	/// <summary>
	/// Destructor - wakes all waiting threads.
	/// </summary>
	~AutoResetEvent();

	/// <summary>
	/// Manually reset event to non-signaled state.
	/// </summary>
	/// <remarks>
	/// Usually not needed - event auto-resets after Wait() returns.
	/// Can be used to cancel pending signals before they're consumed.
	/// </remarks>
	void Reset();
	
	/// <summary>
	/// Wait for event to be signaled with optional timeout.
	/// </summary>
	/// <param name="timeoutDelay">Timeout in milliseconds (0 = wait forever)</param>
	/// <returns>True if signaled, false if timed out</returns>
	/// <remarks>
	/// Blocks calling thread until:
	/// - Event is signaled by another thread (returns true)
	/// - Timeout expires (returns false)
	/// 
	/// Event automatically resets to non-signaled after Wait() returns true.
	/// If event is already signaled when Wait() called, returns immediately.
	/// </remarks>
	bool Wait(int timeoutDelay = 0);
	
	/// <summary>
	/// Signal event and wake one waiting thread.
	/// </summary>
	/// <remarks>
	/// If multiple threads waiting, wakes one arbitrary thread.
	/// If no threads waiting, sets signaled flag (next Wait() returns immediately).
	/// Thread-safe - can be called from any thread.
	/// </remarks>
	void Signal();
};
