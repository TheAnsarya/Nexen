using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Media;
using Nexen.Config;

namespace Nexen.Utilities;

/// <summary>
/// Provides color manipulation utilities with caching for performance.
/// Supports automatic theme-aware color inversion for dark mode.
/// </summary>
public sealed class ColorHelper {
	// Cache for brushes by color (light theme)
	private static readonly Dictionary<uint, SolidColorBrush> _brushCache = new();
	// Cache for brushes by color (dark theme - inverted)
	private static readonly Dictionary<uint, SolidColorBrush> _darkBrushCache = new();
	// Cache for pens by color (light theme)
	private static readonly Dictionary<uint, Pen> _penCache = new();
	// Cache for pens by color (dark theme - inverted)
	private static readonly Dictionary<uint, Pen> _darkPenCache = new();
	// Track current theme to invalidate cache on theme change
	private static NexenTheme _cachedTheme = NexenTheme.Light;

	/// <summary>
	/// Converts an RGB color to HSL color space.
	/// Uses float precision (sufficient for 8-bit color channels).
	/// </summary>
	public static HslColor RgbToHsl(Color rgbColor) {
		float r = rgbColor.R / 255f;
		float g = rgbColor.G / 255f;
		float b = rgbColor.B / 255f;

		float max = MathF.Max(r, MathF.Max(g, b));
		float min = MathF.Min(r, MathF.Min(g, b));

		float c = max - min; //Chroma

		HslColor hsl;
		hsl.L = (max + min) / 2f;

		if (MathF.Abs(c) < 0.00001f) {
			hsl.S = 0;
			hsl.H = 0;
		} else {
			hsl.S = hsl.L is < 0.0001f or > 0.9999f ? 0 : (max - hsl.L) / MathF.Min(hsl.L, 1f - hsl.L);

			hsl.H = r == max ? 0 + ((g - b) / c) : g == max ? 2 + ((b - r) / c) : 4 + ((r - g) / c);

			hsl.H *= 60;
			if (hsl.H < 0) {
				hsl.H += 360;
			}
		}

		return hsl;
	}

	public static Color HslToRgb(HslColor hsl) {
		float c = (1f - MathF.Abs((2f * hsl.L) - 1f)) * hsl.S;
		float hp = hsl.H / 60f;
		float x = c * (1f - MathF.Abs((hp % 2f) - 1f));

		float r = 0;
		float g = 0;
		float b = 0;

		if (hp <= 1) {
			r = c; g = x; b = 0;
		} else if (hp <= 2) {
			r = x; g = c; b = 0;
		} else if (hp <= 3) {
			r = 0; g = c; b = x;
		} else if (hp <= 4) {
			r = 0; g = x; b = c;
		} else if (hp <= 5) {
			r = x; g = 0; b = c;
		} else if (hp <= 6) {
			r = c; g = 0; b = x;
		}

		float m = hsl.L - (c * 0.5f);
		return Color.FromRgb(
			(byte)MathF.Round(255f * (r + m)),
			(byte)MathF.Round(255f * (g + m)),
			(byte)MathF.Round(255f * (b + m))
		);
	}

	public static Color InvertBrightness(Color color) {
		HslColor hsl = RgbToHsl(color);

		if (hsl.L is >= 0.3f and < 0.6f) {
			hsl.L -= 0.2f;
		}

		hsl.L = 1f - hsl.L;

		if (hsl.L < 0.1f) {
			hsl.L += 0.05f;
		} else if (hsl.L > 0.9f) {
			hsl.L -= 0.05f;
		}

		return HslToRgb(hsl);
	}

	/// <summary>
	/// Invalidates the brush/pen cache if theme has changed.
	/// </summary>
	private static void CheckThemeChanged() {
		var currentTheme = ConfigManager.ActiveTheme;
		if (currentTheme != _cachedTheme) {
			_cachedTheme = currentTheme;
			// Theme changed, clear caches
			_brushCache.Clear();
			_darkBrushCache.Clear();
			_penCache.Clear();
			_darkPenCache.Clear();
		}
	}

