#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Shared/BaseControlManager.h"
#include "Shared/SettingTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GenesisConsole;

class GenesisControlManager final : public BaseControlManager {
private:
	GenesisConsole* _console = nullptr;
	GenesisConfig _prevConfig = {};

	// Per-port TH state (directly controlled by 68k via data port writes)
	uint8_t _dataPortWrite[2] = {};
	uint8_t _ctrlPortWrite[2] = {};
	uint8_t _thState[2] = {};
	uint8_t _thCount[2] = {};
	uint64_t _lastThTransitionClock[2] = {};
	uint64_t _masterClock = 0;

	static constexpr uint64_t SixButtonSessionTimeoutMclk = 12288;

	// Controller button states
	GenesisControllerState _padState[2] = {};

	void ApplySixButtonSessionTimeout(uint8_t port);
	void CommitThOutputState(uint8_t port, uint8_t nextThState);
	[[nodiscard]] bool IsSixButtonSessionActive(uint8_t port) const;
	[[nodiscard]] uint8_t ResolveThOutputLevel(uint8_t port) const;
	[[nodiscard]] uint8_t BuildRawInputState(uint8_t port, bool thHigh, bool sixButtonSession) const;
	[[nodiscard]] uint8_t ApplyControlPortDirectionMask(uint8_t port, uint8_t inputValue) const;
	[[nodiscard]] uint8_t BuildDeterministicPortCapabilities(uint8_t port) const;
	[[nodiscard]] uint8_t BuildDeterministicPortDigest(uint8_t port) const;

public:
	GenesisControlManager(Emulator* emu, GenesisConsole* console);

	shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) override;
	void UpdateControlDevices() override;

	void AdvanceMasterClock(uint32_t masterClocks);
	uint8_t ReadDataPort(uint8_t port);
	void WriteDataPort(uint8_t port, uint8_t value);
	void WriteControlPort(uint8_t port, uint8_t value);
	void ResetRuntimeState();
	[[nodiscard]] uint8_t GetDataPortWriteLatch(uint8_t port) const;
	[[nodiscard]] uint8_t GetControlPortWriteLatch(uint8_t port) const;
	[[nodiscard]] uint8_t GetThState(uint8_t port) const;
	[[nodiscard]] uint8_t GetThCount(uint8_t port) const;
	[[nodiscard]] uint8_t GetDeterministicPortCapabilities(uint8_t port) const;
	[[nodiscard]] uint8_t GetDeterministicPortDigest(uint8_t port) const;

	void Serialize(Serializer& s) override;
};
