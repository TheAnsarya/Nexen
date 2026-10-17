#include "pch.h"
#include "Shared/ControlDeviceState.h"

// ===== ControlDeviceState Tests =====
// Verify equality/inequality operators and assignment semantics

class ControlDeviceStateTest : public ::testing::Test {};

TEST_F(ControlDeviceStateTest, Equality_EmptyStates) {
	ControlDeviceState a;
	ControlDeviceState b;
	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
}

TEST_F(ControlDeviceStateTest, Equality_SameData) {
	ControlDeviceState a;
	a.State = {0x01, 0x02, 0x03};
	ControlDeviceState b;
	b.State = {0x01, 0x02, 0x03};
	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
}

TEST_F(ControlDeviceStateTest, Inequality_DifferentData) {
	ControlDeviceState a;
	a.State = {0x01, 0x02, 0x03};
	ControlDeviceState b;
	b.State = {0x01, 0xff, 0x03};
	EXPECT_FALSE(a == b);
	EXPECT_TRUE(a != b);
}

TEST_F(ControlDeviceStateTest, Inequality_DifferentSizes) {
	ControlDeviceState a;
	a.State = {0x01, 0x02};
	ControlDeviceState b;
	b.State = {0x01, 0x02, 0x03};
	EXPECT_FALSE(a == b);
	EXPECT_TRUE(a != b);
}

TEST_F(ControlDeviceStateTest, Equality_AfterCopyAssignment) {
	ControlDeviceState a;
	a.State = {0x10, 0x20, 0x30};
	ControlDeviceState b;
	b = a;
	EXPECT_TRUE(a == b);
}

TEST_F(ControlDeviceStateTest, Equality_AfterMoveAssignment) {
	ControlDeviceState a;
	a.State = {0x10, 0x20, 0x30};
	ControlDeviceState copy = a;
	ControlDeviceState b;
	b = std::move(a);
	EXPECT_TRUE(b == copy);
}
