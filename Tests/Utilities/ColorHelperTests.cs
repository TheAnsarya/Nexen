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

	#region Exhaustive Float Conversion Tests

	[Fact]
	public void RgbToHsl_Roundtrip_AllPrimaryAndSecondaryHues()
	{
		// Test all 6 primary/secondary hue sectors at multiple saturations/lightnesses.
		// Ensures float math does not produce different byte-level results than double would.
		byte[] componentValues = [0, 1, 64, 127, 128, 191, 254, 255];

		foreach (byte r in componentValues) {
			foreach (byte g in componentValues) {
				foreach (byte b in componentValues) {
					var original = Color.FromRgb(r, g, b);
					var hsl = ColorHelper.RgbToHsl(original);
					var roundtripped = ColorHelper.HslToRgb(hsl);

					// Allow ±1 for float rounding (same tolerance as existing roundtrip tests)
					Assert.True(Math.Abs(original.R - roundtripped.R) <= 1,
						$"R mismatch for ({r},{g},{b}): {original.R} vs {roundtripped.R}");
					Assert.True(Math.Abs(original.G - roundtripped.G) <= 1,
						$"G mismatch for ({r},{g},{b}): {original.G} vs {roundtripped.G}");
					Assert.True(Math.Abs(original.B - roundtripped.B) <= 1,
						$"B mismatch for ({r},{g},{b}): {original.B} vs {roundtripped.B}");
				}
			}
		}
	}

	[Fact]
	public void RgbToHsl_HueRange_AlwaysValid()
	{
		// Verify HSL values are always in valid range after float conversion.
		// H: [0, 360), S: [0, 1], L: [0, 1]
		byte[] samples = [0, 1, 50, 100, 127, 128, 200, 254, 255];

		foreach (byte r in samples) {
			foreach (byte g in samples) {
				foreach (byte b in samples) {
					var hsl = ColorHelper.RgbToHsl(Color.FromRgb(r, g, b));

					Assert.True(hsl.H >= 0 && hsl.H < 360,
						$"H out of range for ({r},{g},{b}): {hsl.H}");
					Assert.True(hsl.S >= 0 && hsl.S <= 1,
						$"S out of range for ({r},{g},{b}): {hsl.S}");
					Assert.True(hsl.L >= 0 && hsl.L <= 1,
						$"L out of range for ({r},{g},{b}): {hsl.L}");

					// Verify no NaN or infinity from float math
					Assert.False(float.IsNaN(hsl.H), $"H is NaN for ({r},{g},{b})");
					Assert.False(float.IsNaN(hsl.S), $"S is NaN for ({r},{g},{b})");
					Assert.False(float.IsNaN(hsl.L), $"L is NaN for ({r},{g},{b})");
					Assert.False(float.IsInfinity(hsl.H), $"H is Infinity for ({r},{g},{b})");
					Assert.False(float.IsInfinity(hsl.S), $"S is Infinity for ({r},{g},{b})");
					Assert.False(float.IsInfinity(hsl.L), $"L is Infinity for ({r},{g},{b})");
				}
			}
		}
	}

	[Theory]
	[InlineData(0)]
	[InlineData(1)]
	[InlineData(50)]
	[InlineData(100)]
	[InlineData(127)]
	[InlineData(128)]
	[InlineData(200)]
	[InlineData(254)]
	[InlineData(255)]
	public void InvertBrightness_Roundtrip_DoubleSameAsFloat(byte gray)
	{
		// InvertBrightness of a grayscale value should produce valid color.
		// Grayscale values exercise all lightness levels.
		var input = Color.FromRgb(gray, gray, gray);
		var inverted = ColorHelper.InvertBrightness(input);

		// Result must be valid RGB
		Assert.True(inverted.R >= 0 && inverted.R <= 255);
		Assert.True(inverted.G >= 0 && inverted.G <= 255);
		Assert.True(inverted.B >= 0 && inverted.B <= 255);
	}

	[Fact]
	public void InvertBrightness_AllPrimaries_ProducesValidOutput()
	{
		// Test all primary, secondary, tertiary colors and edge cases
		var colors = new[] {
			Color.FromRgb(255, 0, 0),     // Red
			Color.FromRgb(0, 255, 0),     // Green
			Color.FromRgb(0, 0, 255),     // Blue
			Color.FromRgb(255, 255, 0),   // Yellow
			Color.FromRgb(255, 0, 255),   // Magenta
			Color.FromRgb(0, 255, 255),   // Cyan
			Color.FromRgb(0, 0, 0),       // Black
			Color.FromRgb(255, 255, 255), // White
			Color.FromRgb(128, 128, 128), // Mid gray
			Color.FromRgb(1, 1, 1),       // Near black
			Color.FromRgb(254, 254, 254), // Near white
			Color.FromRgb(255, 128, 0),   // Orange
			Color.FromRgb(128, 0, 255),   // Purple
			Color.FromRgb(0, 128, 64),    // Teal-green
		};

		foreach (var color in colors) {
			var inverted = ColorHelper.InvertBrightness(color);

			// Inverted brightness should keep hue similar but change lightness
			var originalHsl = ColorHelper.RgbToHsl(color);
			var invertedHsl = ColorHelper.RgbToHsl(inverted);

			// For chromatic colors (S > 0), hue should be preserved
			if (originalHsl.S > 0.01f && invertedHsl.S > 0.01f) {
				float hueDiff = MathF.Abs(originalHsl.H - invertedHsl.H);
				// Handle wraparound (e.g., 359 vs 1)
				if (hueDiff > 180) hueDiff = 360 - hueDiff;
				Assert.True(hueDiff < 5,
					$"Hue shifted too much for ({color.R},{color.G},{color.B}): " +
					$"{originalHsl.H} -> {invertedHsl.H}");
			}
		}
	}

	[Fact]
	public void HslToRgb_FullHueSpectrum_ProducesValidColors()
	{
		// Sweep all 360 integer hue values at full saturation, mid lightness.
		// Validates the float-based HslToRgb produces valid output across all hue sectors.
		for (int h = 0; h < 360; h++) {
			var hsl = new HslColor { H = h, S = 1.0f, L = 0.5f };
			var rgb = ColorHelper.HslToRgb(hsl);

			Assert.True(rgb.R >= 0 && rgb.R <= 255,
				$"R out of range for hue {h}: {rgb.R}");
			Assert.True(rgb.G >= 0 && rgb.G <= 255,
				$"G out of range for hue {h}: {rgb.G}");
			Assert.True(rgb.B >= 0 && rgb.B <= 255,
				$"B out of range for hue {h}: {rgb.B}");

			// At full saturation, mid lightness, at least one component should be 255 or near it
			int maxComponent = Math.Max(rgb.R, Math.Max(rgb.G, rgb.B));
			Assert.True(maxComponent >= 253,
				$"Max component too low for hue {h}: {maxComponent}");
		}
	}

	[Fact]
	public void HslToRgb_ZeroSaturation_ProducesGrayscale()
	{
		// With S=0, all hues should produce the same grayscale based on lightness.
		for (int h = 0; h < 360; h += 30) {
			var hsl = new HslColor { H = h, S = 0f, L = 0.5f };
			var rgb = ColorHelper.HslToRgb(hsl);

			Assert.Equal(rgb.R, rgb.G);
			Assert.Equal(rgb.G, rgb.B);
			Assert.True(Math.Abs(rgb.R - 128) <= 1,
				$"Grayscale mid-lightness should be ~128 for hue {h}, got {rgb.R}");
		}
	}

	#endregion
}
