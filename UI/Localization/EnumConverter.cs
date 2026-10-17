using System;
using Avalonia.Data.Converters;

namespace Nexen.Localization; 
public sealed class EnumConverter : IValueConverter {
	public object Convert(object? value, Type targetType, object? parameter, System.Globalization.CultureInfo culture) {
		if (value is Enum enumValue && targetType == typeof(string)) {
			return ResourceHelper.GetEnumText(enumValue);
		}

		return value?.ToString() ?? "null value";
	}


	public object ConvertBack(object? value, Type targetType, object? parameter, System.Globalization.CultureInfo culture) {
		throw new NotImplementedException();
	}
}
