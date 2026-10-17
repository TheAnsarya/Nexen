#include "pch.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/Input/GenesisController.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/KeyManager.h"
#include "Utilities/Serializer.h"

namespace {
	bool HasAnyKeyMapping(KeyMappingSet& keys) {
		return keys.Mapping1.HasKeySet()
			|| keys.Mapping2.HasKeySet()
			|| keys.Mapping3.HasKeySet()
			|| keys.Mapping4.HasKeySet();
	}

	void ApplyDefaultGenesisMapping(KeyMappingSet& keys, uint8_t port, bool sixButton) {
		if (HasAnyKeyMapping(keys)) {
			return;
		}

		KeyMapping map = {};
		if (port == 0) {
			map.Up = KeyManager::GetKeyCode("Up Arrow");
			map.Down = KeyManager::GetKeyCode("Down Arrow");
			map.Left = KeyManager::GetKeyCode("Left Arrow");
			map.Right = KeyManager::GetKeyCode("Right Arrow");
			map.A = KeyManager::GetKeyCode("Z");
			map.B = KeyManager::GetKeyCode("X");
			map.TurboA = KeyManager::GetKeyCode("C");
			map.Start = KeyManager::GetKeyCode("Enter");
			if (sixButton) {
				map.X = KeyManager::GetKeyCode("A");
				map.Y = KeyManager::GetKeyCode("S");
				map.TurboB = KeyManager::GetKeyCode("D");
				map.Select = KeyManager::GetKeyCode("Right Shift");
			}
		} else {
			map.Up = KeyManager::GetKeyCode("W");
			map.Down = KeyManager::GetKeyCode("S");
			map.Left = KeyManager::GetKeyCode("A");
			map.Right = KeyManager::GetKeyCode("D");
			map.A = KeyManager::GetKeyCode("F");
			map.B = KeyManager::GetKeyCode("G");
			map.TurboA = KeyManager::GetKeyCode("H");
			map.Start = KeyManager::GetKeyCode("Tab");
			if (sixButton) {
				map.X = KeyManager::GetKeyCode("R");
				map.Y = KeyManager::GetKeyCode("T");
				map.TurboB = KeyManager::GetKeyCode("Y");
				map.Select = KeyManager::GetKeyCode("Left Shift");
			}
		}

		keys.Mapping1 = map;
		keys.TurboSpeed = 2;
	}
}

GenesisControlManager::GenesisControlManager(Emulator* emu, GenesisConsole* console)
	: BaseControlManager(emu, CpuType::Genesis) {
	_console = console;
	UpdateControlDevices();
}

void GenesisControlManager::ApplySixButtonSessionTimeout(uint8_t port) {
	if (port > 1 || _thPulseCounter[port] == 0) {
		return;
	}

	uint64_t idleTime = _masterClock - _lastThTransitionClock[port];
	if (idleTime >= SixButtonSessionTimeoutMclk) {
		_thPulseCounter[port] = 0;
	}
}

void GenesisControlManager::WriteGamepadPort(uint8_t port, uint8_t data, uint8_t directionMask) {
	if (port > 1) {
		return;
	}
	uint8_t prevState = (uint8_t)(_thState[port] & 0x40u);
	bool thOutput = (directionMask & 0x40u) != 0u;
	bool compatBootThDrive = !thOutput && directionMask == 0x00u;

	if (thOutput || compatBootThDrive) {
		uint8_t nextState = (uint8_t)(data & 0x40u);
		_thInputLatencyUntilClock[port] = 0;

		if (IsSixButtonDevice(port) && nextState != 0 && prevState == 0) {
			if (_thPulseCounter[port] <= 6u) {
				_thPulseCounter[port] = (uint8_t)(_thPulseCounter[port] + 2u);
			} else {
				_thPulseCounter[port] = 8u;
			}
			_lastThTransitionClock[port] = _masterClock;
		}

		if (nextState != prevState) {
			_lastThTransitionClock[port] = _masterClock;
		}
		_thState[port] = nextState;
	} else {
		_thState[port] = 0x40u;
		if (prevState == 0) {
			// Hardware delays TH input-high visibility after direction flips to input.
			_thInputLatencyUntilClock[port] = _masterClock + 172u;
		}
	}

	auto device = GetControlDevice(port);
	if (!device || !device->IsConnected()) {
		_thPulseCounter[port] = 0;
		_thInputLatencyUntilClock[port] = 0;
	}
}

