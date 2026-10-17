#pragma once
#include "pch.h"
#include "Netplay/NetMessage.h"
#include "Utilities/sha1.h"

class HandShakeMessage : public NetMessage {
private:
	static constexpr int CurrentVersion = 200; // Use 200+ to distinguish from original Nexen & Nexen-S
	uint32_t _emuVersion = 0;
	uint32_t _protocolVersion = CurrentVersion;
	string _hashedPassword;
	bool _spectator = false;

protected:
	void Serialize(Serializer& s) override {
		SV(_emuVersion);
		SV(_protocolVersion);
		SV(_hashedPassword);
		SV(_spectator);
	}

public:
	HandShakeMessage(void* buffer, uint32_t length) : NetMessage(buffer, length) {}

	HandShakeMessage(const string& hashedPassword, bool spectator, uint32_t emuVersion) : NetMessage(MessageType::HandShake) {
		_emuVersion = emuVersion;
		_protocolVersion = HandShakeMessage::CurrentVersion;
		_hashedPassword = hashedPassword;
		_spectator = spectator;
	}

	[[nodiscard]] bool IsValid(uint32_t emuVersion) {
		return _protocolVersion == CurrentVersion && _emuVersion == emuVersion;
	}

	[[nodiscard]] bool CheckPassword(const string& serverPassword, const string& connectionHash) {
		return GetPasswordHash(serverPassword, connectionHash) == string(_hashedPassword);
	}

	[[nodiscard]] bool IsSpectator() {
		return _spectator;
	}

	[[nodiscard]] static string GetPasswordHash(const string& serverPassword, const string& connectionHash) {
		string saltedPassword = serverPassword + connectionHash;
		vector<uint8_t> dataToHash = vector<uint8_t>(saltedPassword.c_str(), saltedPassword.c_str() + saltedPassword.size());
		return SHA1::GetHash(dataToHash);
	}
};
