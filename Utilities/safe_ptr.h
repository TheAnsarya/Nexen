#pragma once
#include "pch.h"
#include "Utilities/SimpleLock.h"

/// <summary>
/// Thread-safe smart pointer wrapper providing lockable access to shared_ptr.
/// Combines raw pointer for fast access with shared_ptr for lifetime management.
/// </summary>
/// <typeparam name="T">Managed object type</typeparam>
/// <remarks>
/// Design pattern: Dual-pointer approach for performance vs safety tradeoff.
///
/// _ptr: Fast but NOT thread-safe (use for operator->, get(), bool checks)
/// _shared: Thread-safe via lock() method (use for cross-thread access)
///
/// Common use case: Emulator core objects accessed from multiple threads.
/// Main thread uses fast _ptr access, background threads use safe lock() access.
///
/// WARNING: operator-> and get() are NOT thread-safe for performance.
/// Use lock() to obtain thread-safe shared_ptr when crossing thread boundaries.
/// </remarks>
template <typename T>
class safe_ptr {
private:
	T* _ptr = nullptr;     ///< Raw pointer for fast access (NOT thread-safe)
	shared_ptr<T> _shared; ///< Shared pointer for lifetime management
	SimpleLock _lock;      ///< Lock protecting shared_ptr access

public:
	/// <summary>
	/// Construct safe_ptr from raw pointer (takes ownership).
	/// </summary>
	/// <param name="ptr">Pointer to take ownership of (will be deleted via shared_ptr)</param>
	safe_ptr(T* ptr = nullptr) {
		reset(ptr);
	}

	/// <summary>
	/// Destructor - shared_ptr handles deallocation automatically.
	/// </summary>
	~safe_ptr() {
		// shared_ptr will free the instance as needed
	}

	/// <summary>
	/// Obtain thread-safe shared_ptr copy with lock protection.
	/// </summary>
	/// <returns>Shared pointer copy if valid, nullptr if reset to null</returns>
	/// <remarks>
	/// USE THIS METHOD for cross-thread access - it's thread-safe.
	/// Lock is only taken if _ptr is non-null (optimization).
	/// Returned shared_ptr keeps object alive until all copies released.
	/// </remarks>
	shared_ptr<T> lock() {
		if (_ptr) {
			// Only need to take the lock if _ptr is still set
			// Otherwise the shared_ptr is empty and there's no need to copy it
			auto lock = _lock.AcquireSafe();
			return _shared;
		} else {
			return nullptr;
		}
	}

	/// <summary>
	/// Reset to new raw pointer (takes ownership).
	/// </summary>
	/// <param name="ptr">New pointer to manage, or nullptr to release</param>
	/// <remarks>
	/// Thread-safe: Acquires lock before modifying pointers.
	/// Old object will be deleted when last shared_ptr copy is destroyed.
	/// </remarks>
	void reset(T* ptr = nullptr) {
		auto lock = _lock.AcquireSafe();
		_ptr = ptr;
		_shared.reset(ptr);
	}

	/// <summary>
	/// Reset to new shared_ptr (shares ownership).
	/// </summary>
	/// <param name="ptr">Shared pointer to copy from</param>
	/// <remarks>
	/// Thread-safe: Acquires lock before modifying pointers.
	/// Reference count is incremented - object lifetime shared with other shared_ptr owners.
	/// </remarks>
	void reset(shared_ptr<T> ptr) {
		auto lock = _lock.AcquireSafe();
		_ptr = ptr.get();
		_shared = ptr;
	}

	/// <summary>
	/// Reset to unique_ptr (transfers ownership).
	/// </summary>
	/// <param name="ptr">Unique pointer to transfer from (will be released)</param>
	/// <remarks>
	/// Thread-safe: Acquires lock before modifying pointers.
	/// unique_ptr is released after transfer (caller loses ownership).
	/// </remarks>
	void reset(unique_ptr<T>& ptr) {
		auto lock = _lock.AcquireSafe();
		_ptr = ptr.get();
		_shared.reset(ptr.get());
		ptr.release();
	}

	/// <summary>
	/// Get raw pointer for fast access.
	/// </summary>
	/// <returns>Raw pointer, or nullptr if unset</returns>
	/// <remarks>
	/// WARNING: NOT THREAD-SAFE - use only from single thread.
	/// For cross-thread access, use lock() instead.
	/// Fast access path with zero synchronization overhead.
	/// </remarks>
	T* get() {
		return _ptr;
	}

	/// <summary>
	/// Boolean conversion operator (checks if pointer is non-null).
	/// </summary>
	/// <returns>True if pointer is valid, false if nullptr</returns>
	/// <remarks>
	/// NOT THREAD-SAFE - reads _ptr without lock for performance.
	/// Allows usage: if (safe_ptr) { ... }
	/// </remarks>
	operator bool() const {
		return _ptr != nullptr;
	}

	/// <summary>
	/// Dereference operator for member access.
	/// </summary>
	/// <returns>Raw pointer for member access</returns>
	/// <remarks>
	/// WARNING: NOT THREAD-SAFE - use only from single thread.
	/// Fast access path with zero synchronization overhead.
	/// Allows usage: safe_ptr->Method()
	/// Undefined behavior if pointer is nullptr (no null check).
	/// </remarks>
	T* operator->() const {
		// Accessing the pointer this way is fast, but not thread-safe
		return _ptr;
	}
};

/// <summary>
/// Equality comparison with nullptr.
/// </summary>
/// <typeparam name="T">Managed object type</typeparam>
/// <param name="ptr">safe_ptr to compare</param>
/// <returns>True if pointer is null</returns>
template <typename T>
bool operator==(const safe_ptr<T>& ptr, std::nullptr_t) {
	return !(bool)ptr;
}

/// <summary>
/// Inequality comparison with nullptr.
/// </summary>
/// <typeparam name="T">Managed object type</typeparam>
/// <param name="ptr">safe_ptr to compare</param>
/// <returns>True if pointer is non-null</returns>
template <typename T>
bool operator!=(const safe_ptr<T>& ptr, std::nullptr_t) {
	return (bool)ptr;
}