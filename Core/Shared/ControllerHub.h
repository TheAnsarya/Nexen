#pragma once

#include "pch.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/InputHud.h"
#include "Shared/IControllerHub.h"
#include "SNES/Input/SnesController.h"
#include "SNES/Input/SnesMouse.h"
#include "NES/Input/NesController.h"
#include "PCE/Input/PceController.h"
#include "PCE/Input/PceAvenuePad6.h"
#include "Utilities/Serializer.h"
#include "Utilities/StringUtilities.h"

/// <summary>
/// Multi-controller hub (multitap) for connecting multiple controllers to single port.
/// Template supports 2-5 controller ports (NES Four Score, SNES Super Multitap, etc.).
/// </summary>
/// <remarks>
/// Multitap types:
/// - NES Four Score (4 ports)
/// - SNES Super Multitap (4 or 5 ports)
/// - PCE Multitap (5 ports)
///
/// Architecture:
/// - Template parameter HubPortCount specifies number of sub-ports
/// - Each sub-port can have different controller type
/// - Hub aggregates input state from all connected controllers
/// - State serialized as length-prefixed chunks
///
/// Input flow:
/// 1. InternalSetStateFromInput() - Poll all sub-port controllers
/// 2. UpdateStateFromPorts() - Aggregate states into hub state
/// 3. ReadRam/WriteRam - Multiplex hardware reads/writes
///
/// State format:
/// - [length:1][port0_data:length][length:1][port1_data:length]...
/// - Length-prefixed for variable controller types
/// - Text state: "port0:port1:port2:port3"
///
/// Features:
/// - Dynamic controller types per port
/// - Save state support
/// - Input HUD visualization
/// - Text state import/export
/// - RefreshHubState() for debugger controller changes
///
/// Thread safety: Inherits lock from BaseControlDevice.
/// </remarks>
template <int HubPortCount>
class ControllerHub : public BaseControlDevice, public IControllerHub {
protected:
	shared_ptr<BaseControlDevice> _ports[HubPortCount]; ///< Sub-port controller instances

	/// <summary>Poll input from all connected controllers</summary>
	void InternalSetStateFromInput() override {
		for (int i = 0; i < HubPortCount; i++) {
			if (_ports[i]) {
				_ports[i]->SetStateFromInput();
			}
		}

		UpdateStateFromPorts();
	}

	/// <summary>
	/// Aggregate state from sub-ports into hub state.
	/// Format: [length:1][data...] for each port.
	/// </summary>
	void UpdateStateFromPorts() {
		for (int i = 0; i < HubPortCount; i++) {
			if (_ports[i]) {
				ControlDeviceState portState = _ports[i]->GetRawState();
				_state.State.push_back((uint8_t)portState.State.size());
				_state.State.insert(_state.State.end(), portState.State.begin(), portState.State.end());
			}
		}
	}

	/// <summary>Read byte from sub-port controller</summary>
	uint8_t ReadPort(int i) {
		if (_ports[i]) {
			return _ports[i]->ReadRam(0x4016);
		} else {
			return 0;
		}
	}

	/// <summary>Write byte to sub-port controller</summary>
	void WritePort(int i, uint8_t value) {
		if (_ports[i]) {
			_ports[i]->WriteRam(0x4016, value);
		}
	}

public:
	/// <summary>
	/// Construct controller hub with specified sub-port controllers.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="type">Hub type (FourScore, SuperMultitap, etc.)</param>
	/// <param name="port">Physical port number</param>
	/// <param name="controllers">Array of controller configurations</param>
	ControllerHub(Emulator* emu, ControllerType type, int port, ControllerConfig controllers[]) : BaseControlDevice(emu, type, port) {
		static_assert(HubPortCount <= MaxSubPorts, "Port count too large");

		for (int i = 0; i < HubPortCount; i++) {
			switch (controllers[i].Type) {
				case ControllerType::FamicomController:
				case ControllerType::FamicomControllerP2:
				case ControllerType::NesController:
					_ports[i].reset(new NesController(emu, controllers[i].Type, 0, controllers[i].Keys));
					break;

				case ControllerType::SnesController:
					_ports[i].reset(new SnesController(emu, 0, controllers[i].Keys));
					break;

				case ControllerType::SnesMouse:
					_ports[i].reset(new SnesMouse(emu, 0, controllers[i].Keys));
					break;

				case ControllerType::PceController:
					_ports[i].reset(new PceController(emu, 0, controllers[i].Keys));
					break;

				case ControllerType::PceAvenuePad6:
					_ports[i].reset(new PceAvenuePad6(emu, 0, controllers[i].Keys));
					break;
			}
		}
	}

