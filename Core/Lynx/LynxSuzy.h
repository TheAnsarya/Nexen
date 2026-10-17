#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class LynxConsole;
class LynxMemoryManager;
class LynxCart;

/// @file LynxSuzy.h
/// @brief Atari Lynx Suzy chip emulation.
///
/// Suzy is the Lynx's custom graphics/math coprocessor containing:
/// - **Sprite Engine**: Hardware sprite rendering with scaling, clipping, collisions
/// - **Math Unit**: 16×16→32 multiply, 32÷16 divide with accumulate modes
/// - **Joystick/Switch Interface**: Button input and system switches
///
/// **Memory Map** ($FC00–$FCFF):
/// | Address | Name | Description |
/// |---------|------|-------------|
/// | $FC00–$FC0F | SPRSYS | Sprite control registers |
/// | $FC10–$FC1F | MATHA–MATHN | Math operands/results |
/// | $FC80 | JOYSTICK | Controller input |
/// | $FC88 | SWITCHES | System switches (Opt1/2, Pause, Cart power) |
/// | $FC90 | SUZY_BUSEN | Bus enable (for debugging) |
///
/// **Hardware Bugs Emulated** (Chapter 13 of Lynx Hardware Reference):
/// - Bug 13.8: Signed multiply edge cases ($8000, $0000)
/// - Bug 13.10: MathOverflow flag overwritten on each operation
///
/// @see https://atarilynxdeveloper.wordpress.com/suzy-chip/
/// @see ~docs/plans/lynx-suzy-sprite-engine.md

class LynxSuzy final : public ISerializable {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;
	LynxMemoryManager* _memoryManager = nullptr;
	LynxCart* _cart = nullptr;

	LynxSuzyState _state = {};



	/// <summary>Max collision number encountered during current sprite rendering.
	/// Reset to 0 at start of each sprite; written to depositary (SCBAddr + CollOffset)
	/// after sprite completes. Per Handy: tracks the highest collision number read
	/// from the RAM-based collision buffer at COLLBAS during this sprite's pixels.</summary>
	uint8_t _spriteCollision = 0;

	/// <summary>Pen index remap table (16 entries → palette indices).</summary>
	/// <remarks>
	/// Games can reroute decoded sprite pixels to different palette entries.
	/// Written via registers $FC00-$FC0F (PenIndex 0-15).
	/// </remarks>
	uint8_t _penIndex[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

	// Persistent SCB field values (reused when reload flags are clear)
	int16_t _persistHpos = 0;
	int16_t _persistVpos = 0;
	uint16_t _persistHsize = 0x0100;  // 8.8 fixed: 1.0
	uint16_t _persistVsize = 0x0100;
	int16_t _persistStretch = 0;
	int16_t _persistTilt = 0;

	/// <summary>CPU cycles consumed by sprite bus accesses.</summary>
	/// <remarks>
	/// On real hardware, the CPU stalls while Suzy accesses work RAM.
	/// Each bus read/write costs 1 CPU cycle.
	/// </remarks>
	uint32_t _spriteBusCycles = 0;
	bool _spriteProcessingActive = false;

	/// <summary>Perform 16×16→32-bit hardware multiply (MATHC × MATHE).</summary>
	/// <remarks>
	/// Result stored in MATHG:MATHH (high:low).
	/// If MathAccumulate is set, result is added to accumulator.
	/// </remarks>
	void DoMultiply();

	/// <summary>Perform 32÷16-bit hardware divide (numerator / MATHP).</summary>
	/// <remarks>
	/// Quotient stored in MATHC, remainder in MATHE or accumulator.
	/// </remarks>
	void DoDivide();

	/// <summary>Walk sprite chain starting from SPRCTL1 SCB pointer.</summary>
	void ProcessSpriteChain();

	/// <summary>Render a single sprite from its SCB (Sprite Control Block).</summary>
	/// <param name="scbAddr">Address of SCB header in work RAM.</param>
	void ProcessSprite(uint16_t scbAddr);

	/// <summary>Write one sprite pixel with collision detection.</summary>
	/// <param name="x">Screen X coordinate.</param>
	/// <param name="y">Screen Y coordinate.</param>
	/// <param name="penIndex">Remapped palette index (0-15).</param>
	/// <param name="collNum">Collision number for this sprite (SPRCOLL bits 3:0).</param>
	/// <param name="dontCollide">Per-sprite "don't collide" flag (SPRCOLL bit 5).</param>
	/// <param name="spriteType">Controls visibility and collision behavior.</param>
	void WriteSpritePixel(int x, int y, uint8_t penIndex, uint8_t collNum, bool dontCollide, LynxSpriteType spriteType);

	/// <summary>Decode packed sprite line data into pixel buffer.</summary>
	/// <param name="dataAddr">Current address in sprite data (updated).</param>
	/// <param name="lineEnd">End address for this line's data.</param>
	/// <param name="bpp">Bits per pixel (1, 2, 3, or 4).</param>
	/// <param name="literalMode">True for raw pixel data, false for packed/RLE format.</param>
	/// <param name="pixelBuf">Output buffer for decoded pixels.</param>
	/// <param name="maxPixels">Maximum pixels to decode.</param>
	/// <returns>Number of pixels decoded.</returns>
	int DecodeSpriteLinePixels(uint16_t& dataAddr, uint16_t lineEnd, int bpp, bool literalMode, uint8_t* pixelBuf, int maxPixels);

	[[nodiscard]] __forceinline uint8_t ReadRam(uint16_t addr);
	[[nodiscard]] __forceinline uint16_t ReadRam16(uint16_t addr);
	__forceinline void WriteRam(uint16_t addr, uint8_t value);

public:
	/// <summary>Initialize Suzy with system references.</summary>
	void Init(Emulator* emu, LynxConsole* console, LynxMemoryManager* memoryManager, LynxCart* cart);

	/// <summary>Read Suzy register ($FC00-$FCFF offset).</summary>
	[[nodiscard]] uint8_t ReadRegister(uint8_t addr);

	/// <summary>Read Suzy register without side effects (for debugger).
	/// Avoids cart address auto-increment on RCART0/RCART1 reads.</summary>
	[[nodiscard]] uint8_t PeekRegister(uint8_t addr) const;

	/// <summary>Write Suzy register ($FC00-$FCFF offset).</summary>
	void WriteRegister(uint8_t addr, uint8_t value);

	/// <summary>Check if sprite engine is currently busy.</summary>
	[[nodiscard]] bool IsSpriteBusy() const { return _state.SpriteBusy; }

	/// <summary>Get current joystick button state.</summary>
	[[nodiscard]] uint8_t GetJoystick() const { return _state.Joystick; }

	/// <summary>Set joystick button state (from controller input).</summary>
	void SetJoystick(uint8_t value) { _state.Joystick = value; }

	/// <summary>Get system switch state (Opt1, Opt2, Pause, Cart0).</summary>
	[[nodiscard]] uint8_t GetSwitches() const { return _state.Switches; }

	/// <summary>Set system switch state.</summary>
	void SetSwitches(uint8_t value) { _state.Switches = value; }

	/// <summary>Get internal state (for debugging/serialization).</summary>
	[[nodiscard]] LynxSuzyState& GetState() { return _state; }

	void Serialize(Serializer& s) override;
};
