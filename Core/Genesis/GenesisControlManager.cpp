#include "pch.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/Input/GenesisController.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Utilities/Serializer.h"

GenesisControlManager::GenesisControlManager(Emulator* emu, GenesisConsole* console)
	: BaseControlManager(emu, CpuType::Genesis) {
	_console = console;
}

void GenesisControlManager::ApplySixButtonSessionTimeout(uint8_t port) {
	if (port > 1 || _thCount[port] == 0) {
		return;
	}

	uint64_t idleTime = _masterClock - _lastThTransitionClock[port];
	if (idleTime >= SixButtonSessionTimeoutMclk) {
		_thCount[port] = 0;
	}
}

void GenesisControlManager::CommitThOutputState(uint8_t port, uint8_t nextThState) {
	if (port > 1) {
		return;
	}

	if (nextThState != _thState[port]) {
		_thState[port] = nextThState;
		if (_thCount[port] < 0xff) {
			_thCount[port]++;
		}
		_lastThTransitionClock[port] = _masterClock;
	}
}

bool GenesisControlManager::IsSixButtonSessionActive(uint8_t port) const {
	if (port > 1) {
		return false;
	}

	return _thCount[port] >= 4;
}

uint8_t GenesisControlManager::ResolveThOutputLevel(uint8_t port) const {
	if (port > 1) {
		return 0x40;
	}

	// TH defaults high when configured as input.
	if ((_ctrlPortWrite[port] & 0x40) == 0) {
		return 0x40;
	}

	return (uint8_t)(_dataPortWrite[port] & 0x40);
}

uint8_t GenesisControlManager::BuildRawInputState(uint8_t port, bool thHigh, bool sixButtonSession) const {
	if (port > 1) {
		return 0x7f;
	}

	shared_ptr<BaseControlDevice> device = const_cast<GenesisControlManager*>(this)->GetControlDevice(port);
	if (!device || !device->IsConnected()) {
		return 0x7f;
	}

	uint8_t value = 0;
	if (thHigh) {
		bool exposeExtraButtons = sixButtonSession && ((_thCount[port] & 0x07u) >= 4u);
		if (exposeExtraButtons) {
			value |= device->IsPressed(GenesisController::Buttons::Z)    ? 0 : 0x01;
			value |= device->IsPressed(GenesisController::Buttons::Y)    ? 0 : 0x02;
			value |= device->IsPressed(GenesisController::Buttons::X)    ? 0 : 0x04;
			value |= device->IsPressed(GenesisController::Buttons::Mode) ? 0 : 0x08;
			value |= device->IsPressed(GenesisController::Buttons::B)    ? 0 : 0x10;
			value |= device->IsPressed(GenesisController::Buttons::C)    ? 0 : 0x20;
		} else {
			value |= device->IsPressed(GenesisController::Buttons::Up)    ? 0 : 0x01;
			value |= device->IsPressed(GenesisController::Buttons::Down)  ? 0 : 0x02;
			value |= device->IsPressed(GenesisController::Buttons::Left)  ? 0 : 0x04;
			value |= device->IsPressed(GenesisController::Buttons::Right) ? 0 : 0x08;
			value |= device->IsPressed(GenesisController::Buttons::B)     ? 0 : 0x10;
			value |= device->IsPressed(GenesisController::Buttons::C)     ? 0 : 0x20;
		}
		value |= 0x40;
	} else {
		value |= device->IsPressed(GenesisController::Buttons::Up)    ? 0 : 0x01;
		value |= device->IsPressed(GenesisController::Buttons::Down)  ? 0 : 0x02;
		if (sixButtonSession && ((_thCount[port] & 0x07u) >= 4u)) {
			// 6-button signature nibble once the session is active.
			value |= 0x0c;
		}
		value |= device->IsPressed(GenesisController::Buttons::A)     ? 0 : 0x10;
		value |= device->IsPressed(GenesisController::Buttons::Start) ? 0 : 0x20;
	}

	return value;
}

uint8_t GenesisControlManager::ApplyControlPortDirectionMask(uint8_t port, uint8_t inputValue) const {
	if (port > 1) {
		return inputValue;
	}

	uint8_t mask = (uint8_t)(_ctrlPortWrite[port] & 0x7f);
	uint8_t outputBits = (uint8_t)(_dataPortWrite[port] & mask);
	uint8_t inputBits = (uint8_t)(inputValue & (uint8_t)(~mask));
	return (uint8_t)(inputBits | outputBits);
}

uint8_t GenesisControlManager::BuildDeterministicPortCapabilities(uint8_t port) const {
	if (port > 1) {
		return 0;
	}

	uint8_t capabilities = (uint8_t)(port == 0 ? 0x10 : 0x20);
	shared_ptr<BaseControlDevice> device = const_cast<GenesisControlManager*>(this)->GetControlDevice(port);
	if (device && device->IsConnected()) {
		capabilities |= 0x01;
	}
	if (_thCount[port] >= 4) {
		capabilities |= 0x02;
	}
	if ((_thState[port] & 0x40) != 0) {
		capabilities |= 0x04;
	}
	if ((_dataPortWrite[port] & 0x40) != 0) {
		capabilities |= 0x08;
	}
	if ((_ctrlPortWrite[port] & 0x40) != 0) {
		capabilities |= 0x40;
	}
	if ((_ctrlPortWrite[port] & 0x20) != 0) {
		capabilities |= 0x80;
	}

	return capabilities;
}