	/// <summary>
	/// Gets a cached SolidColorBrush for the specified color, with automatic dark mode inversion.
	/// </summary>
	public static SolidColorBrush GetBrush(Color color) {
		CheckThemeChanged();
		uint key = color.ToUInt32();

		if (ConfigManager.ActiveTheme == NexenTheme.Dark) {
			if (!_darkBrushCache.TryGetValue(key, out var brush)) {
				brush = new SolidColorBrush(InvertBrightness(color));
				_darkBrushCache[key] = brush;
			}
			return brush;
		} else {
			if (!_brushCache.TryGetValue(key, out var brush)) {
				brush = new SolidColorBrush(color);
				_brushCache[key] = brush;
			}
			return brush;
		}
	}

	/// <summary>
	/// Gets a cached SolidColorBrush for the specified brush's color, with automatic dark mode inversion.
	/// </summary>
	public static SolidColorBrush GetBrush(SolidColorBrush b) {
		return GetBrush(b.Color);
	}

	/// <summary>
	/// Gets a cached Pen for the specified color, with automatic dark mode inversion.
	/// </summary>
	public static Pen GetPen(Color color) {
		CheckThemeChanged();
		uint key = color.ToUInt32();

		if (ConfigManager.ActiveTheme == NexenTheme.Dark) {
			if (!_darkPenCache.TryGetValue(key, out var pen)) {
				pen = new Pen(GetBrush(color));
				_darkPenCache[key] = pen;
			}
			return pen;
		} else {
			if (!_penCache.TryGetValue(key, out var pen)) {
				pen = new Pen(GetBrush(color));
				_penCache[key] = pen;
			}
			return pen;
		}
	}

	public static Color GetColor(Color color) {
		if (ConfigManager.ActiveTheme == NexenTheme.Dark) {
			return InvertBrightness(color);
		} else {
			return color;
		}
	}

	public static Color GetColor(UInt32 u32Color) {
		Color color = Color.FromUInt32(u32Color);
		if (ConfigManager.ActiveTheme == NexenTheme.Dark) {
			return InvertBrightness(color);
		} else {
			return color;
		}
	}

	private static double GetColorLuminance(Color color) {
		double r = color.R / 255.0;
		double g = color.G / 255.0;
		double b = color.B / 255.0;

		double convertColor(double c) {
			if (c <= 0.03928) {
				return c / 12.92;
			} else {
				return Math.Pow((c + 0.055) / 1.055, 2.4);
			}
		}

		;

		r = convertColor(r);
		g = convertColor(g);
		b = convertColor(b);

		return (0.2126 * r) + (0.7152 * g) + (0.0722 * b);
	}

	private static double _lumBlack = GetColorLuminance(Colors.Black);
	private static double _lumWhite = GetColorLuminance(Colors.White);

	public static Color GetContrastTextColor(Color color) {
		double lumColor = GetColorLuminance(color);

		double contrastBlack = (lumColor + 0.05) / (_lumBlack + 0.05);
		double contrastWhite = lumColor > _lumWhite ? (lumColor + 0.05) / (_lumWhite + 0.05) : (_lumWhite + 0.05) / (lumColor + 0.05);
		if (contrastBlack > contrastWhite) {
			return Colors.Black;
		}

		return Colors.White;
	}

	/// <summary>
	/// Explicitly clears all brush and pen caches.
	/// Call this when theme changes or when memory needs to be freed.
	/// </summary>
	public static void ClearCache() {
		_brushCache.Clear();
		_darkBrushCache.Clear();
		_penCache.Clear();
		_darkPenCache.Clear();
	}

	public static void InvalidateControlOnThemeChange(Control ctrl, Action? callback = null) {
		ctrl.ActualThemeVariantChanged += (s, e) => {
			ClearCache();
			callback?.Invoke();
			ctrl.InvalidateVisual();
		};
	}
}

public struct HslColor {
	public float H;
	public float S;
	public float L;
}
