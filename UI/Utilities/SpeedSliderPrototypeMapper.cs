using System;

namespace Nexen.Utilities;

/// <summary>
/// Maps a prototype speed slider position to emulation speed behavior.
/// Position semantics are intentionally discrete to reduce accidental precision drift in menu interactions.
/// </summary>
public static class SpeedSliderPrototypeMapper {
	public const int PausePosition = 0;
	public const int QuarterSpeedPosition = 1;
	public const int HalfSpeedPosition = 2;
	public const int NormalSpeedPosition = 3;
	public const int DoubleSpeedPosition = 4;
	public const int TripleSpeedPosition = 5;
	public const int MaximumSpeedPosition = 6;

	public const int MinPosition = PausePosition;
	public const int MaxPosition = MaximumSpeedPosition;

	public static uint[] PresetSpeeds { get; } = [25, 50, 100, 200, 300];

	public static SpeedSliderSelection FromSliderValue(double sliderValue) {
		int position = (int)Math.Round(sliderValue, MidpointRounding.AwayFromZero);
		return FromPosition(position);
	}

	public static SpeedSliderSelection FromPosition(int position) {
		position = Math.Clamp(position, MinPosition, MaxPosition);

		return position switch {
			PausePosition => new SpeedSliderSelection(position, SpeedSliderMode.Pause, 100, "Pause"),
			QuarterSpeedPosition => new SpeedSliderSelection(position, SpeedSliderMode.EmulationSpeed, 25, "25%"),
			HalfSpeedPosition => new SpeedSliderSelection(position, SpeedSliderMode.EmulationSpeed, 50, "50%"),
			NormalSpeedPosition => new SpeedSliderSelection(position, SpeedSliderMode.EmulationSpeed, 100, "100%"),
			DoubleSpeedPosition => new SpeedSliderSelection(position, SpeedSliderMode.EmulationSpeed, 200, "200%"),
			TripleSpeedPosition => new SpeedSliderSelection(position, SpeedSliderMode.EmulationSpeed, 300, "300%"),
			MaximumSpeedPosition => new SpeedSliderSelection(position, SpeedSliderMode.MaximumSpeed, 0, "Maximum"),
			_ => new SpeedSliderSelection(NormalSpeedPosition, SpeedSliderMode.EmulationSpeed, 100, "100%")
		};
	}

	public static int ToPosition(uint emulationSpeed) {
		if (emulationSpeed == 0) {
			return MaximumSpeedPosition;
		}

		if (emulationSpeed <= 25) {
			return QuarterSpeedPosition;
		}
		if (emulationSpeed <= 50) {
			return HalfSpeedPosition;
		}
		if (emulationSpeed <= 100) {
			return NormalSpeedPosition;
		}
		if (emulationSpeed <= 200) {
			return DoubleSpeedPosition;
		}
		return TripleSpeedPosition;
	}
}

public enum SpeedSliderMode {
	Pause,
	EmulationSpeed,
	MaximumSpeed,
}

public readonly record struct SpeedSliderSelection(int Position, SpeedSliderMode Mode, uint EmulationSpeed, string Label);
