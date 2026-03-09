#include "pch.h"
#include "Shared/NotificationManager.h"

class TestNotificationListener : public INotificationListener {
public:
	int NotificationCount = 0;
	ConsoleNotificationType LastType = ConsoleNotificationType::GameLoaded;

	void ProcessNotification(ConsoleNotificationType type, void* parameter) override {
		(void)parameter;
		NotificationCount++;
		LastType = type;
	}
};

TEST(NotificationManagerTests, RegisteringSameListenerTwice_DoesNotDuplicateNotifications) {
	NotificationManager manager;
	auto listener = std::make_shared<TestNotificationListener>();

	manager.RegisterNotificationListener(listener);
	manager.RegisterNotificationListener(listener);
	manager.SendNotification(ConsoleNotificationType::GamePaused, nullptr);

	EXPECT_EQ(listener->NotificationCount, 1);
	EXPECT_TRUE(listener->LastType == ConsoleNotificationType::GamePaused);
}

TEST(NotificationManagerTests, ExpiredListeners_AreIgnoredWhenSendingNotifications) {
	NotificationManager manager;
	auto liveListener = std::make_shared<TestNotificationListener>();
	manager.RegisterNotificationListener(liveListener);

	{
		auto expiredListener = std::make_shared<TestNotificationListener>();
		manager.RegisterNotificationListener(expiredListener);
	}

	manager.SendNotification(ConsoleNotificationType::GameResumed, nullptr);

	EXPECT_EQ(liveListener->NotificationCount, 1);
	EXPECT_TRUE(liveListener->LastType == ConsoleNotificationType::GameResumed);
}
