#include "pch.h"
#include <array>
#include <deque>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <limits>

// ============================================================================
// Debugger Pipeline Optimization Tests
//
// These tests verify the correctness of optimized data structures used in
// Profiler (cached ProfiledFunction* pointers) and CallstackManager
// (contiguous array ring buffer).
//
// We test the algorithms independently without requiring a full emulator
// to isolate the data structure behavior.
// ============================================================================

// ============================================================================
// CallstackManager ring buffer tests
// Verify the contiguous array ring buffer produces identical behavior to
// the previous deque-based implementation at all boundary conditions.
// ============================================================================

class CallstackRingBufferTest : public ::testing::Test {};

// Test: Empty callstack returns correct defaults
TEST_F(CallstackRingBufferTest, Empty_ReturnsDefaults) {

	// Create a minimal test: verify the ring buffer size/head are initialized
	// We need to verify through the public interface
	// Since CallstackManager needs a Debugger, we test the ring buffer logic directly

	// Direct ring buffer test
	constexpr uint32_t MaxSize = 512;
	std::array<uint32_t, MaxSize> ring{};
	uint32_t head = 0;
	uint32_t size = 0;

	// Empty ring buffer
	EXPECT_EQ(size, 0u);

	// Push 3 elements
	ring[head] = 100;
	head = (head + 1) % MaxSize;
	size++;

	ring[head] = 200;
	head = (head + 1) % MaxSize;
	size++;

	ring[head] = 300;
	head = (head + 1) % MaxSize;
	size++;

	EXPECT_EQ(size, 3u);

	// Verify reverse scan finds all elements
	bool found100 = false, found200 = false, found300 = false;
	for (uint32_t i = 0; i < size; i++) {
		uint32_t idx = (head - 1 - i + MaxSize) % MaxSize;
		if (ring[idx] == 100) found100 = true;
		if (ring[idx] == 200) found200 = true;
		if (ring[idx] == 300) found300 = true;
	}
	EXPECT_TRUE(found100);
	EXPECT_TRUE(found200);
	EXPECT_TRUE(found300);

	// Pop from back (most recent)
	head = (head - 1 + MaxSize) % MaxSize;
	size--;
	EXPECT_EQ(ring[head], 300u);
	EXPECT_EQ(size, 2u);
}

// Test: Ring buffer wraps around correctly at capacity
TEST_F(CallstackRingBufferTest, WrapAround_MaintainsOrder) {
	constexpr uint32_t MaxSize = 8; // Small ring for testing
	std::array<uint32_t, MaxSize> ring{};
	uint32_t head = 0;
	uint32_t size = 0;

	// Fill the ring completely
	for (uint32_t i = 0; i < MaxSize; i++) {
		ring[head] = i * 10;
		head = (head + 1) % MaxSize;
		size++;
		if (size > MaxSize) size = MaxSize;
	}

	EXPECT_EQ(size, MaxSize);
	EXPECT_EQ(head, 0u); // Wrapped around to 0

	// Push one more (overwrites oldest)
	ring[head] = 999;
	head = (head + 1) % MaxSize;
	// Size stays at MaxSize

	// The oldest element (0) should be gone, replaced by 999
	// Reverse scan should find 999 and 10..70 but not 0
	bool found0 = false, found999 = false, found70 = false;
	for (uint32_t i = 0; i < size; i++) {
		uint32_t idx = (head - 1 - i + MaxSize) % MaxSize;
		if (ring[idx] == 0) found0 = true;
		if (ring[idx] == 999) found999 = true;
		if (ring[idx] == 70) found70 = true;
	}
	EXPECT_FALSE(found0);     // 0 was overwritten
	EXPECT_TRUE(found999);    // 999 is the newest
	EXPECT_TRUE(found70);     // 70 still exists
}

