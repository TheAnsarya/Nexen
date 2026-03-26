#include "pch.h"
#include "ChannelF/ChannelFControlManager.h"
#include "Shared/Emulator.h"

ChannelFControlManager::ChannelFControlManager(Emulator* emu)
	: BaseControlManager(emu, CpuType::Atari2600) {
}

shared_ptr<BaseControlDevice> ChannelFControlManager::CreateControllerDevice([[maybe_unused]] ControllerType type, [[maybe_unused]] uint8_t port) {
	return nullptr;
}

void ChannelFControlManager::UpdateControlDevices() {
	// Controller mappings will be wired in a dedicated Channel F input issue.
}

void ChannelFControlManager::UpdateInputState() {
	BaseControlManager::UpdateInputState();
}
