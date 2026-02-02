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
}
