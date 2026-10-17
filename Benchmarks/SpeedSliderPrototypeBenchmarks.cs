using BenchmarkDotNet.Attributes;
using Nexen.Utilities;

namespace Nexen.Benchmarks;

/// <summary>
/// Micro-benchmarks for speed-slider prototype interactions (#1410).
/// Focused on interaction latency (mapping input to applied semantic speed state).
/// </summary>
[MemoryDiagnoser]
public class SpeedSliderPrototypeBenchmarks {
	private readonly double[] _positions = [0, 1, 2, 3, 4, 5, 6, 2.49, 3.51, 5.49];
	private readonly uint[] _speeds = [0, 25, 50, 100, 200, 300, 500, 4999];
	private int _positionIndex;
	private int _speedIndex;

	[Benchmark(Description = "Slider value -> speed selection")]
	public SpeedSliderSelection MapSliderValueToSelection() {
		double value = _positions[_positionIndex++ % _positions.Length];
		return SpeedSliderPrototypeMapper.FromSliderValue(value);
	}

	[Benchmark(Description = "Current speed -> slider position")]
	public int MapSpeedToSliderPosition() {
		uint speed = _speeds[_speedIndex++ % _speeds.Length];
		return SpeedSliderPrototypeMapper.ToPosition(speed);
	}
}
