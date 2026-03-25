using System;
using Avalonia;
using Avalonia.Controls;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for individual controller input state and overrides.
/// </summary>
/// <remarks>
/// <para>
/// This ViewModel manages the input state for a single controller port, providing:
/// <list type="bullet">
///   <item><description>Two-way binding for all standard controller buttons</description></item>
///   <item><description>Console-specific button visibility (shoulder buttons, etc.)</description></item>
///   <item><description>Input override injection into emulation via debug API</description></item>
/// </list>
/// </para>
/// <para>
/// Supports multiple console types with appropriate button layouts: SNES (full layout),
/// NES/GB (no shoulder buttons), SMS (limited buttons), WonderSwan (rotated D-pad).
/// </para>
/// </remarks>
public sealed class ControllerInputViewModel : ViewModelBase {
	/// <summary>Gets or sets the view height in pixels for the controller display.</summary>
	[Reactive] public int ViewHeight { get; set; }

	/// <summary>Gets or sets whether the A button is pressed.</summary>
	[Reactive] public bool ButtonA { get; set; }

	/// <summary>Gets or sets whether the B button is pressed.</summary>
	[Reactive] public bool ButtonB { get; set; }

	/// <summary>Gets or sets whether the X button is pressed (SNES/GBA only).</summary>
	[Reactive] public bool ButtonX { get; set; }

	/// <summary>Gets or sets whether the Y button is pressed (SNES/GBA only).</summary>
	[Reactive] public bool ButtonY { get; set; }

	/// <summary>Gets or sets whether the L shoulder button is pressed.</summary>
	[Reactive] public bool ButtonL { get; set; }

	/// <summary>Gets or sets whether the R shoulder button is pressed.</summary>
	[Reactive] public bool ButtonR { get; set; }

	/// <summary>Gets or sets whether the secondary U button is pressed (WonderSwan Y-pad).</summary>
	[Reactive] public bool ButtonU { get; set; }

	/// <summary>Gets or sets whether the secondary D button is pressed (WonderSwan Y-pad).</summary>
	[Reactive] public bool ButtonD { get; set; }

	/// <summary>Gets or sets whether D-pad Up is pressed.</summary>
	[Reactive] public bool ButtonUp { get; set; }

	/// <summary>Gets or sets whether D-pad Down is pressed.</summary>
	[Reactive] public bool ButtonDown { get; set; }

	/// <summary>Gets or sets whether D-pad Left is pressed.</summary>
	[Reactive] public bool ButtonLeft { get; set; }

	/// <summary>Gets or sets whether D-pad Right is pressed.</summary>
	[Reactive] public bool ButtonRight { get; set; }

	/// <summary>Gets or sets whether the Select button is pressed.</summary>
	[Reactive] public bool ButtonSelect { get; set; }

	/// <summary>Gets or sets whether the Start button is pressed.</summary>
	[Reactive] public bool ButtonStart { get; set; }

	/// <summary>Gets the 1-based controller port index.</summary>
	public int ControllerIndex { get; }

	/// <summary>Gets whether this is an SNES controller layout.</summary>
	public bool IsSnes { get; }

	/// <summary>Gets whether this is a WonderSwan controller layout (has rotated Y-pad).</summary>
	public bool IsWs { get; }

	/// <summary>Gets whether this console type has L/R shoulder buttons.</summary>
	public bool HasShoulderButtons { get; }

	/// <summary>Gets whether this console type has a Select button.</summary>
	public bool HasSelectButton { get; }

	/// <summary>Gets whether this console type has a Start button.</summary>
	public bool HasStartButton { get; }

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public ControllerInputViewModel() : this(ConsoleType.Ws, 0) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="ControllerInputViewModel"/> class.
	/// </summary>
	/// <param name="consoleType">The console type, which determines available buttons and layout.</param>
	/// <param name="index">The 0-based controller port index.</param>
	/// <remarks>
	/// Configures button availability and view height based on console type.
	/// Subscribes to PropertyChanged to automatically apply input overrides when buttons change.
	/// </remarks>
	public ControllerInputViewModel(ConsoleType consoleType, int index) {
		ControllerIndex = index + 1;
		IsSnes = consoleType == ConsoleType.Snes;
		IsWs = consoleType == ConsoleType.Ws;
		HasShoulderButtons = consoleType is ConsoleType.Snes or ConsoleType.Gba;
		HasSelectButton = consoleType is not ConsoleType.Sms and not ConsoleType.Atari2600;
		HasStartButton = (consoleType != ConsoleType.Sms || index == 0) && consoleType != ConsoleType.Atari2600;
		ViewHeight = consoleType != ConsoleType.Ws ? (HasShoulderButtons ? 34 : 30) : 64;

		if (Design.IsDesignMode) {
			return;
		}

		// Subscribe to property changes to automatically update input overrides
		PropertyChanged += ControllerInputViewModel_PropertyChanged;
	}

	/// <summary>
	/// Handles property change events to automatically apply input overrides.
	/// </summary>
	private void ControllerInputViewModel_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e) {
		SetInputOverrides();
	}

	/// <summary>
	/// Applies the current button states as input overrides to the emulation.
	/// </summary>
	/// <remarks>
	/// Sends all button states to the debug API, which will inject them into
	/// the emulated controller input system. This allows testing games with
	/// specific input patterns without physical controller input.
	/// </remarks>
	public void SetInputOverrides() {
		DebugApi.SetInputOverrides((uint)ControllerIndex - 1, new DebugControllerState() {
			A = ButtonA,
			B = ButtonB,
			X = ButtonX,
			Y = ButtonY,
			L = ButtonL,
			R = ButtonR,
			U = ButtonU,
			D = ButtonD,
			Up = ButtonUp,
			Down = ButtonDown,
			Left = ButtonLeft,
			Right = ButtonRight,
			Select = ButtonSelect,
			Start = ButtonStart
		});
	}

	/// <summary>
	/// Toggles a button state when clicked in the UI.
	/// </summary>
	/// <param name="param">The button name as a string (e.g., "A", "B", "Up", "Start").</param>
	/// <remarks>
	/// Used as a command handler for button click events in the controller overlay UI.
	/// </remarks>
	public void OnButtonClick(object param) {
		if (param is string buttonName) {
			switch (buttonName) {
				case "A": ButtonA = !ButtonA; break;
				case "B": ButtonB = !ButtonB; break;
				case "X": ButtonX = !ButtonX; break;
				case "Y": ButtonY = !ButtonY; break;
				case "L": ButtonL = !ButtonL; break;
				case "R": ButtonR = !ButtonR; break;
				case "U": ButtonU = !ButtonU; break;
				case "D": ButtonD = !ButtonD; break;
				case "Up": ButtonUp = !ButtonUp; break;
				case "Down": ButtonDown = !ButtonDown; break;
				case "Left": ButtonLeft = !ButtonLeft; break;
				case "Right": ButtonRight = !ButtonRight; break;
				case "Select": ButtonSelect = !ButtonSelect; break;
				case "Start": ButtonStart = !ButtonStart; break;
			}
		}
	}
}
