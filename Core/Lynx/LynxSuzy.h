#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class LynxConsole;
class LynxMemoryManager;
class LynxCart;

class LynxSuzy final : public ISerializable {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxMemoryManager* _memoryManager = nullptr;
	LynxCart* _cart = nullptr;

	LynxSuzyState _state = {};

	// Sprite rendering scratch space
	uint8_t _lineBuffer[LynxConstants::ScreenWidth] = {};

	// Hardware math helpers
	void DoMultiply();
	void DoDivide();

	// Sprite engine internals
	void ProcessSpriteChain();
	void ProcessSprite(uint16_t scbAddr);
	void WriteSpritePixel(int x, int y, uint8_t penIndex, uint8_t collNum);

	__forceinline uint8_t ReadRam(uint16_t addr) const;
	__forceinline uint16_t ReadRam16(uint16_t addr) const;
	__forceinline void WriteRam(uint16_t addr, uint8_t value) const;

public:
	void Init(Emulator* emu, LynxConsole* console, LynxMemoryManager* memoryManager, LynxCart* cart);

	uint8_t ReadRegister(uint8_t addr);
	void WriteRegister(uint8_t addr, uint8_t value);

	bool IsSpriteBusy() const { return _state.SpriteBusy; }

	uint8_t GetJoystick() const { return _state.Joystick; }
	void SetJoystick(uint8_t value) { _state.Joystick = value; }

	uint8_t GetSwitches() const { return _state.Switches; }
	void SetSwitches(uint8_t value) { _state.Switches = value; }

	LynxSuzyState& GetState() { return _state; }

	void Serialize(Serializer& s) override;
};