uint8_t GenesisControlManager::BuildDeterministicPortDigest(uint8_t port) const {
	if (port > 1) {
		return 0;
	}

	uint8_t digest = 0x5a;
	digest ^= BuildDeterministicPortCapabilities(port);
	digest ^= _dataPortWrite[port];
	digest ^= (uint8_t)(_ctrlPortWrite[port] << 1);
	digest ^= (uint8_t)(_thState[port] >> 1);
	digest ^= (uint8_t)(_thCount[port] * 13u);
	return digest;
}

shared_ptr<BaseControlDevice> GenesisControlManager::CreateControllerDevice(ControllerType type, uint8_t port) {
	shared_ptr<BaseControlDevice> device;

	KeyMappingSet keys;
	switch (port) {
		default:
		case 0:
			keys = _emu->GetSettings()->GetGenesisConfig().Port1.Keys;
			break;
		case 1:
			keys = _emu->GetSettings()->GetGenesisConfig().Port2.Keys;
			break;
	}

	switch (type) {
		default:
		case ControllerType::None:
			break;
		case ControllerType::GenesisController:
			device = std::make_unique<GenesisController>(_emu, port, keys);
			break;
	}

	return device;
}

void GenesisControlManager::UpdateControlDevices() {
	GenesisConfig& cfg = _emu->GetSettings()->GetGenesisConfig();
	if (_emu->GetSettings()->IsEqual(_prevConfig, cfg) && _controlDevices.size() > 0) {
		return;
	}

	auto lock = _deviceLock.AcquireSafe();
	ClearDevices();

	for (int i = 0; i < 2; i++) {
		shared_ptr<BaseControlDevice> device = CreateControllerDevice(
			i == 0 ? cfg.Port1.Type : cfg.Port2.Type, i
		);
		if (device) {
			RegisterControlDevice(device);
		}
	}

	_prevConfig = cfg;
}

void GenesisControlManager::AdvanceMasterClock(uint32_t masterClocks) {
	_masterClock += masterClocks;
	ApplySixButtonSessionTimeout(0);
	ApplySixButtonSessionTimeout(1);
}

uint8_t GenesisControlManager::ReadDataPort(uint8_t port) {
	SetInputReadFlag();

	if (port > 1) {
		return 0xff;
	}

	ApplySixButtonSessionTimeout(port);
	bool thHigh = (ResolveThOutputLevel(port) & 0x40) != 0;
	bool sixButtonSession = IsSixButtonSessionActive(port);
	uint8_t rawInput = BuildRawInputState(port, thHigh, sixButtonSession);
	return ApplyControlPortDirectionMask(port, rawInput);
}

void GenesisControlManager::WriteDataPort(uint8_t port, uint8_t value) {
	if (port > 1) {
		return;
	}

	_dataPortWrite[port] = value;
	uint8_t nextThState = ResolveThOutputLevel(port);
	CommitThOutputState(port, nextThState);
}

void GenesisControlManager::WriteControlPort(uint8_t port, uint8_t value) {
	if (port > 1) {
		return;
	}

	_ctrlPortWrite[port] = value;
	uint8_t nextThState = ResolveThOutputLevel(port);
	CommitThOutputState(port, nextThState);
}

void GenesisControlManager::ResetRuntimeState() {
	memset(_dataPortWrite, 0, sizeof(_dataPortWrite));
	memset(_ctrlPortWrite, 0, sizeof(_ctrlPortWrite));
	memset(_thState, 0, sizeof(_thState));
	memset(_thCount, 0, sizeof(_thCount));
	memset(_lastThTransitionClock, 0, sizeof(_lastThTransitionClock));
	_masterClock = 0;
}

uint8_t GenesisControlManager::GetDataPortWriteLatch(uint8_t port) const {
	if (port > 1) {
		return 0;
	}
	return _dataPortWrite[port];
}

uint8_t GenesisControlManager::GetControlPortWriteLatch(uint8_t port) const {
	if (port > 1) {
		return 0;
	}
	return _ctrlPortWrite[port];
}

uint8_t GenesisControlManager::GetThState(uint8_t port) const {
	if (port > 1) {
		return 0;
	}
	return _thState[port];
}

uint8_t GenesisControlManager::GetThCount(uint8_t port) const {
	if (port > 1) {
		return 0;
	}
	return _thCount[port];
}

uint8_t GenesisControlManager::GetDeterministicPortCapabilities(uint8_t port) const {
	return BuildDeterministicPortCapabilities(port);
}

uint8_t GenesisControlManager::GetDeterministicPortDigest(uint8_t port) const {
	return BuildDeterministicPortDigest(port);
}

void GenesisControlManager::Serialize(Serializer& s) {
	BaseControlManager::Serialize(s);
	SVArray(_dataPortWrite, 2);
	SVArray(_ctrlPortWrite, 2);
	SVArray(_thState, 2);
	SVArray(_thCount, 2);
	SVArray(_lastThTransitionClock, 2);
	SV(_masterClock);
}
