using System.Runtime.CompilerServices;

namespace Nexen.MovieConverter;

/// <summary>
/// Input state for a single controller on a single frame.
/// Supports standard gamepads and special input devices.
/// Uses bitfield for efficient storage and comparison.
/// </summary>
public sealed class ControllerInput : IEquatable<ControllerInput> {
	// ========== Standard SNES/NES Buttons ==========

	/// <summary>A button (right face button)</summary>
	public bool A { get; set; }

	/// <summary>B button (bottom face button)</summary>
	public bool B { get; set; }

	/// <summary>X button (top face button, SNES only)</summary>
	public bool X { get; set; }

	/// <summary>Y button (left face button, SNES only)</summary>
	public bool Y { get; set; }

	/// <summary>L shoulder button (SNES/GBA)</summary>
	public bool L { get; set; }

	/// <summary>R shoulder button (SNES/GBA)</summary>
	public bool R { get; set; }

	/// <summary>Start button</summary>
	public bool Start { get; set; }

	/// <summary>Select button</summary>
	public bool Select { get; set; }

	/// <summary>D-pad Up</summary>
	public bool Up { get; set; }

	/// <summary>D-pad Down</summary>
	public bool Down { get; set; }

	/// <summary>D-pad Left</summary>
	public bool Left { get; set; }

	/// <summary>D-pad Right</summary>
	public bool Right { get; set; }

	// ========== Extended Buttons (Genesis, etc.) ==========

	/// <summary>C button (Genesis)</summary>
	public bool C { get; set; }

	/// <summary>Z button (Genesis 6-button)</summary>
	public bool Z { get; set; }

	/// <summary>Mode button (Genesis 6-button)</summary>
	public bool Mode { get; set; }

	// ========== Mouse/Pointer Input ==========

	/// <summary>Mouse X position (for Mouse, Super Scope, etc.)</summary>
	public int? MouseX { get; set; }

	/// <summary>Mouse Y position</summary>
	public int? MouseY { get; set; }

	/// <summary>Mouse X delta (for relative mode)</summary>
	public int? MouseDeltaX { get; set; }

	/// <summary>Mouse Y delta (for relative mode)</summary>
	public int? MouseDeltaY { get; set; }

	/// <summary>Primary mouse button / trigger</summary>
	public bool? MouseButton1 { get; set; }

	/// <summary>Secondary mouse button / cursor</summary>
	public bool? MouseButton2 { get; set; }

	/// <summary>Middle mouse button</summary>
	public bool? MouseButton3 { get; set; }

	// ========== Analog Input ==========

	/// <summary>Analog stick X (-128 to 127)</summary>
	public sbyte? AnalogX { get; set; }

	/// <summary>Analog stick Y (-128 to 127)</summary>
	public sbyte? AnalogY { get; set; }

	/// <summary>Right analog stick X</summary>
	public sbyte? AnalogRX { get; set; }

	/// <summary>Right analog stick Y</summary>
	public sbyte? AnalogRY { get; set; }

	/// <summary>Left trigger (analog)</summary>
	public byte? TriggerL { get; set; }

	/// <summary>Right trigger (analog)</summary>
	public byte? TriggerR { get; set; }

	// ========== Special Input ==========

	/// <summary>Paddle/dial position (0-255)</summary>
	public byte? PaddlePosition { get; set; }

	/// <summary>Power Pad / Mat button states (16 buttons)</summary>
	public ushort? PowerPadButtons { get; set; }

	/// <summary>Keyboard key data</summary>
	public byte[]? KeyboardData { get; set; }

	// ========== Controller Type ==========

	/// <summary>The type of controller for this port</summary>
	public ControllerType Type { get; set; } = ControllerType.Gamepad;

	// ========== Format Conversion ==========

