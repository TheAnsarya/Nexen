using System.Text;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.Utilities;
using Avalonia.Media;

using HslColor = Nexen.Utilities.HslColor;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for color conversion operations used in the hex editor,
/// debugger views, and sprite viewer. These are called thousands of times
/// per frame when the debugger is open.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class ColorConversionBenchmarks
{
	private Color[] _testColors = null!;
	private HslColor[] _hslColors = null!;
	private readonly Random _rng = new(42);

	[GlobalSetup]
	public void Setup()
	{
		_testColors = new Color[1024];
		_hslColors = new HslColor[1024];
		for (int i = 0; i < 1024; i++) {
			_testColors[i] = Color.FromRgb(
				(byte)_rng.Next(256),
				(byte)_rng.Next(256),
				(byte)_rng.Next(256));
			_hslColors[i] = ColorHelper.RgbToHsl(_testColors[i]);
		}
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("RgbHsl")]
	public HslColor[] RgbToHsl_1024Colors()
	{
		var results = new HslColor[1024];
		for (int i = 0; i < 1024; i++) {
			results[i] = ColorHelper.RgbToHsl(_testColors[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("RgbHsl")]
	public Color[] HslToRgb_1024Colors()
	{
		var results = new Color[1024];
		for (int i = 0; i < 1024; i++) {
			results[i] = ColorHelper.HslToRgb(_hslColors[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("RgbHsl")]
	public Color[] RgbToHslToRgb_Roundtrip_1024()
	{
		var results = new Color[1024];
		for (int i = 0; i < 1024; i++) {
			var hsl = ColorHelper.RgbToHsl(_testColors[i]);
			results[i] = ColorHelper.HslToRgb(hsl);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("Invert")]
	public Color[] InvertBrightness_1024Colors()
	{
		var results = new Color[1024];
		for (int i = 0; i < 1024; i++) {
			results[i] = ColorHelper.InvertBrightness(_testColors[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("Contrast")]
	public Color[] GetContrastTextColor_1024Colors()
	{
		var results = new Color[1024];
		for (int i = 0; i < 1024; i++) {
			results[i] = ColorHelper.GetContrastTextColor(_testColors[i]);
		}
		return results;
	}
}
