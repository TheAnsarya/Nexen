using Xunit;
using Nexen.Interop;
using Nexen.MovieConverter;
using ControllerType = Nexen.Config.ControllerType;

namespace Nexen.Tests.Interop;

/// <summary>
/// Unit tests for InputApi.DecodeControllerState button decoding.
/// Tests console-specific button mappings for all supported controller types.
/// </summary>
public class InputApiDecoderTests {
	#region Test Helpers

	private static ControllerStateInterop CreateState(ControllerType type, params byte[] bytes) {
		var state = new ControllerStateInterop {
			Type = type,
			Port = 0,
			StateSize = (byte)bytes.Length,
			StateBytes = new byte[32]
		};
		Array.Copy(bytes, state.StateBytes, bytes.Length);
		return state;
	}

	#endregion

	#region Lynx Controller Tests

	[Fact]
	public void Decode_LynxController_AllButtonsOff() {
		var state = CreateState(ControllerType.LynxController, 0x00, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.Up);
		Assert.False(result.Down);
		Assert.False(result.Left);
		Assert.False(result.Right);
		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.L);
		Assert.False(result.R);
		Assert.False(result.Start);
	}

	[Fact]
	public void Decode_LynxController_DPadAndFaceButtons() {
		// Lynx: Up=0, Down=1, Left=2, Right=3, A=4, B=5
		var state = CreateState(ControllerType.LynxController, 0x3F, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.True(result.A);
		Assert.True(result.B);
		Assert.False(result.L);
		Assert.False(result.R);
		Assert.False(result.Start);
	}

	[Fact]
	public void Decode_LynxController_OptionAndPauseButtons() {
		// Lynx: Option1=6, Option2=7, Pause=8
		var state = CreateState(ControllerType.LynxController, 0xC0, 0x01);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.L); // Option1
		Assert.True(result.R); // Option2
		Assert.True(result.Start); // Pause
		Assert.False(result.Up);
		Assert.False(result.A);
	}

	#endregion

	#region SNES Controller Tests

	[Fact]
	public void Decode_SnesController_AllButtonsOff() {
		var state = CreateState(ControllerType.SnesController, 0x00, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.X);
		Assert.False(result.Y);
		Assert.False(result.L);
		Assert.False(result.R);
		Assert.False(result.Select);
		Assert.False(result.Start);
		Assert.False(result.Up);
		Assert.False(result.Down);
		Assert.False(result.Left);
		Assert.False(result.Right);
	}

	[Fact]
	public void Decode_SnesController_FaceButtons() {
		// SNES: A=0, B=1, X=2, Y=3
		var state = CreateState(ControllerType.SnesController, 0x0F, 0x00); // bits 0-3 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A);
		Assert.True(result.B);
		Assert.True(result.X);
		Assert.True(result.Y);
		Assert.False(result.Up);
		Assert.False(result.Down);
	}

	[Fact]
	public void Decode_SnesController_ShoulderButtons() {
		// SNES: L=4, R=5
		var state = CreateState(ControllerType.SnesController, 0x30, 0x00); // bits 4-5 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.L);
		Assert.True(result.R);
		Assert.False(result.A);
		Assert.False(result.B);
	}

	[Fact]
	public void Decode_SnesController_DPad() {
		// SNES: Up=8, Down=9, Left=10, Right=11
		var state = CreateState(ControllerType.SnesController, 0x00, 0x0F); // bits 8-11 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.False(result.A);
		Assert.False(result.B);
	}

	[Fact]
	public void Decode_SnesController_StartSelect() {
		// SNES: Select=6, Start=7
		var state = CreateState(ControllerType.SnesController, 0xC0, 0x00); // bits 6-7 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Select);
		Assert.True(result.Start);
		Assert.False(result.A);
	}

	#endregion

	#region NES Controller Tests

	[Theory]
	[InlineData(ControllerType.NesController)]
	[InlineData(ControllerType.FamicomController)]
	[InlineData(ControllerType.FamicomControllerP2)]
	public void Decode_NesController_AllButtonsOff(ControllerType type) {
		var state = CreateState(type, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.Select);
		Assert.False(result.Start);
		Assert.False(result.Up);
		Assert.False(result.Down);
		Assert.False(result.Left);
		Assert.False(result.Right);
	}

	[Fact]
	public void Decode_NesController_DPad() {
		// NES: Up=0, Down=1, Left=2, Right=3
		var state = CreateState(ControllerType.NesController, 0x0F); // bits 0-3 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.False(result.A);
		Assert.False(result.B);
	}

	[Fact]
	public void Decode_NesController_ActionButtons() {
		// NES: Start=4, Select=5, B=6, A=7
		var state = CreateState(ControllerType.NesController, 0xF0); // bits 4-7 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Start);
		Assert.True(result.Select);
		Assert.True(result.B);
		Assert.True(result.A);
		Assert.False(result.Up);
	}

	#endregion

	#region Game Boy / GBA Tests

	[Theory]
	[InlineData(ControllerType.GameboyController)]
	[InlineData(ControllerType.GbaController)]
	public void Decode_GameboyController_AllButtonsOff(ControllerType type) {
		var state = CreateState(type, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.Select);
		Assert.False(result.Start);
		Assert.False(result.Up);
		Assert.False(result.Down);
		Assert.False(result.Left);
		Assert.False(result.Right);
	}

	[Fact]
	public void Decode_GameboyController_FaceButtons() {
		// GB: A=0, B=1, Select=2, Start=3
		var state = CreateState(ControllerType.GameboyController, 0x0F); // bits 0-3 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A);
		Assert.True(result.B);
		Assert.True(result.Select);
		Assert.True(result.Start);
		Assert.False(result.Up);
	}

	[Fact]
	public void Decode_GameboyController_DPad() {
		// GB: Right=4, Left=5, Up=6, Down=7
		var state = CreateState(ControllerType.GameboyController, 0xF0); // bits 4-7 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Right);
		Assert.True(result.Left);
		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.False(result.A);
	}

	[Fact]
	public void Decode_GbaController_ShoulderButtons() {
		// GBA: L=8, R=9 (bits in second byte)
		var state = CreateState(ControllerType.GbaController, 0x00, 0x03); // bits 8-9 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.L);
		Assert.True(result.R);
		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.Up);
	}

	[Fact]
	public void Decode_GbaController_AllButtons() {
		// GBA: A=0, B=1, Select=2, Start=3, Right=4, Left=5, Up=6, Down=7, L=8, R=9
		var state = CreateState(ControllerType.GbaController, 0xFF, 0x03); // all 10 bits set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A);
		Assert.True(result.B);
		Assert.True(result.Select);
		Assert.True(result.Start);
		Assert.True(result.Right);
		Assert.True(result.Left);
		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.L);
		Assert.True(result.R);
	}

	[Fact]
	public void Decode_GbaController_LOnly() {
		// GBA: L=8 only
		var state = CreateState(ControllerType.GbaController, 0x00, 0x01); // bit 8 only
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.L);
		Assert.False(result.R);
		Assert.False(result.A);
	}

	[Fact]
	public void Decode_GbaController_ROnly() {
		// GBA: R=9 only
		var state = CreateState(ControllerType.GbaController, 0x00, 0x02); // bit 9 only
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.L);
		Assert.True(result.R);
		Assert.False(result.A);
	}

	[Fact]
	public void Decode_GameboyController_NoShoulderButtons() {
		// GB controller should NOT have L/R even with bits 8-9 set
		// (GB has no shoulder buttons — only 8 bits of state)
		var state = CreateState(ControllerType.GameboyController, 0xFF);
		var result = InputApi.DecodeControllerState(state);

		// GB only decodes 8 bits; L and R should remain false
		Assert.False(result.L);
		Assert.False(result.R);
	}

	#endregion

	#region SMS Controller Tests

	[Theory]
	[InlineData(ControllerType.SmsController)]
	[InlineData(ControllerType.ColecoVisionController)]
	public void Decode_SmsController_AllButtonsOff(ControllerType type) {
		var state = CreateState(type, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.Up);
		Assert.False(result.Down);
		Assert.False(result.Left);
		Assert.False(result.Right);
	}

	[Fact]
	public void Decode_SmsController_DPad() {
		// SMS: Up=0, Down=1, Left=2, Right=3
		var state = CreateState(ControllerType.SmsController, 0x0F); // bits 0-3 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.False(result.A);
	}

	[Fact]
	public void Decode_SmsController_ActionButtons() {
		// SMS: Button1=4, Button2=5 -> A, B
		var state = CreateState(ControllerType.SmsController, 0x30); // bits 4-5 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A); // Button 1
		Assert.True(result.B); // Button 2
		Assert.False(result.Up);
	}

	#endregion

	#region PC Engine Controller Tests

	[Theory]
	[InlineData(ControllerType.PceController)]
	[InlineData(ControllerType.PceAvenuePad6)]
	public void Decode_PceController_AllButtonsOff(ControllerType type) {
		var state = CreateState(type, 0x00, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.Select);
		Assert.False(result.Start);
		Assert.False(result.Up);
		Assert.False(result.Down);
		Assert.False(result.Left);
		Assert.False(result.Right);
	}

	[Fact]
	public void Decode_PceController_FaceButtons() {
		// PCE: I=0, II=1, Select=2, Run=3
		var state = CreateState(ControllerType.PceController, 0x0F); // bits 0-3 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A);     // I -> A
		Assert.True(result.B);     // II -> B
		Assert.True(result.Select);
		Assert.True(result.Start); // Run -> Start
		Assert.False(result.Up);
	}

	[Fact]
	public void Decode_PceController_DPad() {
		// PCE: Up=4, Down=5, Left=6, Right=7
		var state = CreateState(ControllerType.PceController, 0xF0); // bits 4-7 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.False(result.A);
	}

	[Fact]
	public void Decode_PceAvenuePad6_ExtraButtons() {
		// PCE 6-button: III=8, IV=9, V=10, VI=11
		var state = CreateState(ControllerType.PceAvenuePad6, 0x00, 0x0F); // bits 8-11 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.X);  // III
		Assert.True(result.Y);  // IV
		Assert.True(result.L);  // V
		Assert.True(result.R);  // VI
		Assert.False(result.A); // I not pressed
	}

	#endregion

	#region WonderSwan Controller Tests

	[Theory]
	[InlineData(ControllerType.WsController)]
	[InlineData(ControllerType.WsControllerVertical)]
	public void Decode_WsController_AllButtonsOff(ControllerType type) {
		var state = CreateState(type, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.Start);
		Assert.False(result.Up);
		Assert.False(result.Down);
		Assert.False(result.Left);
		Assert.False(result.Right);
	}

	[Fact]
	public void Decode_WsController_DPad() {
		// WS: Up=0, Down=1, Left=2, Right=3
		var state = CreateState(ControllerType.WsController, 0x0F); // bits 0-3 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.False(result.A);
	}

	[Fact]
	public void Decode_WsController_ActionButtons() {
		// WS: Start=4, A=5, B=6
		var state = CreateState(ControllerType.WsController, 0x70); // bits 4-6 set
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Start);
		Assert.True(result.A);
		Assert.True(result.B);
		Assert.False(result.Up);
	}

	#endregion

	#region Genesis Controller Tests

	[Fact]
	public void Decode_GenesisController_ThreeButtonLayout() {
		// Genesis: Up=0, Down=1, Left=2, Right=3, A=4, B=5, C=6, Start=7
		var state = CreateState(ControllerType.GenesisController, 0xFF, 0x00);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.True(result.A);
		Assert.True(result.B);
		Assert.True(result.X); // C
		Assert.True(result.Start);
		Assert.False(result.Y);
		Assert.False(result.L);
		Assert.False(result.R);
		Assert.False(result.Select);
	}

	[Fact]
	public void Decode_GenesisController_SixButtonExtras() {
		// Extras: X=8, Y=9, Z=10, Mode=11
		var state = CreateState(ControllerType.GenesisController, 0x00, 0x0F);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Y);      // X
		Assert.True(result.L);      // Y
		Assert.True(result.R);      // Z
		Assert.True(result.Select); // Mode
		Assert.False(result.Up);
		Assert.False(result.A);
		Assert.False(result.X);
		Assert.False(result.Start);
	}

	#endregion

	#region Atari 2600 Controller Tests

	[Fact]
	public void Decode_Atari2600Joystick_DPadAndFire() {
		// A2600 joystick: Up=0, Down=1, Left=2, Right=3, Fire=4
		var state = CreateState(ControllerType.Atari2600Joystick, 0x1F);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.True(result.A); // Fire
		Assert.False(result.B);
	}

	[Fact]
	public void Decode_Atari2600Driving_LeftRightFire() {
		// A2600 driving: Left=0, Right=1, Fire=2
		var state = CreateState(ControllerType.Atari2600DrivingController, 0x07);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.True(result.A); // Fire
		Assert.False(result.Up);
		Assert.False(result.Down);
	}

	[Fact]
	public void Decode_Atari2600BoosterGrip_MapsAllConfiguredButtons() {
		// A2600 booster: Fire=0, Trigger=1, Booster=2, Up=3, Down=4, Left=5, Right=6
		var state = CreateState(ControllerType.Atari2600BoosterGrip, 0x7F);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A); // Fire
		Assert.True(result.B); // Trigger
		Assert.True(result.X); // Booster
		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
	}

	[Fact]
	public void Decode_Atari2600Keypad_MapsAllTwelveBits() {
		// A2600 keypad bits 0-11 -> A/B/X/Y/L/R/Up/Down/Left/Right/Select/Start
		var state = CreateState(ControllerType.Atari2600Keypad, 0xFF, 0x0F);
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A);
		Assert.True(result.B);
		Assert.True(result.X);
		Assert.True(result.Y);
		Assert.True(result.L);
		Assert.True(result.R);
		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.True(result.Select);
		Assert.True(result.Start);
	}

	#endregion

	#region Edge Cases

	[Fact]
	public void Decode_EmptyState_ReturnsDefaultInput() {
		var state = new ControllerStateInterop {
			Type = ControllerType.SnesController,
			Port = 0,
			StateSize = 0,
			StateBytes = new byte[32]
		};
		var result = InputApi.DecodeControllerState(state);

		Assert.False(result.A);
		Assert.False(result.B);
		Assert.False(result.Up);
		Assert.False(result.Down);
	}

	[Fact]
	public void Decode_UnknownController_UsesFallback() {
		// Uses a controller type that falls through to default
		var state = CreateState(ControllerType.None, 0xFF);
		var result = InputApi.DecodeControllerState(state);

		// Default fallback: Up=0, Down=1, Left=2, Right=3, A=4, B=5, Start=6, Select=7
		Assert.True(result.Up);
		Assert.True(result.Down);
		Assert.True(result.Left);
		Assert.True(result.Right);
		Assert.True(result.A);
		Assert.True(result.B);
		Assert.True(result.Start);
		Assert.True(result.Select);
	}

	[Fact]
	public void Decode_SingleByteState_DecodesCorrectly() {
		// Only 1 byte provided, second byte defaults to 0
		var state = CreateState(ControllerType.SnesController, 0xFF); // bits 0-7 only
		var result = InputApi.DecodeControllerState(state);

		Assert.True(result.A);      // bit 0
		Assert.True(result.B);      // bit 1
		Assert.True(result.X);      // bit 2
		Assert.True(result.Y);      // bit 3
		Assert.True(result.L);      // bit 4
		Assert.True(result.R);      // bit 5
		Assert.True(result.Select); // bit 6
		Assert.True(result.Start);  // bit 7
		Assert.False(result.Up);    // bit 8 (not set)
	}

	#endregion

	#region Controller Type Mapping

	[Theory]
	[InlineData(ControllerType.Atari2600Joystick, Nexen.MovieConverter.ControllerType.Atari2600Joystick)]
	[InlineData(ControllerType.Atari2600Paddle, Nexen.MovieConverter.ControllerType.Atari2600Paddle)]
	[InlineData(ControllerType.Atari2600Keypad, Nexen.MovieConverter.ControllerType.Atari2600Keypad)]
	[InlineData(ControllerType.Atari2600DrivingController, Nexen.MovieConverter.ControllerType.Atari2600DrivingController)]
	[InlineData(ControllerType.Atari2600BoosterGrip, Nexen.MovieConverter.ControllerType.Atari2600BoosterGrip)]
	[InlineData(ControllerType.SnesController, Nexen.MovieConverter.ControllerType.Gamepad)]
	[InlineData(ControllerType.NesController, Nexen.MovieConverter.ControllerType.Gamepad)]
	[InlineData(ControllerType.GameboyController, Nexen.MovieConverter.ControllerType.Gamepad)]
	[InlineData(ControllerType.LynxController, Nexen.MovieConverter.ControllerType.Gamepad)]
	[InlineData(ControllerType.SnesMouse, Nexen.MovieConverter.ControllerType.Mouse)]
	[InlineData(ControllerType.SuperScope, Nexen.MovieConverter.ControllerType.SuperScope)]
	[InlineData(ControllerType.NesZapper, Nexen.MovieConverter.ControllerType.Zapper)]
	public void MapRuntimeToMovieControllerType_MapsCorrectly(ControllerType runtime, Nexen.MovieConverter.ControllerType expected) {
		Nexen.MovieConverter.ControllerType result = InputApi.MapRuntimeToMovieControllerType(runtime);
		Assert.Equal(expected, result);
	}

	[Fact]
	public void DecodeControllerState_SetsTypeFromMapping() {
		var state = CreateState(ControllerType.Atari2600Joystick, 0x01);
		Nexen.MovieConverter.ControllerInput result = InputApi.DecodeControllerState(state);
		Assert.Equal(Nexen.MovieConverter.ControllerType.Atari2600Joystick, result.Type);
	}

	#endregion
}