	/// <summary>
	/// Check if any button is pressed
	/// </summary>
	public bool HasInput {
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get => A || B || X || Y || L || R || Start || Select ||
			   Up || Down || Left || Right || C || Z || Mode ||
			   MouseButton1 == true || MouseButton2 == true || MouseButton3 == true ||
			   AnalogX.HasValue || AnalogY.HasValue;
	}

	/// <summary>
	/// Get button state as a 16-bit value (for compact storage)
	/// </summary>
	public ushort ButtonBits {
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		get {
			ushort bits = 0;
			if (A) {
				bits |= 1 << 0;
			}

			if (B) {
				bits |= 1 << 1;
			}

			if (X) {
				bits |= 1 << 2;
			}

			if (Y) {
				bits |= 1 << 3;
			}

			if (L) {
				bits |= 1 << 4;
			}

			if (R) {
				bits |= 1 << 5;
			}

			if (Start) {
				bits |= 1 << 6;
			}

			if (Select) {
				bits |= 1 << 7;
			}

			if (Up) {
				bits |= 1 << 8;
			}

			if (Down) {
				bits |= 1 << 9;
			}

			if (Left) {
				bits |= 1 << 10;
			}

			if (Right) {
				bits |= 1 << 11;
			}

			if (C) {
				bits |= 1 << 12;
			}

			if (Z) {
				bits |= 1 << 13;
			}

			if (Mode) {
				bits |= 1 << 14;
			}

			return bits;
		}
		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		set {
			A = (value & (1 << 0)) != 0;
			B = (value & (1 << 1)) != 0;
			X = (value & (1 << 2)) != 0;
			Y = (value & (1 << 3)) != 0;
			L = (value & (1 << 4)) != 0;
			R = (value & (1 << 5)) != 0;
			Start = (value & (1 << 6)) != 0;
			Select = (value & (1 << 7)) != 0;
			Up = (value & (1 << 8)) != 0;
			Down = (value & (1 << 9)) != 0;
			Left = (value & (1 << 10)) != 0;
			Right = (value & (1 << 11)) != 0;
			C = (value & (1 << 12)) != 0;
			Z = (value & (1 << 13)) != 0;
			Mode = (value & (1 << 14)) != 0;
		}
	}

	/// <summary>
	/// Convert to Nexen/Mesen text format: BYsSUDLRAXLR
	/// </summary>
	/// <returns>12-character string representing button states</returns>
	public string ToNexenFormat() {
		if (Type == ControllerType.None) {
			return "............";
		}

		return string.Create(12, this, static (chars, input) => {
			chars[0] = input.B ? 'B' : '.';
			chars[1] = input.Y ? 'Y' : '.';
			chars[2] = input.Select ? 's' : '.';
			chars[3] = input.Start ? 'S' : '.';
			chars[4] = input.Up ? 'U' : '.';
			chars[5] = input.Down ? 'D' : '.';
			chars[6] = input.Left ? 'L' : '.';
			chars[7] = input.Right ? 'R' : '.';
			chars[8] = input.A ? 'A' : '.';
			chars[9] = input.X ? 'X' : '.';
			chars[10] = input.L ? 'l' : '.';
			chars[11] = input.R ? 'r' : '.';
		});
	}

	/// <summary>
	/// Parse from Nexen/Mesen text format
	/// </summary>
	/// <param name="input">12-character input string</param>
	/// <returns>Parsed ControllerInput</returns>
	public static ControllerInput FromNexenFormat(ReadOnlySpan<char> input) {
		var ctrl = new ControllerInput { Type = ControllerType.Gamepad };

		if (input.Length < 12) {
			return ctrl;
		}

		// Nexen format: BYsSUDLRAXLR
		ctrl.B = input[0] != '.';
		ctrl.Y = input[1] != '.';
		ctrl.Select = input[2] != '.';
		ctrl.Start = input[3] != '.';
		ctrl.Up = input[4] != '.';
		ctrl.Down = input[5] != '.';
		ctrl.Left = input[6] != '.';
		ctrl.Right = input[7] != '.';
		ctrl.A = input[8] != '.';
		ctrl.X = input[9] != '.';
		ctrl.L = input[10] != '.';
		ctrl.R = input[11] != '.';

		return ctrl;
	}

