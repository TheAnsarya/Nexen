using Xunit;
using Avalonia.Media;
using Nexen.Utilities;

using HslColor = Nexen.Utilities.HslColor;

namespace Nexen.Tests.Utilities;

/// <summary>
/// Unit tests for <see cref="ColorHelper"/> color manipulation and conversion logic.
/// Tests pure color math (RGB↔HSL) without requiring Avalonia UI initialization.
/// </summary>
public class ColorHelperTests
{
	#region RgbToHsl Tests

	[Fact]
	public void RgbToHsl_PureRed_ReturnsCorrectHsl()
	{
		var color = Color.FromRgb(255, 0, 0);
		var hsl = ColorHelper.RgbToHsl(color);

		Assert.Equal(0, hsl.H, 1);
		Assert.Equal(1.0, hsl.S, 2);
		Assert.Equal(0.5, hsl.L, 2);
	}

	[Fact]
	public void RgbToHsl_PureGreen_ReturnsCorrectHsl()
	{
		var color = Color.FromRgb(0, 255, 0);
		var hsl = ColorHelper.RgbToHsl(color);

		Assert.Equal(120, hsl.H, 1);
		Assert.Equal(1.0, hsl.S, 2);
		Assert.Equal(0.5, hsl.L, 2);
	}

	[Fact]
	public void RgbToHsl_PureBlue_ReturnsCorrectHsl()
	{
		var color = Color.FromRgb(0, 0, 255);
		var hsl = ColorHelper.RgbToHsl(color);

		Assert.Equal(240, hsl.H, 1);
		Assert.Equal(1.0, hsl.S, 2);
		Assert.Equal(0.5, hsl.L, 2);
	}

	[Fact]
	public void RgbToHsl_White_ReturnsMaxLightness()
	{
		var color = Color.FromRgb(255, 255, 255);
		var hsl = ColorHelper.RgbToHsl(color);

		Assert.Equal(1.0, hsl.L, 2);
		Assert.Equal(0, hsl.S, 2); // achromatic
	}

	[Fact]
	public void RgbToHsl_Black_ReturnsZeroLightness()
	{
		var color = Color.FromRgb(0, 0, 0);
		var hsl = ColorHelper.RgbToHsl(color);

		Assert.Equal(0, hsl.L, 2);
		Assert.Equal(0, hsl.S, 2); // achromatic
	}

	[Fact]
	public void RgbToHsl_MidGray_ReturnsHalfLightness()
	{
		var color = Color.FromRgb(128, 128, 128);
		var hsl = ColorHelper.RgbToHsl(color);

		Assert.InRange(hsl.L, 0.49, 0.51);
		Assert.Equal(0, hsl.S, 2); // achromatic
	}

	[Theory]
	[InlineData(255, 128, 0)]   // Orange
	[InlineData(128, 0, 255)]   // Purple
	[InlineData(0, 255, 128)]   // Spring green
	[InlineData(64, 64, 64)]    // Dark gray
	[InlineData(192, 192, 192)] // Light gray
	public void RgbToHsl_ThenHslToRgb_Roundtrips(byte r, byte g, byte b)
	{
		var original = Color.FromRgb(r, g, b);
		var hsl = ColorHelper.RgbToHsl(original);
		var roundtripped = ColorHelper.HslToRgb(hsl);

		// Allow ±1 for rounding
		Assert.InRange(roundtripped.R, Math.Max(0, r - 1), Math.Min(255, r + 1));
		Assert.InRange(roundtripped.G, Math.Max(0, g - 1), Math.Min(255, g + 1));
		Assert.InRange(roundtripped.B, Math.Max(0, b - 1), Math.Min(255, b + 1));
	}

	#endregion

	#region HslToRgb Tests

	[Fact]
	public void HslToRgb_RedHsl_ReturnsPureRed()
	{
		var hsl = new HslColor { H = 0, S = 1.0f, L = 0.5f };
		var color = ColorHelper.HslToRgb(hsl);

		Assert.Equal(255, color.R);
		Assert.Equal(0, color.G);
		Assert.Equal(0, color.B);
	}

	[Fact]
	public void HslToRgb_ZeroSaturation_ReturnsGray()
	{
		var hsl = new HslColor { H = 180, S = 0, L = 0.5f };
		var color = ColorHelper.HslToRgb(hsl);

		// With 0 saturation, should be equal R=G=B
		Assert.Equal(color.R, color.G);
		Assert.Equal(color.G, color.B);
	}

