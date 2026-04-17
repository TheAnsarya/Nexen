using BenchmarkDotNet.Attributes;
using Nexen.Controls;

namespace Nexen.Benchmarks;

/// <summary>
/// Micro-benchmarks for piano roll prefetch scheduling decisions.
/// </summary>
[MemoryDiagnoser]
public class PianoRollPrefetchBenchmarks {
	private int _lastStart;
	private int _lastEnd;
	private double _lastZoom;

	[GlobalSetup]
	public void Setup() {
		_lastStart = 100;
		_lastEnd = 300;
		_lastZoom = 1.0;
	}

	[Benchmark(Baseline = true)]
	public bool AlwaysQueue() {
		return true;
	}

	[Benchmark]
	public bool Debounced_UnchangedViewport() {
		return PianoRollControl.ShouldQueueFrameTextPrefetch(_lastStart, _lastEnd, _lastZoom, 100, 300, 1.0);
	}

	[Benchmark]
	public bool Debounced_ChangedViewport() {
		return PianoRollControl.ShouldQueueFrameTextPrefetch(_lastStart, _lastEnd, _lastZoom, 120, 320, 1.0);
	}

	[Benchmark]
	public bool Debounced_ChangedZoom() {
		return PianoRollControl.ShouldQueueFrameTextPrefetch(_lastStart, _lastEnd, _lastZoom, 100, 300, 1.25);
	}
}