	/// <summary>
	/// Convert to SMV 2-byte binary format
	/// </summary>
	/// <returns>16-bit value representing button states</returns>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public ushort ToSmvFormat() {
		ushort value = 0;

		// SMV bit layout (SNES):
		// Low byte:  ----RLXA
		// High byte: BYsS UDLR

		if (R) {
			value |= 0x10;
		}

		if (L) {
			value |= 0x20;
		}

		if (X) {
			value |= 0x40;
		}

		if (A) {
			value |= 0x80;
		}

		if (Right) {
			value |= 0x0100;
		}

		if (Left) {
			value |= 0x0200;
		}

		if (Down) {
			value |= 0x0400;
		}

		if (Up) {
			value |= 0x0800;
		}

		if (Start) {
			value |= 0x1000;
		}

		if (Select) {
			value |= 0x2000;
		}

		if (Y) {
			value |= 0x4000;
		}

		if (B) {
			value |= 0x8000;
		}

		return value;
	}

	/// <summary>
	/// Parse from SMV 2-byte binary format
	/// </summary>
	/// <param name="value">16-bit input value</param>
	/// <returns>Parsed ControllerInput</returns>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public static ControllerInput FromSmvFormat(ushort value) => new() {
		Type = ControllerType.Gamepad,
		R = (value & 0x10) != 0,
		L = (value & 0x20) != 0,
		X = (value & 0x40) != 0,
		A = (value & 0x80) != 0,
		Right = (value & 0x0100) != 0,
		Left = (value & 0x0200) != 0,
		Down = (value & 0x0400) != 0,
		Up = (value & 0x0800) != 0,
		Start = (value & 0x1000) != 0,
		Select = (value & 0x2000) != 0,
		Y = (value & 0x4000) != 0,
		B = (value & 0x8000) != 0
	};

	/// <summary>
	/// Convert to LSMV text format (similar to Nexen but case-insensitive)
	/// </summary>
	/// <returns>12-character string</returns>
	public string ToLsmvFormat() {
		return string.Create(12, this, static (chars, input) => {
			chars[0] = input.B ? 'B' : '.';
			chars[1] = input.Y ? 'Y' : '.';
			chars[2] = input.Select ? 's' : '.';
			chars[3] = input.Start ? 'S' : '.';
			chars[4] = input.Up ? 'u' : '.';
			chars[5] = input.Down ? 'd' : '.';
			chars[6] = input.Left ? 'l' : '.';
			chars[7] = input.Right ? 'r' : '.';
			chars[8] = input.A ? 'A' : '.';
			chars[9] = input.X ? 'X' : '.';
			chars[10] = input.L ? 'L' : '.';
			chars[11] = input.R ? 'R' : '.';
		});
	}

	/// <summary>
	/// Parse from LSMV text format
	/// </summary>
	/// <param name="input">Input string</param>
	/// <returns>Parsed ControllerInput</returns>
	public static ControllerInput FromLsmvFormat(ReadOnlySpan<char> input) {
		var ctrl = new ControllerInput { Type = ControllerType.Gamepad };

		if (input.Length < 12) {
			return ctrl;
		}

		// LSMV format: BYsSudlrAXLR (case variations)
		ctrl.B = char.ToUpperInvariant(input[0]) == 'B';
		ctrl.Y = char.ToUpperInvariant(input[1]) == 'Y';
		ctrl.Select = char.ToLowerInvariant(input[2]) == 's';
		ctrl.Start = char.ToUpperInvariant(input[3]) == 'S';
		ctrl.Up = char.ToLowerInvariant(input[4]) == 'u';
		ctrl.Down = char.ToLowerInvariant(input[5]) == 'd';
		ctrl.Left = char.ToLowerInvariant(input[6]) == 'l';
		ctrl.Right = char.ToLowerInvariant(input[7]) == 'r';
		ctrl.A = char.ToUpperInvariant(input[8]) == 'A';
		ctrl.X = char.ToUpperInvariant(input[9]) == 'X';
		ctrl.L = char.ToUpperInvariant(input[10]) == 'L';
		ctrl.R = char.ToUpperInvariant(input[11]) == 'R';

		return ctrl;
	}

	/// <summary>
	/// Convert to FM2 text format (NES): RLDUSTBA
	/// </summary>
	/// <returns>8-character string for NES input</returns>
	public string ToFm2Format() {
		Span<char> chars =
		[
			Right ? 'R' : '.',
			Left ? 'L' : '.',
			Down ? 'D' : '.',
			Up ? 'U' : '.',
			Start ? 'T' : '.',  // FM2 uses T for Start
			Select ? 'S' : '.',
			B ? 'B' : '.',
			A ? 'A' : '.',
		];
		return new string(chars);
	}

	/// <summary>
	/// Parse from FM2 text format (NES)
	/// </summary>
	/// <param name="input">8-character input string</param>
	/// <returns>Parsed ControllerInput</returns>
	public static ControllerInput FromFm2Format(ReadOnlySpan<char> input) {
		var ctrl = new ControllerInput { Type = ControllerType.Gamepad };

		if (input.Length < 8) {
			return ctrl;
		}

		// FM2 format: RLDUSTBA
		ctrl.Right = input[0] != '.';
		ctrl.Left = input[1] != '.';
		ctrl.Down = input[2] != '.';
		ctrl.Up = input[3] != '.';
		ctrl.Start = input[4] != '.';
		ctrl.Select = input[5] != '.';
		ctrl.B = input[6] != '.';
		ctrl.A = input[7] != '.';

		return ctrl;
	}

	/// <summary>
	/// Convert to BK2/BizHawk text format
	/// </summary>
	/// <param name="system">Target system type</param>
	/// <returns>BK2 format string</returns>
	public string ToBk2Format(SystemType system) {
		return system switch {
			SystemType.Nes => string.Create(8, this, static (chars, input) => {
				// BK2 NES: UDLRSSBA (Up, Down, Left, Right, Select, Start, B, A)
				chars[0] = input.Up ? 'U' : '.';
				chars[1] = input.Down ? 'D' : '.';
				chars[2] = input.Left ? 'L' : '.';
				chars[3] = input.Right ? 'R' : '.';
				chars[4] = input.Select ? 's' : '.';
				chars[5] = input.Start ? 'S' : '.';
				chars[6] = input.B ? 'B' : '.';
				chars[7] = input.A ? 'A' : '.';
			}),
			SystemType.Snes => string.Create(12, this, static (chars, input) => {
				// BK2 SNES: UDLRsSlrBAXY
				chars[0] = input.Up ? 'U' : '.';
				chars[1] = input.Down ? 'D' : '.';
				chars[2] = input.Left ? 'L' : '.';
				chars[3] = input.Right ? 'R' : '.';
				chars[4] = input.Select ? 's' : '.';
				chars[5] = input.Start ? 'S' : '.';
				chars[6] = input.L ? 'l' : '.';
				chars[7] = input.R ? 'r' : '.';
				chars[8] = input.B ? 'B' : '.';
				chars[9] = input.A ? 'A' : '.';
				chars[10] = input.X ? 'X' : '.';
				chars[11] = input.Y ? 'Y' : '.';
			}),
			SystemType.Genesis => string.Create(8, this, static (chars, input) => {
				// BK2 Genesis 3-button: UDLRSABC
				chars[0] = input.Up ? 'U' : '.';
				chars[1] = input.Down ? 'D' : '.';
				chars[2] = input.Left ? 'L' : '.';
				chars[3] = input.Right ? 'R' : '.';
				chars[4] = input.Start ? 'S' : '.';
				chars[5] = input.A ? 'A' : '.';
				chars[6] = input.B ? 'B' : '.';
				chars[7] = input.C ? 'C' : '.';
			}),
			_ => ToNexenFormat()
		};
	}

	/// <summary>
	/// Parse from BK2/BizHawk text format
	/// </summary>
	/// <param name="input">Input string</param>
	/// <param name="system">Source system type</param>
	/// <returns>Parsed ControllerInput</returns>
	public static ControllerInput FromBk2Format(ReadOnlySpan<char> input, SystemType system) {
		var ctrl = new ControllerInput { Type = ControllerType.Gamepad };

		return system switch {
			SystemType.Nes when input.Length >= 8 => new ControllerInput {
				Type = ControllerType.Gamepad,
				Up = input[0] != '.',
				Down = input[1] != '.',
				Left = input[2] != '.',
				Right = input[3] != '.',
				Select = input[4] != '.',
				Start = input[5] != '.',
				B = input[6] != '.',
				A = input[7] != '.'
			},
			SystemType.Snes when input.Length >= 12 => new ControllerInput {
				Type = ControllerType.Gamepad,
				Up = input[0] != '.',
				Down = input[1] != '.',
				Left = input[2] != '.',
				Right = input[3] != '.',
				Select = input[4] != '.',
				Start = input[5] != '.',
				L = input[6] != '.',
				R = input[7] != '.',
				B = input[8] != '.',
				A = input[9] != '.',
				X = input[10] != '.',
				Y = input[11] != '.'
			},
			SystemType.Genesis when input.Length >= 8 => new ControllerInput {
				Type = ControllerType.Gamepad,
				Up = input[0] != '.',
				Down = input[1] != '.',
				Left = input[2] != '.',
				Right = input[3] != '.',
				Start = input[4] != '.',
				A = input[5] != '.',
				B = input[6] != '.',
				C = input[7] != '.'
			},
			_ => ctrl
		};
	}

	/// <summary>
	/// Create a deep copy of this input
	/// </summary>
	/// <returns>New ControllerInput with same values</returns>
	public ControllerInput Clone() => new() {
		A = A, B = B, X = X, Y = Y,
		L = L, R = R,
		Start = Start, Select = Select,
		Up = Up, Down = Down, Left = Left, Right = Right,
		C = C, Z = Z, Mode = Mode,
		MouseX = MouseX, MouseY = MouseY,
		MouseDeltaX = MouseDeltaX, MouseDeltaY = MouseDeltaY,
		MouseButton1 = MouseButton1, MouseButton2 = MouseButton2, MouseButton3 = MouseButton3,
		AnalogX = AnalogX, AnalogY = AnalogY,
		AnalogRX = AnalogRX, AnalogRY = AnalogRY,
		TriggerL = TriggerL, TriggerR = TriggerR,
		PaddlePosition = PaddlePosition,
		PowerPadButtons = PowerPadButtons,
		KeyboardData = KeyboardData is null ? null : [.. KeyboardData],
		Type = Type
	};

	/// <summary>
	/// Check equality with another ControllerInput
	/// </summary>
	public bool Equals(ControllerInput? other) {
		if (other is null) {
			return false;
		}

		if (ReferenceEquals(this, other)) {
			return true;
		}

		return ButtonBits == other.ButtonBits && Type == other.Type &&
			   MouseX == other.MouseX && MouseY == other.MouseY &&
			   AnalogX == other.AnalogX && AnalogY == other.AnalogY;
	}

	public override bool Equals(object? obj) => Equals(obj as ControllerInput);

	public override int GetHashCode() => HashCode.Combine(ButtonBits, Type, MouseX, MouseY, AnalogX, AnalogY);

	public static bool operator ==(ControllerInput? left, ControllerInput? right) =>
		left is null ? right is null : left.Equals(right);

	public static bool operator !=(ControllerInput? left, ControllerInput? right) => !(left == right);

	public override string ToString() => ToNexenFormat();
}
