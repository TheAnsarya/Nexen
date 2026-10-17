#pragma once
#include "pch.h"
#include "Utilities/Serializer.h"
#include "Shared/KeyManager.h"
#include "Shared/BaseControlDevice.h"

/// <summary>
/// MBC7 accelerometer sensor emulation (ADXL202E equivalent).
/// Used in Kirby Tilt 'n' Tumble for tilt-based gameplay.
/// </summary>
/// <remarks>
/// **ADXL202E Specifications:**
/// - Dual-axis accelerometer (X and Y)
/// - ±2g measurement range
/// - Analog output converted to digital by MBC7
///
/// **Digital Interface:**
/// - 16-bit unsigned values per axis
/// - $8000 = center position (no tilt)
/// - Lower values = tilt in one direction
/// - Higher values = tilt in opposite direction
///
/// **Latch Protocol:**
/// 1. Write $55 to $A000-$A00F: Reset values to $8000
/// 2. Write $AA to $A010-$A01F: Latch current tilt values
/// 3. Read $A020-$A05F: Get latched X/Y data
///
/// **Register Reads:**
/// - $A020: X acceleration low byte
/// - $A030: X acceleration high byte
/// - $A040: Y acceleration low byte
/// - $A050: Y acceleration high byte
///
/// **Input Mapping:**
/// Maps gamepad analog stick or motion sensor input to tilt values.
/// Default sensitivity: ±500 units from center for full deflection.
/// </remarks>
class GbMbc7Accelerometer : public BaseControlDevice {
private:
	/// <summary>Input codes for X-axis (left/right tilt).</summary>
	vector<uint16_t> _xAxisCodes;

	/// <summary>Input codes for Y-axis (forward/back tilt).</summary>
	vector<uint16_t> _yAxisCodes;

	/// <summary>Current latched X acceleration (16-bit, $8000 = center).</summary>
	uint16_t _xAccel = 0x8000;

	/// <summary>Current latched Y acceleration (16-bit, $8000 = center).</summary>
	uint16_t _yAccel = 0x8000;

	/// <summary>Values have been latched (prevents re-latching until reset).</summary>
	bool _latched = false;

protected:
	/// <summary>Indicates this device provides coordinate input.</summary>
	/// <returns>Always true for accelerometer.</returns>
	bool HasCoordinates() override { return true; }

	/// <summary>
	/// Reads current tilt from input devices (analog sticks, motion sensors).
	/// </summary>
	void InternalSetStateFromInput() override {
		MouseMovement mov = {};

		// Read X axis from any configured input
		for (uint16_t code : _xAxisCodes) {
			if (code != 0) {
				optional<int16_t> xAxis = KeyManager::GetAxisPosition(code);
				if (xAxis.has_value()) {
					mov.dx = xAxis.value();
				}
			}
		}

		// Read Y axis from any configured input
		for (uint16_t code : _yAxisCodes) {
			if (code != 0) {
				optional<int16_t> yAxis = KeyManager::GetAxisPosition(code);
				if (yAxis.has_value()) {
					mov.dy = yAxis.value();
				}
			}
		}

		SetMovement(mov);
	}

	/// <summary>Serializes accelerometer state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		BaseControlDevice::Serialize(s);
		SV(_xAccel);
		SV(_yAccel);
		SV(_latched);
	}

public:
	/// <summary>
	/// Creates an MBC7 accelerometer with default input mappings.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	GbMbc7Accelerometer(Emulator* emu) : BaseControlDevice(emu, ControllerType::GameboyAccelerometer, BaseControlDevice::MapperInputPort) {
		// TODO add configuration in UI
		// Default to common gamepad analog inputs
		_xAxisCodes = {
		    KeyManager::GetKeyCode("Pad1 LT X"),
		    KeyManager::GetKeyCode("Joy1 X"),
		    KeyManager::GetKeyCode("Pad1 X"),
		};

		_yAxisCodes = {
		    KeyManager::GetKeyCode("Pad1 LT Y"),
		    KeyManager::GetKeyCode("Joy1 Y"),
		    KeyManager::GetKeyCode("Pad1 Y"),
		};
	}

	/// <summary>Not used - accelerometer has no RAM.</summary>
	uint8_t ReadRam(uint16_t addr) override { return 0; }

	/// <summary>Not used - accelerometer has no RAM.</summary>
	void WriteRam(uint16_t addr, uint8_t value) override {}

	/// <summary>
	/// Reads latched accelerometer values.
	/// </summary>
	/// <param name="addr">Register address ($A020-$A05F).</param>
	/// <returns>X or Y acceleration byte.</returns>
	uint8_t Read(uint16_t addr) {
		switch (addr & 0xF0) {
			case 0x20:
				return _xAccel & 0xFF;        // X low byte
			case 0x30:
				return (_xAccel >> 8) & 0xFF; // X high byte
			case 0x40:
				return _yAccel & 0xFF;        // Y low byte
			case 0x50:
				return (_yAccel >> 8) & 0xFF; // Y high byte
		}

		return 0;
	}

	/// <summary>
	/// Handles accelerometer latch protocol.
	/// </summary>
	/// <param name="addr">Control register address.</param>
	/// <param name="value">Control value ($55 = reset, $AA = latch).</param>
	void Write(uint16_t addr, uint8_t value) {
		switch (addr & 0xF0) {
			case 0x00:
				if (value == 0x55) {
					// Reset accelerometer latch to center position
					_xAccel = 0x8000;
					_yAccel = 0x8000;
					_latched = false;
				}
				break;

			case 0x10:
				if (!_latched && value == 0xAA) {
					// Latch current accelerometer values from input
					MouseMovement mov = GetMovement();
					// Convert input to 16-bit tilt values
					// Center is $81D0, inverted X axis
					_xAccel = -(mov.dx / 500) + 0x81D0;
					_yAccel = mov.dy / 500 + 0x81D0;
					_latched = true;
				}
				break;
		}
	}
};