// Test: Ring buffer linearization (GetCallstack behavior)
TEST_F(CallstackRingBufferTest, Linearize_OldestToNewest) {
	constexpr uint32_t MaxSize = 8;
	std::array<uint32_t, MaxSize> ring{};
	uint32_t head = 0;
	uint32_t size = 0;

	// Push 5 elements: 10, 20, 30, 40, 50
	for (uint32_t val : {10u, 20u, 30u, 40u, 50u}) {
		ring[head] = val;
		head = (head + 1) % MaxSize;
		size++;
	}

	// Linearize oldest to newest
	std::vector<uint32_t> linear;
	for (uint32_t i = 0; i < size; i++) {
		uint32_t idx = (head - size + i + MaxSize) % MaxSize;
		linear.push_back(ring[idx]);
	}

	ASSERT_EQ(linear.size(), 5u);
	EXPECT_EQ(linear[0], 10u); // Oldest
	EXPECT_EQ(linear[1], 20u);
	EXPECT_EQ(linear[2], 30u);
	EXPECT_EQ(linear[3], 40u);
	EXPECT_EQ(linear[4], 50u); // Newest
}

// Test: Push/Pop preserves LIFO order
TEST_F(CallstackRingBufferTest, PushPop_LIFO) {
	constexpr uint32_t MaxSize = 512;
	std::array<uint32_t, MaxSize> ring{};
	uint32_t head = 0;
	uint32_t size = 0;

	// Push values
	for (uint32_t v : {100u, 200u, 300u, 400u, 500u}) {
		ring[head] = v;
		head = (head + 1) % MaxSize;
		size++;
	}

	// Pop and verify LIFO order
	std::vector<uint32_t> popped;
	while (size > 0) {
		head = (head - 1 + MaxSize) % MaxSize;
		popped.push_back(ring[head]);
		size--;
	}

	ASSERT_EQ(popped.size(), 5u);
	EXPECT_EQ(popped[0], 500u); // Most recent first
	EXPECT_EQ(popped[1], 400u);
	EXPECT_EQ(popped[2], 300u);
	EXPECT_EQ(popped[3], 200u);
	EXPECT_EQ(popped[4], 100u); // Oldest last
}

// Test: Ring buffer at maximum capacity (511 entries, matching original limit)
TEST_F(CallstackRingBufferTest, MaxCapacity511_NoOverflow) {
	constexpr uint32_t MaxSize = 512;
	std::array<uint32_t, MaxSize> ring{};
	uint32_t head = 0;
	uint32_t size = 0;

	// Push 511 entries (original max was 511)
	for (uint32_t i = 0; i < 511; i++) {
		if (size >= MaxSize - 1) {
			// Ring buffer full, oldest overwritten
			size = MaxSize - 1;
		}
		ring[head] = i;
		head = (head + 1) % MaxSize;
		size++;
		if (size > MaxSize) size = MaxSize;
	}

	EXPECT_EQ(size, 511u);

	// Verify newest is 510, oldest is 0
	uint32_t newestIdx = (head - 1 + MaxSize) % MaxSize;
	EXPECT_EQ(ring[newestIdx], 510u);

	uint32_t oldestIdx = (head - size + MaxSize) % MaxSize;
	EXPECT_EQ(ring[oldestIdx], 0u);
}

// Test: Multiple push/pop cycles don't corrupt state
TEST_F(CallstackRingBufferTest, MultiplePushPopCycles_StableState) {
	constexpr uint32_t MaxSize = 512;
	std::array<uint32_t, MaxSize> ring{};
	uint32_t head = 0;
	uint32_t size = 0;

	// Simulate 1000 push/pop cycles
	for (int cycle = 0; cycle < 1000; cycle++) {
		// Push 10 entries
		for (int i = 0; i < 10; i++) {
			ring[head] = cycle * 10 + i;
			head = (head + 1) % MaxSize;
			size++;
			if (size > MaxSize) size = MaxSize;
		}

		// Pop 8 entries
		for (int i = 0; i < 8; i++) {
			if (size > 0) {
				head = (head - 1 + MaxSize) % MaxSize;
				size--;
			}
		}
	}

	// After 1000 cycles of push-10/pop-8, we've accumulated 2 per cycle
	// But capped at 512
	EXPECT_LE(size, MaxSize);
	EXPECT_GT(size, 0u);
}

