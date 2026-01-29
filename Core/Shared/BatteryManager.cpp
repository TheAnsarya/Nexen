#include "pch.h"
#include "Shared/BatteryManager.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/StringUtilities.h"

void BatteryManager::Initialize(string romName, bool setBatteryFlag) {
	_romName = romName;
	_hasBattery = setBatteryFlag;
}

string BatteryManager::GetBasePath(string& extension) {
	if (StringUtilities::StartsWith(extension, ".")) {
		return FolderUtilities::CombinePath(FolderUtilities::GetSaveFolder(), _romName + extension);
	} else {
		return FolderUtilities::CombinePath(FolderUtilities::GetSaveFolder(), extension);
	}
}

void BatteryManager::SetBatteryProvider(shared_ptr<IBatteryProvider> provider) {
	_provider = provider;
}

void BatteryManager::SetBatteryRecorder(shared_ptr<IBatteryRecorder> recorder) {
	_recorder = recorder;
}

void BatteryManager::SaveBattery(string extension, std::span<const uint8_t> data) {
	if (_romName.empty()) {
		// Battery saves are disabled (used by history viewer)
		return;
	}

	_hasBattery = true;
	ofstream out(GetBasePath(extension), ios::binary);
	if (out) {
		out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}
}

vector<uint8_t> BatteryManager::LoadBattery(string extension) {
	if (_romName.empty()) {
		// Battery saves are disabled (used by history viewer)
		return {};
	}

	shared_ptr<IBatteryProvider> provider = _provider.lock();

	vector<uint8_t> batteryData;
	if (provider) {
		// Used by movie player to provider initial state of ram at startup
		batteryData = provider->LoadBattery(extension);
	} else {
		VirtualFile file = GetBasePath(extension);
		if (file.IsValid()) {
			file.ReadFile(batteryData);
		}
	}

	if (!batteryData.empty()) {
		shared_ptr<IBatteryRecorder> recorder = _recorder.lock();
		if (recorder) {
			// Used by movies to record initial state of battery-backed ram at power on
			recorder->OnLoadBattery(extension, batteryData);
		}
	}

	_hasBattery = true;
	return batteryData;
}

void BatteryManager::LoadBattery(string extension, std::span<uint8_t> data) {
	vector<uint8_t> batteryData = LoadBattery(extension);
	size_t copySize = std::min(batteryData.size(), data.size());
	std::copy_n(batteryData.begin(), copySize, data.begin());
}

uint32_t BatteryManager::GetBatteryFileSize(string extension) {
	return (uint32_t)LoadBattery(extension).size();
}
