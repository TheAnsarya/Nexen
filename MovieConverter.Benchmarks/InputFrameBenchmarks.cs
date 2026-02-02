using BenchmarkDotNet.Attributes;

namespace Nexen.MovieConverter.Benchmarks;

/// <summary>
/// Benchmarks for InputFrame operations
/// </summary>
[MemoryDiagnoser]
[SimpleJob]
public class InputFrameBenchmarks {
	private InputFrame _frame = null!;
	private string _nexenLogLine = null!;

	[GlobalSetup]
	public void Setup() {
		_frame = new InputFrame(12345);
		_frame.Controllers[0].A = true;
		_frame.Controllers[0].B = true;
		_frame.Controllers[0].Up = true;
		_frame.Controllers[1].Start = true;
		_frame.Comment = "Test frame";

		_nexenLogLine = _frame.ToNexenLogLine(1);
	}

	[Benchmark(Baseline = true)]
	public string ToNexenLogLine() => _frame.ToNexenLogLine(1);

	[Benchmark]
	public InputFrame FromNexenLogLine() => InputFrame.FromNexenLogLine(_nexenLogLine, 12345);

	[Benchmark]
	public bool HasAnyInput() => _frame.HasAnyInput;

	[Benchmark]
	public InputFrame Clone() => _frame.Clone();

	[Benchmark]
	public ControllerInput GetController() => _frame.GetPort(0);
}