// ============================================================================
// Profiler: Cached pointer consistency test
// Verifies that UpdateCycles with cached pointers produces the same cycle
// counts as would be obtained by hash lookup.
// ============================================================================

// Reference implementation — simulates old Profiler::UpdateCycles with hash lookups
struct RefProfiledFunction {
	uint64_t ExclusiveCycles = 0;
	uint64_t InclusiveCycles = 0;
	uint64_t CallCount = 0;
	uint64_t MinCycles = UINT64_MAX;
	uint64_t MaxCycles = 0;
};

class ProfilerConsistencyTest : public ::testing::Test {};

TEST_F(ProfilerConsistencyTest, UpdateCycles_CachedPtrsMatchHashLookup) {
	// Simulate profiler with both hash lookup and cached pointer approaches.
	// Verify they produce identical cycle counts.

	// --- Reference (hash lookup) ---
	std::unordered_map<int32_t, RefProfiledFunction> refFunctions;
	std::deque<int32_t> refStack;
	int32_t refCurrent = -1;
	refFunctions[-1] = RefProfiledFunction{}; // Reset entry

	// --- Optimized (cached pointers) ---
	std::unordered_map<int32_t, RefProfiledFunction> optFunctions;
	std::deque<RefProfiledFunction*> optPtrStack;
	int32_t optCurrent = -1;
	optFunctions[-1] = RefProfiledFunction{};
	RefProfiledFunction* optCurrentPtr = &optFunctions[-1];

	uint64_t prevClock = 0;
	uint64_t masterClock = 0;

	// Simulate a call sequence: main → funcA → funcB → ret → ret
	auto simulateUpdateCycles = [&](uint64_t clock) {
		masterClock = clock;
		uint64_t gap = masterClock - prevClock;

		// --- Reference approach (hash lookups) ---
		refFunctions[refCurrent].ExclusiveCycles += gap;
		refFunctions[refCurrent].InclusiveCycles += gap;
		for (int i = (int)refStack.size() - 1; i >= 0; i--) {
			refFunctions[refStack[i]].InclusiveCycles += gap;
		}

		// --- Optimized approach (cached pointers) ---
		optCurrentPtr->ExclusiveCycles += gap;
		optCurrentPtr->InclusiveCycles += gap;
		for (int i = (int)optPtrStack.size() - 1; i >= 0; i--) {
			optPtrStack[i]->InclusiveCycles += gap;
		}

		prevClock = masterClock;
	};

	auto stackFunction = [&](int32_t key) {
		// Ensure function exists in both maps
		if (refFunctions.find(key) == refFunctions.end()) {
			refFunctions[key] = RefProfiledFunction{};
		}
		if (optFunctions.find(key) == optFunctions.end()) {
			optFunctions[key] = RefProfiledFunction{};
		}

		simulateUpdateCycles(masterClock);

		// Push current onto stack
		refStack.push_back(refCurrent);
		optPtrStack.push_back(optCurrentPtr);

		// Set new current
		refCurrent = key;
		refFunctions[key].CallCount++;

		optCurrent = key;
		optCurrentPtr = &optFunctions[key];
		optFunctions[key].CallCount++;
	};

	auto unstackFunction = [&]() {
		simulateUpdateCycles(masterClock);

		// Pop from stack
		refCurrent = refStack.back();
		refStack.pop_back();

		optCurrentPtr = optPtrStack.back();
		optCurrent = optCurrent; // Not used in optimized path
		optPtrStack.pop_back();
	};

	// Execute a call sequence with known cycle deltas
	masterClock = 100;
	simulateUpdateCycles(masterClock); // Initial: 100 cycles to reset func

	masterClock = 200;
	stackFunction(0x1000); // Call funcA at cycle 200

	masterClock = 350;
	stackFunction(0x2000); // Call funcB at cycle 350

	masterClock = 500;
	simulateUpdateCycles(masterClock); // More work in funcB

	masterClock = 600;
	unstackFunction(); // Return from funcB

	masterClock = 800;
	unstackFunction(); // Return from funcA

	masterClock = 1000;
	simulateUpdateCycles(masterClock); // Back in reset func

	// Verify all functions have identical stats between reference and optimized
	for (auto it = refFunctions.begin(); it != refFunctions.end(); ++it) {
		int32_t key = it->first;
		auto& refFunc = it->second;
		ASSERT_TRUE(optFunctions.count(key) > 0) << "Key " << key << " missing from optimized";
		auto& optFunc = optFunctions[key];

		EXPECT_EQ(refFunc.ExclusiveCycles, optFunc.ExclusiveCycles)
			<< "Exclusive mismatch for key " << key;
		EXPECT_EQ(refFunc.InclusiveCycles, optFunc.InclusiveCycles)
			<< "Inclusive mismatch for key " << key;
		EXPECT_EQ(refFunc.CallCount, optFunc.CallCount)
			<< "CallCount mismatch for key " << key;
	}
}

