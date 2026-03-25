namespace Nexen.MovieConverter.Tests;

/// <summary>
/// Tests for ControllerInput class
/// </summary>
public class ControllerInputTests {
	[Fact]
	public void HasInput_ReturnsFalseWhenNoButtonsPressed() {
		var input = new ControllerInput();

		Assert.False(input.HasInput);
	}

	[Fact]
	public void HasInput_ReturnsTrueWhenButtonPressed() {
		var input = new ControllerInput { A = true };

		Assert.True(input.HasInput);
	}

	[Fact]
	public void ButtonBits_EncodesCorrectly() {
		var input = new ControllerInput {
			A = true,    // bit 0
			B = true,    // bit 1
			Start = true // bit 6
		};

		ushort expected = (1 << 0) | (1 << 1) | (1 << 6);
		Assert.Equal(expected, input.ButtonBits);
	}

	[Fact]
	public void ButtonBits_HandlesAllButtons() {
		var input = new ControllerInput {
			A = true, B = true, X = true, Y = true,
			L = true, R = true, Start = true, Select = true,
			Up = true, Down = true, Left = true, Right = true
		};

		// All 12 standard buttons set
		Assert.Equal(0x0FFF, input.ButtonBits);
	}

	[Fact]
	public void Clone_CreatesDeepCopy() {
		var input = new ControllerInput {
			A = true,
			B = true,
			MouseX = 100,
			MouseY = 50
		};

		var clone = input.Clone();

		Assert.Equal(input.A, clone.A);
		Assert.Equal(input.B, clone.B);
		Assert.Equal(input.MouseX, clone.MouseX);
		Assert.Equal(input.MouseY, clone.MouseY);

		// Verify it's a copy
		clone.A = false;
		Assert.NotEqual(input.A, clone.A);
	}

	[Fact]
	public void ToSmvFormat_EncodesCorrectly() {
		var input = new ControllerInput {
			B = true,     // SMV bit 15
			Y = true,     // SMV bit 14
			Start = true, // SMV bit 12
			Up = true,    // SMV bit 11
			Right = true, // SMV bit 8
			A = true,     // SMV bit 7
			X = true,     // SMV bit 6
			L = true,     // SMV bit 5
			R = true      // SMV bit 4
		};

		ushort smvFormat = input.ToSmvFormat();

		// Verify bits are set according to SMV/SNES controller format
		Assert.True((smvFormat & 0x8000) != 0); // B
		Assert.True((smvFormat & 0x4000) != 0); // Y
		Assert.True((smvFormat & 0x1000) != 0); // Start
		Assert.True((smvFormat & 0x0800) != 0); // Up
		Assert.True((smvFormat & 0x0100) != 0); // Right
		Assert.True((smvFormat & 0x0080) != 0); // A
		Assert.True((smvFormat & 0x0040) != 0); // X
		Assert.True((smvFormat & 0x0020) != 0); // L
		Assert.True((smvFormat & 0x0010) != 0); // R
	}

	[Fact]
	public void FromSmvFormat_DecodesCorrectly() {
		// B, Y, Start, Up, Right
		ushort smvFormat = 0xD900;

		var input = ControllerInput.FromSmvFormat(smvFormat);

		Assert.True(input.B);
		Assert.True(input.Y);
		Assert.True(input.Start);
		Assert.True(input.Up);
		Assert.True(input.Right);
		Assert.False(input.A);
		Assert.False(input.X);
	}

	[Fact]
	public void ToFm2Format_EncodesCorrectly() {
		var input = new ControllerInput {
			A = true,
			B = true,
			Start = true,
			Select = true,
			Up = true
		};

		string fm2Format = input.ToFm2Format();

		// FM2 format: RLDUTSBA (dots for unpressed)
		Assert.Contains("A", fm2Format);
		Assert.Contains("B", fm2Format);
		Assert.Contains("S", fm2Format);
		Assert.Contains("T", fm2Format);
		Assert.Contains("U", fm2Format);
	}

	[Fact]
	public void FromFm2Format_DecodesCorrectly() {
		// FM2 format: RLDUSTBA (Right, Left, Down, Up, Start, Select, B, A)
		string fm2Format = "....TSBA"; // Start, Select, B, A pressed

		var input = ControllerInput.FromFm2Format(fm2Format);

		Assert.True(input.A);
		Assert.True(input.B);
		Assert.True(input.Select);
		Assert.True(input.Start);
		Assert.False(input.Up);
		Assert.False(input.Down);
	}

	[Fact]
	public void Equality_ComparesButtonsCorrectly() {
		var input1 = new ControllerInput { A = true, B = true };
		var input2 = new ControllerInput { A = true, B = true };
		var input3 = new ControllerInput { A = true, B = false };

		Assert.True(input1.Equals(input2));
		Assert.False(input1.Equals(input3));
	}

