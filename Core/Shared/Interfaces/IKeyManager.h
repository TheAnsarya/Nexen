#pragma once
#include "pch.h"

/// <summary>
/// Mouse button identifiers.
/// </summary>
enum class MouseButton {
	LeftButton = 0,
	RightButton = 1,
	MiddleButton = 2,
	Button4 = 3, ///< Mouse thumb button 1 (back)
	Button5 = 4  ///< Mouse thumb button 2 (forward)
};

/// <summary>
/// Mouse position in absolute and relative coordinates.
/// </summary>
struct MousePosition {
	int16_t X;        ///< Absolute X position in pixels
	int16_t Y;        ///< Absolute Y position in pixels
	double RelativeX; ///< Relative X (0.0-1.0, normalized to screen)
	double RelativeY; ///< Relative Y (0.0-1.0, normalized to screen)
};

/// <summary>
/// Mouse movement delta since last poll.
/// </summary>
struct MouseMovement {
	int16_t dx; ///< Horizontal movement in pixels
	int16_t dy; ///< Vertical movement in pixels
};

/// <summary>
/// Interface for input device management (keyboard, mouse, gamepad).
/// Implemented by platform-specific input backends.
/// </summary>
/// <remarks>
/// Implementations:
/// - Windows: DirectInput, XInput, Raw Input
/// - Linux: evdev, X11
/// - macOS: IOKit, Carbon
/// - SDL: Cross-platform fallback
///
/// Key code mapping:
/// - 0x000-0x1FF: Keyboard scan codes
/// - 0x200-0x2FF: Mouse buttons (BaseMouseButtonIndex + MouseButton)
/// - 0x1000+: Gamepad buttons/axes (BaseGamepadIndex + device-specific)
///
/// Thread model:
/// - RefreshState() called from emulation thread every frame
/// - UpdateDevices() called when device list changes (hotplug)
/// - SetKeyState() for replay/scripting (overrides physical input)
/// </remarks>
class IKeyManager {
public:
	static constexpr int BaseMouseButtonIndex = 0x200; ///< Mouse button key code offset
	static constexpr int BaseGamepadIndex = 0x1000;    ///< Gamepad key code offset

	virtual ~IKeyManager() {}

	/// <summary>
	/// Refresh input state from all devices.
	/// </summary>
	/// <remarks>
	/// Called every frame to poll input devices.
	/// Updates internal key state table.
	/// </remarks>
	virtual void RefreshState() = 0;

	/// <summary>
	/// Re-enumerate input devices (handle hotplug).
	/// </summary>
	/// <remarks>
	/// Called when:
	/// - Device connected/disconnected
	/// - Settings changed (device selection)
	/// - Initial startup
	/// </remarks>
	virtual void UpdateDevices() = 0;

	/// <summary>
	/// Check if mouse button is currently pressed.
	/// </summary>
	/// <param name="button">Mouse button ID</param>
	/// <returns>True if pressed</returns>
	virtual bool IsMouseButtonPressed(MouseButton button) = 0;

	/// <summary>
	/// Check if key/button is currently pressed.
	/// </summary>
	/// <param name="keyCode">Key code (keyboard/mouse/gamepad)</param>
	/// <returns>True if pressed</returns>
	virtual bool IsKeyPressed(uint16_t keyCode) = 0;

	/// <summary>
	/// Get analog axis position (joystick/trigger).
	/// </summary>
	/// <param name="keyCode">Axis key code</param>
	/// <returns>Axis value (-32768 to 32767) or nullopt if not an axis</returns>
	virtual optional<int16_t> GetAxisPosition(uint16_t keyCode) { return std::nullopt; }

	/// <summary>
	/// Get list of all currently pressed keys.
	/// </summary>
	/// <returns>Key code array</returns>
	/// <remarks>
	/// Used for:
	/// - Key binding UI (wait for key press)
	/// - Input display overlay
	/// - Debugging
	/// </remarks>
	virtual vector<uint16_t> GetPressedKeys() = 0;

	/// <summary>
	/// Get human-readable name for key code.
	/// </summary>
	/// <param name="keyCode">Key code</param>
	/// <returns>Key name (e.g., "A", "Left Mouse Button", "Gamepad A")</returns>
	virtual string GetKeyName(uint16_t keyCode) = 0;

	/// <summary>
	/// Get key code from name string.
	/// </summary>
	/// <param name="keyName">Key name</param>
	/// <returns>Key code or 0 if not found</returns>
	virtual uint16_t GetKeyCode(string keyName) = 0;

	/// <summary>
	/// Override key state (for scripting/replay).
	/// </summary>
	/// <param name="scanCode">Key code to override</param>
	/// <param name="state">New state (true=pressed, false=released)</param>
	/// <returns>True if override applied</returns>
	/// <remarks>
	/// Used for:
	/// - TAS movie playback
	/// - Lua script input injection
	/// - Network play input synchronization
	/// </remarks>
	virtual bool SetKeyState(uint16_t scanCode, bool state) = 0;

	/// <summary>
	/// Reset all key states to unpressed.
	/// </summary>
	virtual void ResetKeyState() = 0;

	/// <summary>
	/// Enable/disable input polling.
	/// </summary>
	/// <param name="disabled">True to disable (ignore input)</param>
	/// <remarks>
	/// Used when:
	/// - Emulator loses focus (background pause)
	/// - Input focus in UI dialogs
	/// - Netplay client mode (server controls input)
	/// </remarks>
	virtual void SetDisabled(bool disabled) = 0;

	/// <summary>
	/// Set gamepad force feedback/rumble.
	/// </summary>
	/// <param name="magnitudeRight">Right motor intensity (0-65535)</param>
	/// <param name="magnitudeLeft">Left motor intensity (0-65535)</param>
	/// <remarks>
	/// Used for:
	/// - N64 Rumble Pak emulation
	/// - Game Boy Player rumble
	/// - Future rumble support
	/// </remarks>
	virtual void SetForceFeedback(uint16_t magnitudeRight, uint16_t magnitudeLeft) {}
};