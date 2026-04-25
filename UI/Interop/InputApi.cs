using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Nexen.Config;
using Nexen.Utilities;
using ControllerInput = Nexen.MovieConverter.ControllerInput;

[assembly: InternalsVisibleTo("Nexen.Tests")]
[assembly: InternalsVisibleTo("Nexen.Benchmarks")]

namespace Nexen.Interop;
public sealed class InputApi {
	private const string DllPath = EmuApi.DllName;

	[DllImport(DllPath)] public static extern void SetKeyState(UInt16 scanCode, [MarshalAs(UnmanagedType.I1)] bool pressed);
	[DllImport(DllPath)] public static extern void ResetKeyState();

	[DllImport(DllPath)] public static extern void SetMouseMovement(Int16 x, Int16 y);
	[DllImport(DllPath)] public static extern void SetMousePosition(double x, double y);
	[DllImport(DllPath)] public static extern void DisableAllKeys([MarshalAs(UnmanagedType.I1)] bool disabled);
	[DllImport(DllPath)] public static extern void UpdateInputDevices();

	[DllImport(DllPath)] public static extern UInt16 GetKeyCode([MarshalAs(UnmanagedType.LPUTF8Str)] string keyName);

	[DllImport(DllPath)] public static extern void ResetLagCounter();

	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool HasControlDevice(ControllerType type);

	[DllImport(DllPath)] public static extern SystemMouseState GetSystemMouseState(IntPtr rendererHandle);
	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool CaptureMouse(Int32 x, Int32 y, Int32 width, Int32 height, IntPtr rendererHandle);
	[DllImport(DllPath)] public static extern void ReleaseMouse();
	[DllImport(DllPath)] public static extern void SetSystemMousePosition(Int32 x, Int32 y);
	[DllImport(DllPath)] public static extern void SetCursorImage(CursorImage cursor);
	[DllImport(DllPath)] public static extern double GetPixelScale();

	[DllImport(DllPath, EntryPoint = "GetKeyName")] private static extern IntPtr GetKeyNameWrapper(UInt16 key, IntPtr outKeyName, Int32 maxLength);
	public unsafe static string GetKeyName(UInt16 key) {
		byte[] outKeyName = new byte[1000];
		fixed (byte* ptr = outKeyName) {
			InputApi.GetKeyNameWrapper(key, (IntPtr)ptr, outKeyName.Length);
			return Utf8Utilities.PtrToStringUtf8((IntPtr)ptr);
		}
	}

	[DllImport(DllPath, EntryPoint = "GetPressedKeys")] private static extern void GetPressedKeysWrapper(IntPtr keyBuffer);
	public static unsafe List<UInt16> GetPressedKeys() {
		UInt16[] keyBuffer = new UInt16[3];
		fixed (UInt16* ptr = keyBuffer) {
			InputApi.GetPressedKeysWrapper((IntPtr)ptr);
		}

		List<UInt16> keys = [];
		for (int i = 0; i < 3; i++) {
			if (keyBuffer[i] != 0) {
				keys.Add(keyBuffer[i]);
			}
		}

		return keys;
	}

	[DllImport(DllPath, EntryPoint = "GetControllerStates")]
	private static extern void GetControllerStatesWrapper(IntPtr buffer, out uint count);

	[DllImport(DllPath)] public static extern void SetAtari2600ConsoleSwitches([MarshalAs(UnmanagedType.I1)] bool select, [MarshalAs(UnmanagedType.I1)] bool reset);

	/// <summary>
	/// Gets the current controller states from the emulator.
	/// </summary>
	/// <returns>Array of controller input states decoded to ControllerInput format.</returns>
	public static unsafe ControllerInput[] GetControllerStates() {
		var buffer = new ControllerStateInterop[8];
#pragma warning disable CS8500 // Takes address of managed type (required for P/Invoke interop)
		fixed (ControllerStateInterop* ptr = buffer) {
#pragma warning restore CS8500
			GetControllerStatesWrapper((IntPtr)ptr, out uint count);

			var result = new ControllerInput[Math.Min(count, 4)];
			for (int i = 0; i < result.Length; i++) {
				result[i] = DecodeControllerState(buffer[i]);
			}
			return result;
		}
	}