	[Fact]
	public void SetButton_AtariAliases_MapToExpectedButtons() {
		var input = new ControllerInput();

		input.SetButton("FIRE", true);
		input.SetButton("TRIGGER", true);
		input.SetButton("BOOSTER", true);

		Assert.True(input.A);
		Assert.True(input.B);
		Assert.True(input.C);
		Assert.True(input.GetButton("FIRE"));
		Assert.True(input.GetButton("TRIGGER"));
		Assert.True(input.GetButton("BOOSTER"));
	}

	[Fact]
	public void SetButton_AtariKeypadMappings_AreRoundTrippable() {
		var input = new ControllerInput();

		input.SetButton("1", true);
		input.SetButton("2", true);
		input.SetButton("3", true);
		input.SetButton("4", true);
		input.SetButton("5", true);
		input.SetButton("6", true);
		input.SetButton("7", true);
		input.SetButton("8", true);
		input.SetButton("9", true);
		input.SetButton("STAR", true);
		input.SetButton("0", true);
		input.SetButton("POUND", true);

		Assert.True(input.GetButton("1"));
		Assert.True(input.GetButton("2"));
		Assert.True(input.GetButton("3"));
		Assert.True(input.GetButton("4"));
		Assert.True(input.GetButton("5"));
		Assert.True(input.GetButton("6"));
		Assert.True(input.GetButton("7"));
		Assert.True(input.GetButton("8"));
		Assert.True(input.GetButton("9"));
		Assert.True(input.GetButton("STAR"));
		Assert.True(input.GetButton("0"));
		Assert.True(input.GetButton("POUND"));
	}

	#region Equals Covers All Fields

	[Fact]
	public void Equals_Null_ReturnsFalse() {
		var input = new ControllerInput { A = true };
		Assert.False(input.Equals(null));
	}

	[Fact]
	public void Equals_SameReference_ReturnsTrue() {
		var input = new ControllerInput { A = true };
		Assert.True(input.Equals(input));
	}

