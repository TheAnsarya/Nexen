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
}
