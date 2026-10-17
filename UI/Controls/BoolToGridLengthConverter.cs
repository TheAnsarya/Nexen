using System;
using System.Globalization;
using Avalonia.Controls;
using Avalonia.Data.Converters;

namespace Nexen.Controls;

/// <summary>
/// Converts a boolean to a GridLength for showing/hiding grid rows.
/// True = Auto (visible), False = 0 (hidden).
/// </summary>
public sealed class BoolToGridLengthConverter : IValueConverter {
	/// <summary>Gets the singleton instance of the converter.</summary>
	public static readonly BoolToGridLengthConverter Instance = new();

	/// <summary>
	/// Gets or sets the GridLength to use when true.
	/// Default is Auto.
	/// </summary>
	public GridLength TrueValue { get; set; } = GridLength.Auto;

	/// <summary>
	/// Gets or sets the GridLength to use when false.
	/// Default is 0 (hidden).
	/// </summary>
	public GridLength FalseValue { get; set; } = new GridLength(0);

	public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture) {
		if (value is bool boolValue) {
			return boolValue ? TrueValue : FalseValue;
		}

		return FalseValue;
	}

	public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) {
		if (value is GridLength gridLength) {
			return gridLength.Value > 0;
		}

		return false;
	}
}
