#pragma once
#include "pch.h"

/// <summary>
/// State for the SNES hardware multiplication and division unit.
/// </summary>
/// <remarks>
/// The SNES has a hardware ALU that performs:
/// - 8x8 bit unsigned multiplication (16-bit result)
/// - 16/8 bit unsigned division (16-bit quotient, 16-bit remainder)
/// Operations are performed by the CPU between instructions,
/// so results may not be immediately available.
/// </remarks>
struct AluState {
	/// <summary>First multiplication operand (8-bit, written to $4202).</summary>
	uint8_t MultOperand1 = 0;

	/// <summary>Second multiplication operand (8-bit, written to $4203, triggers multiply).</summary>
	uint8_t MultOperand2 = 0;

	/// <summary>
	/// Multiplication result (16-bit) or division remainder.
	/// Read from $4216-$4217 (RDMPYL/RDMPYH or RDDIVL/RDDIVH).
	/// </summary>
	uint16_t MultOrRemainderResult = 0;

	/// <summary>Division dividend (16-bit, written to $4204-$4205).</summary>
	uint16_t Dividend = 0;

	/// <summary>Division divisor (8-bit, written to $4206, triggers divide).</summary>
	uint8_t Divisor = 0;

	/// <summary>Division quotient result (16-bit, read from $4214-$4215).</summary>
	uint16_t DivResult = 0;
};

/// <summary>
/// State for SNES internal CPU registers (memory-mapped I/O).
/// </summary>
/// <remarks>
/// These registers control system-level features:
/// - NMI and IRQ configuration
/// - Auto joypad reading
/// - FastROM access speed
/// - Controller data latching
/// </remarks>
struct InternalRegisterState {
	/// <summary>
	/// True to enable automatic controller reading during VBlank.
	/// When enabled, controller data is read into $4218-$421F.
	/// </summary>
	bool EnableAutoJoypadRead = false;

	/// <summary>
	/// True to enable FastROM access (3.58MHz for $80-$FF banks).
	/// When false, all ROM access is SlowROM (2.68MHz).
	/// </summary>
	bool EnableFastRom = false;

	/// <summary>True to enable NMI on VBlank ($4200 bit 7).</summary>
	bool EnableNmi = false;

	/// <summary>True to enable horizontal IRQ ($4200 bit 4).</summary>
	bool EnableHorizontalIrq = false;

	/// <summary>True to enable vertical IRQ ($4200 bit 5).</summary>
	bool EnableVerticalIrq = false;

	/// <summary>
	/// Horizontal IRQ trigger position (0-339 dots).
	/// Written to $4207-$4208 (HTIMEL/HTIMEH).
	/// </summary>
	uint16_t HorizontalTimer = 0;

	/// <summary>
	/// Vertical IRQ trigger position (0-261/311 scanlines).
	/// Written to $4209-$420A (VTIMEL/VTIMEH).
	/// </summary>
	uint16_t VerticalTimer = 0;

	/// <summary>
	/// Programmable I/O port output value ($4201 WRIO).
	/// Controls accent light and controller latch on some systems.
	/// </summary>
	uint8_t IoPortOutput = 0;

	/// <summary>
	/// Auto-read controller data for ports 1-4.
	/// [0]: Controller 1 ($4218-$4219)
	/// [1]: Controller 2 ($421A-$421B)
	/// [2]: Controller 3 ($421C-$421D, multitap)
	/// [3]: Controller 4 ($421E-$421F, multitap)
	/// </summary>
	uint16_t ControllerData[4] = {};
};
