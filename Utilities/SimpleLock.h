#pragma once
#include "pch.h"
#include <thread>

class SimpleLock;

/// <summary>
/// RAII lock guard for SimpleLock - automatically releases on scope exit.
/// Provides exception-safe lock handling with manual early release option.
/// </summary>
/// <remarks>
/// Usage pattern:
/// <code>
/// {
///     auto lock = simpleLock.AcquireSafe();
///     // Critical section code
///     // Lock automatically released when lock goes out of scope
/// }
/// </code>
/// Can manually Release() early if needed (safe to call multiple times).
/// </remarks>
class LockHandler {
private:
	SimpleLock* _lock;      ///< Pointer to owned lock
	bool _released = false; ///< Tracks if lock has been released

public:
	/// <summary>Construct lock handler and acquire lock</summary>
	/// <param name="lock">SimpleLock to manage</param>
	LockHandler(SimpleLock* lock);

	/// <summary>Manually release lock early (safe to call multiple times)</summary>
	void Release();

	/// <summary>Destructor - releases lock if not already released</summary>
	~LockHandler();
};

/// <summary>
/// Recursive mutex implementation using atomic_flag with thread ownership tracking.
/// Supports recursive locking (same thread can acquire multiple times).
/// </summary>
/// <remarks>
/// Advantages over std::recursive_mutex:
/// - AcquireSafe() returns RAII guard for exception safety
/// - IsLockedByCurrentThread() for ownership checks
/// - WaitForRelease() for spin-waiting until lock becomes free
/// - TryAcquire() with timeout support
///
/// Thread-local storage tracks current thread ID for ownership checks.
/// Lock count tracks recursive acquisitions (must release same number of times).
/// </remarks>
class SimpleLock {
private:
	/// <summary>Thread-local storage for current thread ID</summary>
	thread_local static std::thread::id _threadID;

	std::thread::id _holderThreadID; ///< Thread currently holding lock
	uint32_t _lockCount;             ///< Recursive lock count (0 = unlocked)
	atomic_flag _lock;               ///< Atomic flag for lock state

	/// <summary>Spin-wait until lock acquired or timeout</summary>
	/// <param name="msTimeout">Timeout in milliseconds (0 = infinite)</param>
	/// <returns>True if acquired, false if timed out</returns>
	bool WaitForAcquire(uint32_t msTimeout);

public:
	/// <summary>Construct unlocked SimpleLock</summary>
	SimpleLock();

	/// <summary>Destructor - should only be called when lock is free</summary>
	~SimpleLock();

	/// <summary>
	/// Acquire lock and return RAII guard (exception-safe).
	/// </summary>
	/// <returns>LockHandler that releases lock on destruction</returns>
	/// <remarks>
	/// Preferred method for lock acquisition - guarantees release even if exception thrown.
	/// Blocks until lock is acquired (no timeout).
	/// </remarks>
	LockHandler AcquireSafe();

	/// <summary>
	/// Acquire lock (blocks until available).
	/// </summary>
	/// <remarks>
	/// Must manually call Release() - no automatic cleanup.
	/// Prefer AcquireSafe() for exception safety.
	/// Supports recursive locking from same thread.
	/// </remarks>
	void Acquire();

	/// <summary>
	/// Try to acquire lock with timeout.
	/// </summary>
	/// <param name="msTimeout">Timeout in milliseconds (0 = try once and return)</param>
	/// <returns>True if lock acquired, false if timeout elapsed</returns>
	bool TryAcquire(uint32_t msTimeout);

	/// <summary>
	/// Check if lock is currently free (not owned by any thread).
	/// </summary>
	/// <returns>True if lock is free, false if owned</returns>
	bool IsFree();

	/// <summary>
	/// Check if current thread owns the lock.
	/// </summary>
	/// <returns>True if current thread holds lock, false otherwise</returns>
	bool IsLockedByCurrentThread();

	/// <summary>
	/// Spin-wait until lock is released by owning thread.
	/// </summary>
	/// <remarks>
	/// Does NOT acquire lock - just waits until it becomes free.
	/// Useful for synchronization patterns where you need to wait for lock release.
	/// </remarks>
	void WaitForRelease();

	/// <summary>
	/// Release lock (decrement recursive count, free if count reaches 0).
	/// </summary>
	/// <remarks>
	/// Must be called same number of times as Acquire() for recursive locks.
	/// Only owning thread can release lock.
	/// </remarks>
	void Release();
};
