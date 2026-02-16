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

	// Pen index remap table (16 entries, maps decoded pixel to palette index)
	// Written by games via registers $FC00-$FC0F (PenIndex 0-15)
	uint8_t _penIndex[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

	// Persistent SCB field values (reused when reload flags are clear)
	int16_t _persistHpos = 0;
	int16_t _persistVpos = 0;
	uint16_t _persistHsize = 0x0100;  // 8.8 fixed: 1.0
	uint16_t _persistVsize = 0x0100;
	int16_t _persistStretch = 0;
	int16_t _persistTilt = 0;

	// Sprite bus contention — tracks CPU cycles consumed during sprite processing.
	// On real hardware, the CPU is stalled while Suzy accesses the bus for sprite
	// rendering. Each bus access (read/write of work RAM) costs 1 CPU cycle.
	uint32_t _spriteBusCycles = 0;
	bool _spriteProcessingActive = false;

	// Hardware math helpers
	void DoMultiply();
	void DoDivide();

	// Sprite engine internals
	void ProcessSpriteChain();
	void ProcessSprite(uint16_t scbAddr);
	void WriteSpritePixel(int x, int y, uint8_t penIndex, uint8_t collNum, LynxSpriteType spriteType);
	int DecodeSpriteLinePixels(uint16_t& dataAddr, uint16_t lineEnd, int bpp, uint8_t* pixelBuf, int maxPixels);

	__forceinline uint8_t ReadRam(uint16_t addr);
	__forceinline uint16_t ReadRam16(uint16_t addr);
	__forceinline void WriteRam(uint16_t addr, uint8_t value);

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
