#include "pch.h"
#include "Lynx/ComLynxCable.h"
#include "Lynx/LynxMikey.h"
#include <algorithm>

void ComLynxCable::Connect(LynxMikey* unit) {
	if (!unit) {
		return;
	}

	// Prevent duplicate connections
	auto it = std::find(_connectedUnits.begin(), _connectedUnits.end(), unit);
	if (it == _connectedUnits.end()) {
		_connectedUnits.push_back(unit);
	}
}

void ComLynxCable::Disconnect(LynxMikey* unit) {
	auto it = std::find(_connectedUnits.begin(), _connectedUnits.end(), unit);
	if (it != _connectedUnits.end()) {
		// Swap-and-pop for O(1) erase (order doesn't matter on bus)
		std::swap(*it, _connectedUnits.back());
		_connectedUnits.pop_back();
	}
}

void ComLynxCable::DisconnectAll() {
	_connectedUnits.clear();
}

void ComLynxCable::Broadcast(LynxMikey* sender, uint16_t data) {
	// §11: All connected units receive the data except the sender
	// (sender already got self-loopback via ComLynxTxLoopback)
	for (LynxMikey* unit : _connectedUnits) {
		if (unit != sender) {
			unit->ComLynxRxData(data);
		}
	}
}

bool ComLynxCable::IsConnected(const LynxMikey* unit) const {
	return std::find(_connectedUnits.begin(), _connectedUnits.end(), unit) != _connectedUnits.end();
}
