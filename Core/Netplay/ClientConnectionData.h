#pragma once

#include "pch.h"

class ClientConnectionData {
public:
	string Host;
	uint16_t Port = 0;
	string Password;
	bool Spectator = false;

	ClientConnectionData() {}

	ClientConnectionData(const string& host, uint16_t port, const string& password, bool spectator) : Host(host), Port(port), Password(password), Spectator(spectator) {
	}

	~ClientConnectionData() {
	}
};
