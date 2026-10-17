#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GenesisConsole;

class GenesisPsg final : public ISerializable {
private:
	Emulator* _emu = nullptr;
	GenesisConsole* _console = nullptr;

	GenesisPsgState _state = {};

public:
	GenesisPsg(Emulator* emu, GenesisConsole* console);

	GenesisPsgState& GetState() { return _state; }

	void Write(uint8_t value);
	void Reset();

	void Serialize(Serializer& s) override;
};
