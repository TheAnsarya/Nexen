#pragma once
#include "pch.h"
#include "Shared/SettingTypes.h"
#include "Shared/ControlDeviceState.h"
#include "Shared/Interfaces/IKeyManager.h"

class Emulator;
class DebugHud;
class BaseControlManager;

/// <summary>
/// Input display overlay for controllers and mouse.
/// Renders on-screen display of button presses for recording/streaming.
/// </summary>
/// <remarks>
/// Features:
/// - Controller button visualization (all supported types)
/// - Mouse position indicator
/// - Customizable position and appearance
/// - Multi-controller support (up to 8 players)
/// 
/// Rendering:
/// - Draws via DebugHud overlay system
/// - Transparent background with colored outlines
/// - Button press indicators (filled when pressed)
/// - Frame number display
/// 
/// Usage:
/// <code>
/// InputHud hud(&emu, debugHud);
/// hud.DrawControllers(frameSize, controllerStates);
/// hud.DrawMousePosition(mousePos);
/// </code>
/// 
/// Positioning:
/// - Auto-layout for multiple controllers
/// - _xOffset, _yOffset track current drawing position
/// - EndDrawController() advances to next slot
/// 
/// Thread safety: Called from emulation thread only.
/// </remarks>
class InputHud {
private:
	Emulator* _emu = nullptr;     ///< Emulator instance
	DebugHud* _hud = nullptr;     ///< Debug HUD for rendering

	int _xOffset = 0;             ///< Current X drawing position
	int _yOffset = 0;             ///< Current Y drawing position
	int _outlineWidth = 0;        ///< Controller outline width
	int _outlineHeight = 0;       ///< Controller outline height
	int _controllerIndex = 0;     ///< Current controller index (0-7)

	/// <summary>
	/// Draw single controller with button states.
	/// </summary>
	/// <param name="data">Controller button/axis state</param>
	/// <param name="controlManager">Control manager for device info</param>
	void DrawController(ControllerData& data, BaseControlManager* controlManager);

public:
	/// <summary>Construct input HUD for emulator</summary>
	InputHud(Emulator* emu, DebugHud* hud);

	/// <summary>
	/// Draw mouse position indicator.
	/// </summary>
	/// <param name="pos">Mouse position (screen coordinates)</param>
	void DrawMousePosition(MousePosition pos);
	
	/// <summary>
	/// Draw controller outline rectangle.
	/// </summary>
	/// <param name="width">Outline width</param>
	/// <param name="height">Outline height</param>
	void DrawOutline(int width, int height);
	
	/// <summary>
	/// Draw single button indicator.
	/// </summary>
	/// <param name="x">X position relative to controller outline</param>
	/// <param name="y">Y position relative to controller outline</param>
	/// <param name="width">Button width</param>
	/// <param name="height">Button height</param>
	/// <param name="pressed">Button pressed state (filled if true)</param>
	void DrawButton(int x, int y, int width, int height, bool pressed);
	
	/// <summary>
	/// Draw numeric value (for analog sticks, etc.).
	/// </summary>
	/// <param name="number">Number to display</param>
	/// <param name="x">X position</param>
	/// <param name="y">Y position</param>
	void DrawNumber(int number, int x, int y);
	
	/// <summary>Finish drawing current controller, advance to next slot</summary>
	void EndDrawController();

	/// <summary>Get current controller index being drawn</summary>
	[[nodiscard]] int GetControllerIndex() { return _controllerIndex; }

	/// <summary>
	/// Draw all connected controllers.
	/// </summary>
	/// <param name="size">Frame size for layout</param>
	/// <param name="controllerData">Controller states to visualize</param>
	void DrawControllers(FrameInfo size, vector<ControllerData> controllerData);
};