void GenesisControlManager::ApplyPortWriteState(uint8_t port) {
	if (port > 1) {
		return;
	}

	WriteGamepadPort(port, (uint8_t)(_dataPortWrite[port] & 0x7fu), (uint8_t)(_ctrlPortWrite[port] & 0x7fu));
}

bool GenesisControlManager::IsSixButtonSessionActive(uint8_t port) const {
	if (port > 1) {
		return false;
	}

	return _thPulseCounter[port] >= 4u;
}

uint8_t GenesisControlManager::BuildSixButtonStep(uint8_t port, bool thHigh) const {
	if (port > 1) {
		return thHigh ? 1u : 0u;
	}

	uint8_t step = (uint8_t)(thHigh ? 1u : 0u);
	if (IsSixButtonDevice(port)) {
		step = (uint8_t)(step | _thPulseCounter[port]);
	}

	if (_masterClock < _thInputLatencyUntilClock[port]) {
		step &= 0xfeu;
	}

	if (!IsSixButtonDevice(port)) {
		step = (uint8_t)(thHigh ? 1u : 0u);
	}

	return step;
}

bool GenesisControlManager::IsSixButtonDevice(uint8_t port) const {
	if (port > 1) {
		return false;
	}

	shared_ptr<BaseControlDevice> device = const_cast<GenesisControlManager*>(this)->GetControlDevice(port);
	if (!device || !device->IsConnected()) {
		return false;
	}

	if (device->HasControllerType(ControllerType::GenesisController3Buttons)) {
		return false;
	}

	if (device->HasControllerType(ControllerType::GenesisController)) {
		return true;
	}

	GenesisController* pad = dynamic_cast<GenesisController*>(device.get());
	return pad ? pad->IsSixButtonController() : false;
}

uint8_t GenesisControlManager::ResolveThOutputLevel(uint8_t port) const {
	if (port > 1) {
		return 0x40;
	}

	return (uint8_t)(_thState[port] & 0x40u);
}

