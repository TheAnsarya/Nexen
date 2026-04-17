using BenchmarkDotNet.Attributes;
using Nexen.Controls;
using Nexen.MovieConverter;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks cached button code lookup path for piano roll rendering.
/// </summary>
[MemoryDiagnoser]
public class PianoRollButtonLookupBenchmarks {
	private readonly ControllerInput _input = new() {
		A = true,
		B = true,
		Start = true,
		Left = true
	};

	private int[] _codes = null!;

	[GlobalSetup]
	public void Setup() {
		_codes = [
			PianoRollControl.GetButtonCode("A"),
			PianoRollControl.GetButtonCode("B"),
			PianoRollControl.GetButtonCode("X"),
			PianoRollControl.GetButtonCode("Y"),
			PianoRollControl.GetButtonCode("L"),
			PianoRollControl.GetButtonCode("R"),
			PianoRollControl.GetButtonCode("UP"),
			PianoRollControl.GetButtonCode("DOWN"),
			PianoRollControl.GetButtonCode("LEFT"),
			PianoRollControl.GetButtonCode("RIGHT"),
			PianoRollControl.GetButtonCode("START"),
			PianoRollControl.GetButtonCode("SELECT")
		];
	}

	[Benchmark]
	public int LookupByCode_12Buttons() {
		int pressed = 0;
		for (int i = 0; i < _codes.Length; i++) {
			if (PianoRollControl.GetButtonStateByCode(_input, _codes[i])) {
				pressed++;
			}
		}

		return pressed;
	}
}
