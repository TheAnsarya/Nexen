#pragma once
#include "pch.h"

/// <summary>
/// Cursor image types for emulated mouse devices.
/// </summary>
enum class CursorImage {
	Hidden, ///< Hide cursor (fullscreen gaming)
	Arrow,  ///< Standard arrow cursor
	Cross   ///< Crosshair for light gun/mouse precision
};

/// <summary>
/// System mouse state snapshot.
/// </summary>
/// <remarks>
/// Represents raw OS mouse state at polling time.
/// Used for:
/// - NES Zapper light gun
/// - SNES Super Scope
/// - Game Boy Pocket Camera (mouse input)
/// - Mouse emulation for keyboard/gamepad
/// </remarks>
struct SystemMouseState {
	int32_t XPosition; ///< X coordinate relative to window
	int32_t YPosition; ///< Y coordinate relative to window
	bool LeftButton;   ///< Left button state
	bool RightButton;  ///< Right button state
	bool MiddleButton; ///< Middle button state
	bool Button4;      ///< Thumb button 1 (back)
	bool Button5;      ///< Thumb button 2 (forward)
};

/// <summary>
/// Interface for system mouse management and cursor control.
/// Implemented by platform-specific UI backends.
/// </summary>
/// <remarks>
/// Implementations:
/// - Windows: Win32 API (SetCursorPos, SetCapture, SetCursor)
/// - Linux: X11/Wayland (XWarpPointer, pointer grabs)
/// - macOS: Cocoa (CGWarpMouseCursorPosition, mouse tracking)
/// - SDL: SDL_SetRelativeMouseMode, SDL_WarpMouseInWindow
///
/// Mouse capture mode:
/// - Used for relative mouse movement (FPS games, mouse emulation)
/// - Confines cursor to window bounds
/// - Hides cursor and provides raw input
/// - Released when window loses focus
///
/// Thread model:
/// - All methods called from UI/render thread
/// - GetSystemMouseState() polled every frame
/// - CaptureMouse/ReleaseMouse on focus change
/// </remarks>
class IMouseManager {
public:
	virtual ~IMouseManager() {}

	/// <summary>
	/// Get current system mouse state.
	/// </summary>
	/// <param name="rendererHandle">Platform window/renderer handle</param>
	/// <returns>Mouse state snapshot</returns>
	/// <remarks>
	/// Coordinates:
	/// - Relative to rendererHandle window
	/// - (0,0) = top-left corner
	/// - May be negative or outside window bounds
	/// </remarks>
	virtual SystemMouseState GetSystemMouseState(void* rendererHandle) = 0;

	/// <summary>
	/// Capture mouse input to specified rectangle.
	/// </summary>
	/// <param name="x">Capture rectangle X (window coords)</param>
	/// <param name="y">Capture rectangle Y (window coords)</param>
	/// <param name="width">Capture rectangle width</param>
	/// <param name="height">Capture rectangle height</param>
	/// <param name="rendererHandle">Platform window handle</param>
	/// <returns>True if capture successful</returns>
	/// <remarks>
	/// Capture effects:
	/// - Confine cursor to rectangle bounds
	/// - Hide system cursor (show custom cursor overlay)
	/// - Provide raw mouse input (pixel-perfect tracking)
	///
	/// Used for:
	/// - Light gun calibration (Zapper, Super Scope)
	/// - Relative mouse mode (FPS camera control)
	/// - Touch screen emulation
	/// </remarks>
	virtual bool CaptureMouse(int32_t x, int32_t y, int32_t width, int32_t height, void* rendererHandle) = 0;

	/// <summary>
	/// Release mouse capture (restore normal cursor).
	/// </summary>
	virtual void ReleaseMouse() = 0;

	/// <summary>
	/// Set system mouse cursor position.
	/// </summary>
	/// <param name="x">Screen X coordinate</param>
	/// <param name="y">Screen Y coordinate</param>
	/// <remarks>
	/// Used for:
	/// - Centering cursor in capture mode
	/// - Warping cursor for infinite mouse movement
	/// - Touch screen position emulation
	/// </remarks>
	virtual void SetSystemMousePosition(int32_t x, int32_t y) = 0;

	/// <summary>
	/// Set cursor image/style.
	/// </summary>
	/// <param name="cursor">Cursor type (hidden/arrow/crosshair)</param>
	virtual void SetCursorImage(CursorImage cursor) = 0;

	/// <summary>
	/// Get UI pixel scale factor (for HiDPI displays).
	/// </summary>
	/// <returns>Scale factor (1.0 = normal, 2.0 = Retina/4K)</returns>
	/// <remarks>
	/// Used for:
	/// - Converting mouse coordinates to emulated coordinates
	/// - Scaling light gun position
	/// - HiDPI-aware hit detection
	/// </remarks>
	virtual double GetPixelScale() = 0;
};