	[Fact]
	public void Equals_DifferentMouseDeltaX_ReturnsFalse() {
		var a = new ControllerInput { MouseDeltaX = 10 };
		var b = new ControllerInput { MouseDeltaX = -5 };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentMouseDeltaY_ReturnsFalse() {
		var a = new ControllerInput { MouseDeltaY = 7 };
		var b = new ControllerInput { MouseDeltaY = null };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentMouseButton1_ReturnsFalse() {
		var a = new ControllerInput { MouseButton1 = true };
		var b = new ControllerInput { MouseButton1 = false };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentMouseButton2_ReturnsFalse() {
		var a = new ControllerInput { MouseButton2 = true };
		var b = new ControllerInput { MouseButton2 = null };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentMouseButton3_ReturnsFalse() {
		var a = new ControllerInput { MouseButton3 = false };
		var b = new ControllerInput { MouseButton3 = true };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentAnalogRX_ReturnsFalse() {
		var a = new ControllerInput { AnalogRX = 50 };
		var b = new ControllerInput { AnalogRX = -50 };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentAnalogRY_ReturnsFalse() {
		var a = new ControllerInput { AnalogRY = 127 };
		var b = new ControllerInput { AnalogRY = null };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentTriggerL_ReturnsFalse() {
		var a = new ControllerInput { TriggerL = 255 };
		var b = new ControllerInput { TriggerL = 0 };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentTriggerR_ReturnsFalse() {
		var a = new ControllerInput { TriggerR = 128 };
		var b = new ControllerInput { TriggerR = null };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentPaddlePosition_ReturnsFalse() {
		var a = new ControllerInput { PaddlePosition = 100 };
		var b = new ControllerInput { PaddlePosition = 200 };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentPowerPadButtons_ReturnsFalse() {
		var a = new ControllerInput { PowerPadButtons = 0x00ff };
		var b = new ControllerInput { PowerPadButtons = 0xff00 };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentKeyboardData_ReturnsFalse() {
		var a = new ControllerInput { KeyboardData = new byte[] { 1, 2, 3 } };
		var b = new ControllerInput { KeyboardData = new byte[] { 1, 2, 4 } };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_KeyboardData_NullVsNonNull_ReturnsFalse() {
		var a = new ControllerInput { KeyboardData = null };
		var b = new ControllerInput { KeyboardData = new byte[] { 1 } };
		Assert.False(a.Equals(b));
	}

	[Fact]
	public void Equals_KeyboardData_BothNull_ReturnsTrue() {
		var a = new ControllerInput { KeyboardData = null };
		var b = new ControllerInput { KeyboardData = null };
		Assert.True(a.Equals(b));
	}

	[Fact]
	public void Equals_KeyboardData_BothSame_ReturnsTrue() {
		var a = new ControllerInput { KeyboardData = new byte[] { 0xab, 0xcd } };
		var b = new ControllerInput { KeyboardData = new byte[] { 0xab, 0xcd } };
		Assert.True(a.Equals(b));
	}

	[Fact]
	public void Equals_AllFieldsMatch_ReturnsTrue() {
		var a = CreateFullInput();
		var b = CreateFullInput();
		Assert.True(a.Equals(b));
	}

	[Fact]
	public void Equals_DifferentType_ReturnsFalse() {
		var a = new ControllerInput { Type = ControllerType.Gamepad };
		var b = new ControllerInput { Type = ControllerType.Mouse };
		Assert.False(a.Equals(b));
	}

	#endregion

	#region GetHashCode Consistency

	[Fact]
	public void GetHashCode_EqualObjects_SameHash() {
		var a = CreateFullInput();
		var b = CreateFullInput();
		Assert.Equal(a.GetHashCode(), b.GetHashCode());
	}

	[Fact]
	public void GetHashCode_DifferentObjects_DifferentHash() {
		var a = new ControllerInput { A = true, MouseDeltaX = 10 };
		var b = new ControllerInput { A = true, MouseDeltaX = -10 };
		// Hash codes may collide, but for these distinct inputs they should differ
		Assert.NotEqual(a.GetHashCode(), b.GetHashCode());
	}

	[Fact]
	public void GetHashCode_DifferentAnalogTriggers_DifferentHash() {
		var a = new ControllerInput { TriggerL = 0, TriggerR = 255 };
		var b = new ControllerInput { TriggerL = 255, TriggerR = 0 };
		Assert.NotEqual(a.GetHashCode(), b.GetHashCode());
	}

	[Fact]
	public void GetHashCode_DefaultInput_Consistent() {
		var a = new ControllerInput();
		var b = new ControllerInput();
		Assert.Equal(a.GetHashCode(), b.GetHashCode());
	}

	#endregion

	#region Operator Equality

	[Fact]
	public void OperatorEquals_BothNull_ReturnsTrue() {
		ControllerInput? a = null;
		ControllerInput? b = null;
		Assert.True(a == b);
	}

	[Fact]
	public void OperatorEquals_OneNull_ReturnsFalse() {
		var a = new ControllerInput { A = true };
		ControllerInput? b = null;
		Assert.False(a == b);
		Assert.True(a != b);
	}

	[Fact]
	public void OperatorEquals_EqualInputs_ReturnsTrue() {
		var a = new ControllerInput { A = true, MouseDeltaX = 5 };
		var b = new ControllerInput { A = true, MouseDeltaX = 5 };
		Assert.True(a == b);
		Assert.False(a != b);
	}

	#endregion

	private static ControllerInput CreateFullInput() {
		return new ControllerInput {
			A = true, B = true, Start = true,
			Type = ControllerType.Gamepad,
			MouseX = 100, MouseY = 200,
			MouseDeltaX = 5, MouseDeltaY = -3,
			MouseButton1 = true, MouseButton2 = false, MouseButton3 = null,
			AnalogX = 64, AnalogY = -32,
			AnalogRX = 10, AnalogRY = -10,
			TriggerL = 128, TriggerR = 64,
			PaddlePosition = 150,
			PowerPadButtons = 0x1234,
			KeyboardData = new byte[] { 0x01, 0x02, 0x03 }
		};
	}

	#region SetButton / GetButton

	[Theory]
	[InlineData("A", "a")]
	[InlineData("B", "b")]
	[InlineData("X", "x")]
	[InlineData("Y", "y")]
	[InlineData("L", "l")]
	[InlineData("R", "r")]
	[InlineData("Up", "up")]
	[InlineData("Down", "down")]
	[InlineData("Left", "left")]
	[InlineData("Right", "right")]
	[InlineData("Start", "start")]
	[InlineData("Select", "select")]
	public void SetButton_SetsAndGetButton_Reads(string upper, string lower) {
		var input = new ControllerInput();

		Assert.False(input.GetButton(upper));
		input.SetButton(upper, true);
		Assert.True(input.GetButton(upper));
		Assert.True(input.GetButton(lower));

		input.SetButton(lower, false);
		Assert.False(input.GetButton(upper));
	}

	[Theory]
	[InlineData("UP")]
	[InlineData("DOWN")]
	[InlineData("LEFT")]
	[InlineData("RIGHT")]
	[InlineData("START")]
	[InlineData("SELECT")]
	public void SetButton_AllCapsVariant_Works(string button) {
		var input = new ControllerInput();
		input.SetButton(button, true);
		Assert.True(input.GetButton(button));
	}

	[Fact]
	public void GetButton_UnknownButton_ReturnsFalse() {
		var input = new ControllerInput { A = true, B = true };
		Assert.False(input.GetButton("Unknown"));
		Assert.False(input.GetButton(""));
	}

	[Fact]
	public void SetButton_UnknownButton_NoOp() {
		var input = new ControllerInput();
		input.SetButton("Unknown", true);
		Assert.False(input.HasInput);
	}

	#endregion
}