	[Theory]
	[InlineData(0)]
	[InlineData(60)]
	[InlineData(120)]
	[InlineData(180)]
	[InlineData(240)]
	[InlineData(300)]
	public void HslToRgb_FullSaturation_HasAtLeastOneMaxComponent(float hue)
	{
		var hsl = new HslColor { H = hue, S = 1.0f, L = 0.5f };
		var color = ColorHelper.HslToRgb(hsl);

		// At full saturation/half lightness, at least one component is 255
		Assert.True(color.R == 255 || color.G == 255 || color.B == 255,
			$"Hue {hue}: Expected at least one 255 component, got ({color.R}, {color.G}, {color.B})");
	}

	#endregion

	#region InvertBrightness Tests

	[Fact]
	public void InvertBrightness_White_ReturnsDark()
	{
		var white = Color.FromRgb(255, 255, 255);
		var inverted = ColorHelper.InvertBrightness(white);

		// White (L=1.0) inverted → L=0.0 + 0.05 adjustment
		var hsl = ColorHelper.RgbToHsl(inverted);
		Assert.True(hsl.L < 0.2, $"Expected low lightness, got {hsl.L}");
	}

	[Fact]
	public void InvertBrightness_Black_ReturnsLight()
	{
		var black = Color.FromRgb(0, 0, 0);
		var inverted = ColorHelper.InvertBrightness(black);

		var hsl = ColorHelper.RgbToHsl(inverted);
		Assert.True(hsl.L > 0.8, $"Expected high lightness, got {hsl.L}");
	}

	[Fact]
	public void InvertBrightness_MidRange_ShiftsLightness()
	{
		var mid = Color.FromRgb(128, 64, 32);
		var inverted = ColorHelper.InvertBrightness(mid);

		var originalHsl = ColorHelper.RgbToHsl(mid);
		var invertedHsl = ColorHelper.RgbToHsl(inverted);

		// Inverted lightness should be significantly different
		Assert.NotEqual(originalHsl.L, invertedHsl.L, 1);
	}

	[Theory]
	[InlineData(255, 0, 0)]
	[InlineData(0, 255, 0)]
	[InlineData(0, 0, 255)]
	[InlineData(255, 255, 0)]
	[InlineData(0, 255, 255)]
	[InlineData(255, 0, 255)]
	public void InvertBrightness_PrimaryColors_PreservesHue(byte r, byte g, byte b)
	{
		var original = Color.FromRgb(r, g, b);
		var inverted = ColorHelper.InvertBrightness(original);

		var originalHsl = ColorHelper.RgbToHsl(original);
		var invertedHsl = ColorHelper.RgbToHsl(inverted);

		// Hue should remain the same (or very close)
		Assert.Equal(originalHsl.H, invertedHsl.H, 1);
	}

	#endregion

	#region GetContrastTextColor Tests

	[Fact]
	public void GetContrastTextColor_White_ReturnsBlack()
	{
		var result = ColorHelper.GetContrastTextColor(Colors.White);
		Assert.Equal(Colors.Black, result);
	}

	[Fact]
	public void GetContrastTextColor_Black_ReturnsWhite()
	{
		var result = ColorHelper.GetContrastTextColor(Colors.Black);
		Assert.Equal(Colors.White, result);
	}

	[Fact]
	public void GetContrastTextColor_BrightYellow_ReturnsBlack()
	{
		// Yellow is bright → black text for contrast
		var result = ColorHelper.GetContrastTextColor(Color.FromRgb(255, 255, 0));
		Assert.Equal(Colors.Black, result);
	}

	[Fact]
	public void GetContrastTextColor_DarkBlue_ReturnsWhite()
	{
		// Dark blue → white text for contrast
		var result = ColorHelper.GetContrastTextColor(Color.FromRgb(0, 0, 128));
		Assert.Equal(Colors.White, result);
	}

	[Theory]
	[InlineData(255, 255, 255)]
	[InlineData(0, 0, 0)]
	[InlineData(255, 0, 0)]
	[InlineData(0, 128, 0)]
	[InlineData(128, 128, 128)]
	public void GetContrastTextColor_AlwaysReturnsBlackOrWhite(byte r, byte g, byte b)
	{
		var result = ColorHelper.GetContrastTextColor(Color.FromRgb(r, g, b));

		Assert.True(
			result == Colors.Black || result == Colors.White,
			$"Expected Black or White, got ({result.R}, {result.G}, {result.B})"
		);
	}

	#endregion
}