	/// <summary>
	/// Decodes raw controller state bytes to ControllerInput based on controller type.
	/// </summary>
	internal static ControllerInput DecodeControllerState(ControllerStateInterop state) {
		var input = new ControllerInput();
		if (state.StateSize == 0) {
			return input;
		}

		input.Type = MapRuntimeToMovieControllerType(state.Type);

		// Get button bits from first 2 bytes (most controllers use 16 bits max)
		ushort buttons = state.StateBytes[0];
		if (state.StateSize > 1) {
			buttons |= (ushort)(state.StateBytes[1] << 8);
		}

		// Decode based on controller type
		switch (state.Type) {
			case ControllerType.SnesController:
				// SNES: A=0, B=1, X=2, Y=3, L=4, R=5, Select=6, Start=7, Up=8, Down=9, Left=10, Right=11
				input.A = (buttons & (1 << 0)) != 0;
				input.B = (buttons & (1 << 1)) != 0;
				input.X = (buttons & (1 << 2)) != 0;
				input.Y = (buttons & (1 << 3)) != 0;
				input.L = (buttons & (1 << 4)) != 0;
				input.R = (buttons & (1 << 5)) != 0;
				input.Select = (buttons & (1 << 6)) != 0;
				input.Start = (buttons & (1 << 7)) != 0;
				input.Up = (buttons & (1 << 8)) != 0;
				input.Down = (buttons & (1 << 9)) != 0;
				input.Left = (buttons & (1 << 10)) != 0;
				input.Right = (buttons & (1 << 11)) != 0;
				break;

			case ControllerType.NesController:
			case ControllerType.FamicomController:
			case ControllerType.FamicomControllerP2:
				// NES: Up=0, Down=1, Left=2, Right=3, Start=4, Select=5, B=6, A=7
				input.Up = (buttons & (1 << 0)) != 0;
				input.Down = (buttons & (1 << 1)) != 0;
				input.Left = (buttons & (1 << 2)) != 0;
				input.Right = (buttons & (1 << 3)) != 0;
				input.Start = (buttons & (1 << 4)) != 0;
				input.Select = (buttons & (1 << 5)) != 0;
				input.B = (buttons & (1 << 6)) != 0;
				input.A = (buttons & (1 << 7)) != 0;
				break;

			case ControllerType.GameboyController:
				// GB: A=0, B=1, Select=2, Start=3, Right=4, Left=5, Up=6, Down=7
				input.A = (buttons & (1 << 0)) != 0;
				input.B = (buttons & (1 << 1)) != 0;
				input.Select = (buttons & (1 << 2)) != 0;
				input.Start = (buttons & (1 << 3)) != 0;
				input.Right = (buttons & (1 << 4)) != 0;
				input.Left = (buttons & (1 << 5)) != 0;
				input.Up = (buttons & (1 << 6)) != 0;
				input.Down = (buttons & (1 << 7)) != 0;
				break;

			case ControllerType.GbaController:
				// GBA: A=0, B=1, Select=2, Start=3, Right=4, Left=5, Up=6, Down=7, L=8, R=9
				input.A = (buttons & (1 << 0)) != 0;
				input.B = (buttons & (1 << 1)) != 0;
				input.Select = (buttons & (1 << 2)) != 0;
				input.Start = (buttons & (1 << 3)) != 0;
				input.Right = (buttons & (1 << 4)) != 0;
				input.Left = (buttons & (1 << 5)) != 0;
				input.Up = (buttons & (1 << 6)) != 0;
				input.Down = (buttons & (1 << 7)) != 0;
				input.L = (buttons & (1 << 8)) != 0;
				input.R = (buttons & (1 << 9)) != 0;
				break;

			case ControllerType.SmsController:
			case ControllerType.ColecoVisionController:
				// SMS: Up=0, Down=1, Left=2, Right=3, Button1=4, Button2=5
				input.Up = (buttons & (1 << 0)) != 0;
				input.Down = (buttons & (1 << 1)) != 0;
				input.Left = (buttons & (1 << 2)) != 0;
				input.Right = (buttons & (1 << 3)) != 0;
				input.A = (buttons & (1 << 4)) != 0;  // Button 1
				input.B = (buttons & (1 << 5)) != 0;  // Button 2
				break;

			case ControllerType.PceController:
			case ControllerType.PceAvenuePad6:
				// PCE: I=0, II=1, Select=2, Run=3, Up=4, Down=5, Left=6, Right=7
				input.A = (buttons & (1 << 0)) != 0; // I -> A
				input.B = (buttons & (1 << 1)) != 0; // II -> B
				input.Select = (buttons & (1 << 2)) != 0;
				input.Start = (buttons & (1 << 3)) != 0; // Run -> Start
				input.Up = (buttons & (1 << 4)) != 0;
				input.Down = (buttons & (1 << 5)) != 0;
				input.Left = (buttons & (1 << 6)) != 0;
				input.Right = (buttons & (1 << 7)) != 0;
				// 6-button pad has additional buttons at bits 8-11
				if (state.Type == ControllerType.PceAvenuePad6) {
					input.X = (buttons & (1 << 8)) != 0;  // III
					input.Y = (buttons & (1 << 9)) != 0;  // IV
					input.L = (buttons & (1 << 10)) != 0; // V
					input.R = (buttons & (1 << 11)) != 0; // VI
				}
				break;

			case ControllerType.WsController:
			case ControllerType.WsControllerVertical:
				// WS: Up=0, Down=1, Left=2, Right=3, Start=4, A=5, B=6
				input.Up = (buttons & (1 << 0)) != 0;
				input.Down = (buttons & (1 << 1)) != 0;
				input.Left = (buttons & (1 << 2)) != 0;
				input.Right = (buttons & (1 << 3)) != 0;
				input.Start = (buttons & (1 << 4)) != 0;
				input.A = (buttons & (1 << 5)) != 0;
				input.B = (buttons & (1 << 6)) != 0;
				break;

			case ControllerType.Atari2600Joystick:
			case ControllerType.Atari2600Paddle:
				// A2600 joystick/paddle: Up=0, Down=1, Left=2, Right=3, Fire=4
				input.Up = (buttons & (1 << 0)) != 0;
				input.Down = (buttons & (1 << 1)) != 0;
				input.Left = (buttons & (1 << 2)) != 0;
				input.Right = (buttons & (1 << 3)) != 0;
				input.A = (buttons & (1 << 4)) != 0; // Fire
				break;

			case ControllerType.Atari2600DrivingController:
				// A2600 driving: Left=0, Right=1, Fire=2
				input.Left = (buttons & (1 << 0)) != 0;
				input.Right = (buttons & (1 << 1)) != 0;
				input.A = (buttons & (1 << 2)) != 0; // Fire
				break;

			case ControllerType.Atari2600BoosterGrip:
				// A2600 booster grip: Fire=0, Trigger=1, Booster=2, Up=3, Down=4, Left=5, Right=6
				input.A = (buttons & (1 << 0)) != 0; // Fire
				input.B = (buttons & (1 << 1)) != 0; // Trigger
				input.X = (buttons & (1 << 2)) != 0; // Booster
				input.Up = (buttons & (1 << 3)) != 0;
				input.Down = (buttons & (1 << 4)) != 0;
				input.Left = (buttons & (1 << 5)) != 0;
				input.Right = (buttons & (1 << 6)) != 0;
				break;

			case ControllerType.Atari2600Keypad:
				// A2600 keypad (12 keys) mapped to available ControllerInput fields.
				input.A = (buttons & (1 << 0)) != 0;      // 1
				input.B = (buttons & (1 << 1)) != 0;      // 2
				input.X = (buttons & (1 << 2)) != 0;      // 3
				input.Y = (buttons & (1 << 3)) != 0;      // 4
				input.L = (buttons & (1 << 4)) != 0;      // 5
				input.R = (buttons & (1 << 5)) != 0;      // 6
				input.Up = (buttons & (1 << 6)) != 0;     // 7
				input.Down = (buttons & (1 << 7)) != 0;   // 8
				input.Left = (buttons & (1 << 8)) != 0;   // 9
				input.Right = (buttons & (1 << 9)) != 0;  // *
				input.Select = (buttons & (1 << 10)) != 0; // 0
				input.Start = (buttons & (1 << 11)) != 0; // #
				break;

			case ControllerType.LynxController:
				// Lynx: Up=0, Down=1, Left=2, Right=3, A=4, B=5, Option1=6, Option2=7, Pause=8
				input.Up = (buttons & (1 << 0)) != 0;
				input.Down = (buttons & (1 << 1)) != 0;
				input.Left = (buttons & (1 << 2)) != 0;
				input.Right = (buttons & (1 << 3)) != 0;
				input.A = (buttons & (1 << 4)) != 0;
				input.B = (buttons & (1 << 5)) != 0;
				input.L = (buttons & (1 << 6)) != 0; // Option1
				input.R = (buttons & (1 << 7)) != 0; // Option2
				input.Start = (buttons & (1 << 8)) != 0; // Pause
				break;

			case ControllerType.GenesisController:
				// Genesis: U=0, D=1, L=2, R=3, A=4, B=5, C=6, Start=7, X=8, Y=9, Z=10, Mode=11
				// Map C to ControllerInput.X and 6-button extras to Y/L/R/Select.
				input.Up = (buttons & (1 << 0)) != 0;
				input.Down = (buttons & (1 << 1)) != 0;
				input.Left = (buttons & (1 << 2)) != 0;
				input.Right = (buttons & (1 << 3)) != 0;
				input.A = (buttons & (1 << 4)) != 0;
				input.B = (buttons & (1 << 5)) != 0;
				input.X = (buttons & (1 << 6)) != 0; // C
				input.Start = (buttons & (1 << 7)) != 0;
				input.Y = (buttons & (1 << 8)) != 0; // X
				input.L = (buttons & (1 << 9)) != 0; // Y
				input.R = (buttons & (1 << 10)) != 0; // Z
				input.Select = (buttons & (1 << 11)) != 0; // Mode
				break;

			default:
				// Generic fallback - assume NES-like layout
				input.Up = (buttons & (1 << 0)) != 0;
				input.Down = (buttons & (1 << 1)) != 0;
				input.Left = (buttons & (1 << 2)) != 0;
				input.Right = (buttons & (1 << 3)) != 0;
				input.A = (buttons & (1 << 4)) != 0;
				input.B = (buttons & (1 << 5)) != 0;
				input.Start = (buttons & (1 << 6)) != 0;
				input.Select = (buttons & (1 << 7)) != 0;
				break;
		}

		return input;
	}

