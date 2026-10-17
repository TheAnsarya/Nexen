using System.Text;
using BenchmarkDotNet.Attributes;

namespace Nexen.MovieConverter.Benchmarks;

/// <summary>
/// Benchmarks for ControllerInput format conversion operations
/// </summary>
[MemoryDiagnoser]
[SimpleJob]
public class ControllerInputBenchmarks {
	private ControllerInput _input = null!;
	private string _nexenFormat = null!;
	private string _fm2Format = null!;
	private ushort _smvFormat;

	[GlobalSetup]
	public void Setup() {
		_input = new ControllerInput {
			A = true,
			B = true,
			X = true,
			Y = true,
			Start = true,
			Select = true,
			Up = true,
			Right = true,
			L = true,
			R = true,
			Type = ControllerType.Gamepad
		};

		_nexenFormat = _input.ToNexenFormat();
		_smvFormat = _input.ToSmvFormat();
		_fm2Format = _input.ToFm2Format();
	}

	[Benchmark(Baseline = true)]
	public ushort ButtonBits_Get() => _input.ButtonBits;

	[Benchmark]
	public void ButtonBits_Set() => _input.ButtonBits = 0x0FFF;

	[Benchmark]
	public string ToNexenFormat() => _input.ToNexenFormat();

	[Benchmark]
	public ControllerInput FromNexenFormat() => ControllerInput.FromNexenFormat(_nexenFormat);

	[Benchmark]
	public ushort ToSmvFormat() => _input.ToSmvFormat();

	[Benchmark]
	public ControllerInput FromSmvFormat() => ControllerInput.FromSmvFormat(_smvFormat);

	[Benchmark]
	public string ToFm2Format() => _input.ToFm2Format();

	[Benchmark]
	public ControllerInput FromFm2Format() => ControllerInput.FromFm2Format(_fm2Format);

	[Benchmark]
	public bool HasInput() => _input.HasInput;

	[Benchmark]
	public ControllerInput Clone() => _input.Clone();
}
