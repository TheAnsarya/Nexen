#include "pch.h"
#include "Shared/Video/SystemHud.h"

TEST(SystemHudLayoutTests, BottomLeftPosition_UsesConfiguredMargins) {
	auto position = SystemHud::GetBottomLeftBoxPosition(1920, 1080, 220, 48);

	EXPECT_EQ(position.first, 10);
	EXPECT_EQ(position.second, 1018);
}

TEST(SystemHudLayoutTests, BottomLeftPosition_ClampsWhenBoxWouldOverflowWidth) {
	auto position = SystemHud::GetBottomLeftBoxPosition(180, 120, 220, 40);

	EXPECT_EQ(position.first, 0);
	EXPECT_EQ(position.second, 66);
}

TEST(SystemHudLayoutTests, BottomLeftPosition_ClampsWhenBoxWouldOverflowHeight) {
	auto position = SystemHud::GetBottomLeftBoxPosition(320, 60, 120, 80);

	EXPECT_EQ(position.first, 10);
	EXPECT_EQ(position.second, 0);
}
