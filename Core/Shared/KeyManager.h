#pragma once
#include "pch.h"
#include "Shared/Interfaces/IKeyManager.h"
#include "Utilities/SimpleLock.h"

class Emulator;
class EmuSettings;

/// <summary>
/// Global keyboard and mouse input manager.
/// Interfaces with platform-specific keyboard/mouse backends.
/// </summary>
/// <remarks>
/// Architecture:
/// - Static class (one instance per process)
/// - IKeyManager backend provides platform-specific input (Windows, Linux, etc.)
/// - Caches key state for fast polling
/// - Supports keyboard, mouse, and gamepad axes
///
/// Input types:
/// - Keyboard: Key press/release, key names
/// - Mouse: Button presses, position, relative movement
/// - Gamepad: Analog stick axes, force feedback
///
/// Usage:
/// <code>
/// KeyManager::RegisterKeyManager(platformBackend);
/// bool pressed = KeyManager::IsKeyPressed(VK_SPACE);
/// MousePosition pos = KeyManager::GetMousePosition();
/// vector<uint16_t> keys = KeyManager::GetPressedKeys();
/// </code>
///
/// Force feedback:
/// - Dual-motor rumble support (left/right motors)
/// - SetForceFeedback(magnitude) or SetForceFeedback(right, left)
///
/// Thread safety: All methods synchronized via SimpleLock.
/// </remarks>
class KeyManager {
private:
	static IKeyManager* _keyManager;     ///< Platform-specific backend
	static MousePosition _mousePosition; ///< Current mouse position
	static double _xMouseMovement;       ///< Accumulated X movement
	static double _yMouseMovement;       ///< Accumulated Y movement
	static EmuSettings* _settings;       ///< Settings reference
	static SimpleLock _lock;             ///< Thread synchronization

public:
	/// <summary>Register platform-specific keyboard/mouse backend</summary>
	static void RegisterKeyManager(IKeyManager* keyManager);

	/// <summary>Set settings reference (for mouse sensitivity, etc.)</summary>
	static void SetSettings(EmuSettings* settings);

	/// <summary>Refresh cached key state from backend</summary>
	static void RefreshKeyState();

	/// <summary>
	/// Check if key currently pressed.
	/// </summary>
	/// <param name="keyCode">Platform-specific key code</param>
	/// <returns>True if key down</returns>
	static bool IsKeyPressed(uint16_t keyCode);

	/// <summary>
	/// Get analog axis position (for gamepad sticks/triggers).
	/// </summary>
	/// <param name="keyCode">Axis key code</param>
	/// <returns>Axis value (-32768 to 32767), or empty if not analog</returns>
	static optional<int16_t> GetAxisPosition(uint16_t keyCode);

	/// <summary>
	/// Check if mouse button currently pressed.
	/// </summary>
	/// <param name="button">Mouse button (Left, Right, Middle)</param>
	static bool IsMouseButtonPressed(MouseButton button);

	/// <summary>Get list of all currently pressed keys</summary>
	static vector<uint16_t> GetPressedKeys();

	/// <summary>
	/// Get human-readable key name.
	/// </summary>
	/// <param name="keyCode">Platform-specific key code</param>
	/// <returns>Key name (e.g., "Space", "Enter", "A")</returns>
	static string GetKeyName(uint16_t keyCode);

	/// <summary>
	/// Get key code from name.
	/// </summary>
	/// <param name="keyName">Key name (e.g., "Space", "Enter")</param>
	/// <returns>Platform-specific key code</returns>
	static uint16_t GetKeyCode(string keyName);

	/// <summary>Update connected device list (gamepads, etc.)</summary>
	static void UpdateDevices();

	/// <summary>
	/// Set mouse relative movement (for next GetMouseMovement call).
	/// </summary>
	/// <param name="x">X movement in pixels</param>
	/// <param name="y">Y movement in pixels</param>
	static void SetMouseMovement(int16_t x, int16_t y);

	/// <summary>
	/// Get accumulated mouse movement with sensitivity applied.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="mouseSensitivity">Sensitivity multiplier (0-100)</param>
	/// <returns>Mouse movement (clears accumulator)</returns>
	static MouseMovement GetMouseMovement(Emulator* emu, uint32_t mouseSensitivity);

	/// <summary>
	/// Set absolute mouse position.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="x">X position (0-1, relative to window)</param>
	/// <param name="y">Y position (0-1, relative to window)</param>
	static void SetMousePosition(Emulator* emu, double x, double y);

	/// <summary>Get current mouse position</summary>
	static MousePosition GetMousePosition();

	/// <summary>
	/// Set force feedback (single motor).
	/// </summary>
	/// <param name="magnitude">Rumble strength (0-65535)</param>
	static void SetForceFeedback(uint16_t magnitude);

	/// <summary>
	/// Set force feedback (dual motor).
	/// </summary>
	/// <param name="magnitudeRight">Right motor strength (0-65535)</param>
	/// <param name="magnitudeLeft">Left motor strength (0-65535)</param>
	static void SetForceFeedback(uint16_t magnitudeRight, uint16_t magnitudeLeft);
};