	/// <summary>Write strobe and broadcast to all sub-ports</summary>
	void WriteRam(uint16_t addr, uint8_t value) override {
		StrobeProcessWrite(value);
		for (int i = 0; i < HubPortCount; i++) {
			if (_ports[i]) {
				_ports[i]->WriteRam(addr, value);
			}
		}
	}

	/// <summary>Draw all sub-port controllers on input HUD</summary>
	void DrawController(InputHud& hud) override {
		for (int i = 0; i < HubPortCount; i++) {
			if (_ports[i]) {
				_ports[i]->DrawController(hud);
			} else {
				hud.EndDrawController();
			}
		}
	}

	/// <summary>
	/// Set state from text format (e.g., "RLDU:A:B:START").
	/// Colon-separated for each sub-port.
	/// </summary>
	void SetTextState(string state) override {
		vector<string> portStates = StringUtilities::Split(state, ':');
		int i = 0;
		for (string& portState : portStates) {
			if (_ports[i]) {
				_ports[i]->SetTextState(portState);
			}
			i++;
		}

		UpdateStateFromPorts();
	}

	/// <summary>
	/// Get state as text format (e.g., "RLDU:A:B:START").
	/// Colon-separated for each sub-port.
	/// </summary>
	string GetTextState() override {
		auto lock = _stateLock.AcquireSafe();

		string state;
		for (int i = 0; i < HubPortCount; i++) {
			if (i != 0) {
				state += ":";
			}
			if (_ports[i]) {
				state += _ports[i]->GetTextState();
			}
		}

		return state;
	}

	/// <summary>
	/// Set raw state from binary format (length-prefixed chunks).
	/// </summary>
	void SetRawState(ControlDeviceState state) override {
		auto lock = _stateLock.AcquireSafe();
		_state = state;

		vector<uint8_t> data = state.State;
		int pos = 0;

		for (int i = 0; i < HubPortCount; i++) {
			if (_ports[i] && pos < data.size()) {
				int length = data[pos++];

				if (pos + length > data.size()) {
					break;
				}

				ControlDeviceState portState;
				portState.State.insert(portState.State.begin(), data.begin() + pos, data.begin() + pos + length);
				_ports[i]->SetRawState(portState);
				pos += length;
			}
		}
	}

	/// <summary>
	/// Check if specific controller type connected to any sub-port.
	/// </summary>
	bool HasControllerType(ControllerType type) override {
		if (_type == type) {
			return true;
		}

		for (int i = 0; i < HubPortCount; i++) {
			if (_ports[i] && _ports[i]->HasControllerType(type)) {
				return true;
			}
		}

		return false;
	}

	/// <summary>
	/// Refresh hub state after debugger controller changes.
	/// </summary>
	void RefreshHubState() override {
		// Used when the connected devices are updated by code (e.g by the debugger)
		_state.State.clear();
		UpdateStateFromPorts();
	}

	/// <summary>Get number of sub-ports</summary>
	int GetHubPortCount() override {
		return HubPortCount;
	}

	/// <summary>
	/// Get controller at sub-port index.
	/// </summary>
	/// <param name="index">Sub-port index (0-based)</param>
	shared_ptr<BaseControlDevice> GetController(int index) override {
		if (index >= HubPortCount) {
			return nullptr;
		}

		return _ports[index];
	}
};