	/// <summary>
	/// Maps a runtime ControllerType (Nexen.Config) to a movie ControllerType (MovieConverter).
	/// </summary>
	internal static MovieConverter.ControllerType MapRuntimeToMovieControllerType(ControllerType runtimeType) {
		return runtimeType switch {
			ControllerType.Atari2600Joystick => MovieConverter.ControllerType.Atari2600Joystick,
			ControllerType.Atari2600Paddle => MovieConverter.ControllerType.Atari2600Paddle,
			ControllerType.Atari2600Keypad => MovieConverter.ControllerType.Atari2600Keypad,
			ControllerType.Atari2600DrivingController => MovieConverter.ControllerType.Atari2600DrivingController,
			ControllerType.Atari2600BoosterGrip => MovieConverter.ControllerType.Atari2600BoosterGrip,
			ControllerType.SnesController => MovieConverter.ControllerType.Gamepad,
			ControllerType.NesController or ControllerType.FamicomController or ControllerType.FamicomControllerP2 => MovieConverter.ControllerType.Gamepad,
			ControllerType.GameboyController or ControllerType.GbaController => MovieConverter.ControllerType.Gamepad,
			ControllerType.SnesMouse => MovieConverter.ControllerType.Mouse,
			ControllerType.SuperScope => MovieConverter.ControllerType.SuperScope,
			ControllerType.NesZapper or ControllerType.FamicomZapper => MovieConverter.ControllerType.Zapper,
			ControllerType.GenesisController => MovieConverter.ControllerType.Gamepad,
			ControllerType.SmsController or ControllerType.ColecoVisionController => MovieConverter.ControllerType.Gamepad,
			ControllerType.PceController or ControllerType.PceAvenuePad6 => MovieConverter.ControllerType.Gamepad,
			ControllerType.WsController or ControllerType.WsControllerVertical => MovieConverter.ControllerType.Gamepad,
			ControllerType.LynxController => MovieConverter.ControllerType.Gamepad,
			_ => MovieConverter.ControllerType.Gamepad,
		};
	}
}

/// <summary>
/// Interop structure for marshalling controller state from C++.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public struct ControllerStateInterop {
	public ControllerType Type;
	public byte Port;
	public byte StateSize;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
	public byte[] StateBytes;
}

public enum CursorImage {
	Hidden,
	Arrow,
	Cross
}

[StructLayout(LayoutKind.Sequential)]
public struct SystemMouseState {
	public Int32 XPosition;
	public Int32 YPosition;
	[MarshalAs(UnmanagedType.I1)] public bool LeftButton;
	[MarshalAs(UnmanagedType.I1)] public bool RightButton;
	[MarshalAs(UnmanagedType.I1)] public bool MiddleButton;
	[MarshalAs(UnmanagedType.I1)] public bool Button4;
	[MarshalAs(UnmanagedType.I1)] public bool Button5;
}
