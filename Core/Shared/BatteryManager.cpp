#include "pch.h"
#include "Shared/BatteryManager.h"
#include "Utilities/VirtualFile.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/StringUtilities.h"

void BatteryManager::Initialize(const string& romName, bool setBatteryFlag) {
	_romName = romName;
	_hasBattery = setBatteryFlag;
}

string BatteryManager::GetBasePath(const string& extension) {
	string folder;
	if (!_perRomSaveDir.empty()) {
		FolderUtilities::CreateFolder(_perRomSaveDir);
		folder = _perRomSaveDir;
	} else {
		folder = FolderUtilities::GetSaveFolder();
	}

	if (StringUtilities::StartsWith(extension, ".")) {
		return FolderUtilities::CombinePath(folder, _romName + extension);
	} else {
		return FolderUtilities::CombinePath(folder, extension);
	}
}

void BatteryManager::SetBatteryProvider(shared_ptr<IBatteryProvider> provider) {
	_provider = provider;
}

void BatteryManager::SetBatteryRecorder(shared_ptr<IBatteryRecorder> recorder) {
	_recorder = recorder;
}

void BatteryManager::SaveBattery(const string& extension, std::span<const uint8_t> data) {
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

vector<uint8_t> BatteryManager::LoadBattery(const string& extension) {
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
			(void)file.ReadFile(batteryData);
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

void BatteryManager::LoadBattery(const string& extension, std::span<uint8_t> data) {
	vector<uint8_t> batteryData = LoadBattery(extension);
	size_t copySize = std::min(batteryData.size(), data.size());
	std::copy_n(batteryData.begin(), copySize, data.begin());
}

uint32_t BatteryManager::GetBatteryFileSize(const string& extension) {
	return (uint32_t)LoadBattery(extension).size();
}

void BatteryManager::SetPerRomSaveDirectory(const string& path) {
	_perRomSaveDir = path;
	if (!path.empty()) {
		FolderUtilities::CreateFolder(path);
	}
}