uint8_t GenesisControlManager::BuildRawInputState(uint8_t port, bool thHigh, bool sixButtonSession) const {
	if (port > 1) {
		return 0x7f;
	}

	shared_ptr<BaseControlDevice> device = const_cast<GenesisControlManager*>(this)->GetControlDevice(port);
	if (!device || !device->IsConnected()) {
		return 0x7f;
	}

	uint8_t value = (uint8_t)((thHigh ? 0x40u : 0x00u) | 0x3fu);
	uint8_t step = BuildSixButtonStep(port, thHigh);
	if (!sixButtonSession) {
		step = (uint8_t)(thHigh ? 1u : 0u);
	}

	auto activeLow = [&](uint8_t bit, GenesisController::Buttons button) {
		if (device->IsPressed(button)) {
			uint8_t bitMask = (uint8_t)(1u << bit);
			if ((value & bitMask) != 0u) {
				value = (uint8_t)(value - bitMask);
			}
		}
	};

	switch (step) {
		case 2: // Third low: ?0SA0000
			value = 0x30u;
			activeLow(5, GenesisController::Buttons::Start);
			activeLow(4, GenesisController::Buttons::A);
			break;

		case 5: // Fourth high: ?1CBMXYZ
			value = 0x7Fu;
			activeLow(5, GenesisController::Buttons::C);
			activeLow(4, GenesisController::Buttons::B);
			activeLow(3, GenesisController::Buttons::Mode);
			activeLow(2, GenesisController::Buttons::X);
			activeLow(1, GenesisController::Buttons::Y);
			activeLow(0, GenesisController::Buttons::Z);
			break;

		case 4: // Fourth low: ?0SA1111
			value = 0x3Fu;
			activeLow(5, GenesisController::Buttons::Start);
			activeLow(4, GenesisController::Buttons::A);
			break;

		default:
			if ((step & 1u) != 0u) {
				// TH=1: ?1CBRLDU
				value = 0x7Fu;
				activeLow(5, GenesisController::Buttons::C);
				activeLow(4, GenesisController::Buttons::B);
				activeLow(3, GenesisController::Buttons::Right);
				activeLow(2, GenesisController::Buttons::Left);
				activeLow(1, GenesisController::Buttons::Down);
				activeLow(0, GenesisController::Buttons::Up);
			} else {
				// TH=0: ?0SA00DU
				value = 0x33u;
				activeLow(5, GenesisController::Buttons::Start);
				activeLow(4, GenesisController::Buttons::A);
				activeLow(1, GenesisController::Buttons::Down);
				activeLow(0, GenesisController::Buttons::Up);
			}
			break;
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
	if (_thPulseCounter[port] >= 4u) {
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
	digest ^= (uint8_t)(_thPulseCounter[port] * 13u);
	digest ^= (uint8_t)((_thInputLatencyUntilClock[port] > _masterClock) ? 0x33u : 0u);
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
			ApplyDefaultGenesisMapping(keys, port, true);
			device = std::make_unique<GenesisController>(_emu, port, keys, true);
			break;
		case ControllerType::GenesisController3Buttons:
			ApplyDefaultGenesisMapping(keys, port, false);
			device = std::make_unique<GenesisController>(_emu, port, keys, false);
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

uint32_t GenesisControlManager::GetButtonsForPort(int port) {
	auto lock = _deviceLock.AcquireSafe();
	for (shared_ptr<BaseControlDevice>& dev : _controlDevices) {
		if (dev->GetPort() == (uint8_t)port && (
			dev->HasControllerType(ControllerType::GenesisController) ||
			dev->HasControllerType(ControllerType::GenesisController3Buttons)
		)) {
			GenesisController* pad = dynamic_cast<GenesisController*>(dev.get());
			return pad ? pad->GetButtonMask() : 0u;
		}
	}
	return 0u;
}

bool GenesisControlManager::IsPortConnected(int port) {
	auto lock = _deviceLock.AcquireSafe();
	for (shared_ptr<BaseControlDevice>& dev : _controlDevices) {
		if (dev->GetPort() == (uint8_t)port) {
			return dev->IsConnected();
		}
	}
	return false;
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
	shared_ptr<BaseControlDevice> device = GetControlDevice(port);
	if (!device || !device->IsConnected()) {
		_thPulseCounter[port] = 0;
		_thInputLatencyUntilClock[port] = 0;
		return 0x7f;
	}

	bool thHigh = (ResolveThOutputLevel(port) & 0x40) != 0;
	bool sixButtonSession = IsSixButtonDevice(port) && IsSixButtonSessionActive(port);
	uint8_t rawInput = BuildRawInputState(port, thHigh, sixButtonSession);
	return (uint8_t)(ApplyControlPortDirectionMask(port, rawInput) & 0x7fu);
}

void GenesisControlManager::WriteDataPort(uint8_t port, uint8_t value) {
	if (port > 1) {
		return;
	}

	_dataPortWrite[port] = (uint8_t)(value & 0x7fu);
	ApplyPortWriteState(port);
}

void GenesisControlManager::WriteControlPort(uint8_t port, uint8_t value) {
	if (port > 1) {
		return;
	}

	_ctrlPortWrite[port] = (uint8_t)(value & 0x7fu);
	ApplyPortWriteState(port);
}

void GenesisControlManager::ResetRuntimeState() {
	memset(_dataPortWrite, 0, sizeof(_dataPortWrite));
	memset(_ctrlPortWrite, 0, sizeof(_ctrlPortWrite));
	memset(_thState, 0x40, sizeof(_thState));
	memset(_thPulseCounter, 0, sizeof(_thPulseCounter));
	memset(_thInputLatencyUntilClock, 0, sizeof(_thInputLatencyUntilClock));
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
	return _thPulseCounter[port];
}

uint8_t GenesisControlManager::GetDeterministicPortCapabilities(uint8_t port) const {
	return BuildDeterministicPortCapabilities(port);
}

uint8_t GenesisControlManager::GetDeterministicPortDigest(uint8_t port) const {
	return BuildDeterministicPortDigest(port);
}

void GenesisControlManager::Serialize(Serializer& s) {
	BaseControlManager::Serialize(s);

	if (!s.IsSaving()) {
		UpdateControlDevices();
	}

	for (uint8_t i = 0; i < _controlDevices.size(); i++) {
		SVI(_controlDevices[i]);
	}

	SVArray(_dataPortWrite, 2);
	SVArray(_ctrlPortWrite, 2);
	SVArray(_thState, 2);
	SVArray(_thPulseCounter, 2);
	SVArray(_thInputLatencyUntilClock, 2);
	SVArray(_lastThTransitionClock, 2);
	SV(_masterClock);
}