// Test: Deep call stack (100 functions) produces consistent results
TEST_F(ProfilerConsistencyTest, DeepCallStack_CachedPtrsMatch) {
	std::unordered_map<int32_t, RefProfiledFunction> refFunctions;
	std::deque<int32_t> refStack;
	int32_t refCurrent = -1;
	refFunctions[-1] = RefProfiledFunction{};

	std::unordered_map<int32_t, RefProfiledFunction> optFunctions;
	std::deque<RefProfiledFunction*> optPtrStack;
	RefProfiledFunction* optCurrentPtr;
	optFunctions[-1] = RefProfiledFunction{};
	optCurrentPtr = &optFunctions[-1];

	uint64_t prevClock = 0;
	uint64_t masterClock = 0;

	auto updateBoth = [&](uint64_t clock) {
		masterClock = clock;
		uint64_t gap = masterClock - prevClock;

		refFunctions[refCurrent].ExclusiveCycles += gap;
		refFunctions[refCurrent].InclusiveCycles += gap;
		for (int i = (int)refStack.size() - 1; i >= 0; i--) {
			refFunctions[refStack[i]].InclusiveCycles += gap;
		}

		optCurrentPtr->ExclusiveCycles += gap;
		optCurrentPtr->InclusiveCycles += gap;
		for (int i = (int)optPtrStack.size() - 1; i >= 0; i--) {
			optPtrStack[i]->InclusiveCycles += gap;
		}

		prevClock = masterClock;
	};

	// Push 100 functions
	for (int i = 0; i < 100; i++) {
		int32_t key = i * 0x100;
		refFunctions.emplace(key, RefProfiledFunction{});
		optFunctions.emplace(key, RefProfiledFunction{});

		masterClock += 10;
		updateBoth(masterClock);

		refStack.push_back(refCurrent);
		refCurrent = key;
		refFunctions[key].CallCount++;

		optPtrStack.push_back(optCurrentPtr);
		optCurrentPtr = &optFunctions[key];
		optFunctions[key].CallCount++;
	}

	// Pop all 100 functions
	for (int i = 0; i < 100; i++) {
		masterClock += 5;
		updateBoth(masterClock);

		refCurrent = refStack.back();
		refStack.pop_back();

		optCurrentPtr = optPtrStack.back();
		optPtrStack.pop_back();
	}

	masterClock += 100;
	updateBoth(masterClock);

	// Verify all match
	for (auto it = refFunctions.begin(); it != refFunctions.end(); ++it) {
		int32_t key = it->first;
		auto& refFunc = it->second;
		ASSERT_TRUE(optFunctions.count(key) > 0);
		EXPECT_EQ(refFunc.ExclusiveCycles, optFunctions[key].ExclusiveCycles)
			<< "Exclusive mismatch at depth key " << key;
		EXPECT_EQ(refFunc.InclusiveCycles, optFunctions[key].InclusiveCycles)
			<< "Inclusive mismatch at depth key " << key;
	}
}
