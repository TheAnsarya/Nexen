using System;
using System.Text;
using Avalonia.Controls;

namespace Nexen.Controls.DataGridExtensions;

/// <summary>
/// Extension methods for DataGrid clipboard operations, replicating DataBox's CopyToClipboard.
/// Exports all visible rows as CSV text.
/// </summary>
public static class DataGridClipboardExtensions {
	/// <summary>
	/// Copies all rows in the DataGrid to clipboard as CSV-formatted text.
	/// Only includes text columns (DataGridTextColumn) with bindings.
	/// </summary>
	public static async void CopyToClipboard(this DataGrid grid) {
		string text = ConvertToText(grid);
		if (string.IsNullOrEmpty(text)) return;

		var topLevel = TopLevel.GetTopLevel(grid);
		if (topLevel?.Clipboard is { } clipboard) {
			await clipboard.SetTextAsync(text);
		}
	}

	/// <summary>
	/// Converts all DataGrid content to CSV-formatted text.
	/// </summary>
	public static string ConvertToText(DataGrid grid) {
		if (grid.ItemsSource is null) return string.Empty;

		StringBuilder sb = new();

		// Header row
		bool first = true;
		foreach (var col in grid.Columns) {
			if (!first) sb.Append(',');
			first = false;

			string header = col.Header?.ToString() ?? "";
			// Escape CSV: if header contains comma, quote, or newline, wrap in quotes
			sb.Append(EscapeCsv(header));
		}
		sb.AppendLine();

		// Data rows — use the DataGrid's ClipboardContentBinding if available,
		// otherwise fall back to column binding path evaluation
		foreach (var item in grid.ItemsSource) {
			first = true;
			foreach (var col in grid.Columns) {
				if (!first) sb.Append(',');
				first = false;

				string value = GetCellValue(col, item);
				sb.Append(EscapeCsv(value));
			}
			sb.AppendLine();
		}

		return sb.ToString();
	}

	/// <summary>
	/// Gets the text value of a cell by evaluating the column's binding against the data item.
	/// </summary>
	private static string GetCellValue(DataGridColumn column, object item) {
		if (column is DataGridBoundColumn boundCol && boundCol.Binding is Avalonia.Data.Binding binding) {
			// Use reflection to get the property value via the binding path
			string path = binding.Path ?? "";
			if (!string.IsNullOrEmpty(path)) {
				try {
					var prop = item.GetType().GetProperty(path);
					return prop?.GetValue(item)?.ToString() ?? "";
				} catch {
					return "";
				}
			}
		}

		// For DataGridTextColumn with ClipboardContentBinding
		if (column.ClipboardContentBinding is Avalonia.Data.Binding clipBinding) {
			string path = clipBinding.Path ?? "";
			if (!string.IsNullOrEmpty(path)) {
				try {
					var prop = item.GetType().GetProperty(path);
					return prop?.GetValue(item)?.ToString() ?? "";
				} catch {
					return "";
				}
			}
		}

		return "";
	}

	/// <summary>
	/// Escapes a value for CSV format: wraps in quotes if it contains comma, quote, or newline.
	/// </summary>
	private static string EscapeCsv(string value) {
		if (value.Contains(',') || value.Contains('"') || value.Contains('\n') || value.Contains('\r')) {
			return $"\"{value.Replace("\"", "\"\"")}\"";
		}
		return value;
	}
}
