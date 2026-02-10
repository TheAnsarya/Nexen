using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger; 
/// <summary>
/// Manages watch expressions for monitoring values during debugging.
/// </summary>
/// <remarks>
/// <para>
/// The WatchManager maintains a list of watch expressions per CPU type. Watch expressions
/// can be:
/// <list type="bullet">
///   <item><description>Labels (e.g., "PlayerHP")</description></item>
///   <item><description>Memory addresses (e.g., "$7E0000")</description></item>
///   <item><description>Complex expressions (e.g., "[PlayerHP] + 10")</description></item>
///   <item><description>Array displays (e.g., "[$300,10]" to show 10 bytes from $300)</description></item>
/// </list>
/// </para>
/// <para>
/// Watch values support format specifiers appended after a comma:
/// <list type="bullet">
///   <item><description>H - Hexadecimal (e.g., "PlayerHP, H")</description></item>
///   <item><description>B - Binary (e.g., "PlayerHP, B")</description></item>
///   <item><description>S - Signed decimal (e.g., "PlayerHP, S4" for 4-byte signed)</description></item>
///   <item><description>U - Unsigned decimal (e.g., "PlayerHP, U")</description></item>
/// </list>
/// </para>
/// </remarks>
public sealed class WatchManager {
	/// <summary>
	/// Delegate for handling watch list changes.
	/// </summary>
	/// <param name="resetSelection">
	/// If <c>true</c>, the UI should reset its selection (e.g., after removing items).
	/// </param>
	public delegate void WatchChangedEventHandler(bool resetSelection);

	/// <summary>
	/// Regex for parsing format specifiers (e.g., ", H" or ", S4").
	/// </summary>
	/// <remarks>
	/// Groups: 1=expression, 2=format letter (B/H/S/U), 3=optional byte length (1-4).
	/// </remarks>
	public static Regex FormatSuffixRegex = new Regex(@"^(.*),\s*([B|H|S|U])([\d]){0,1}$", RegexOptions.Compiled);

	/// <summary>
	/// Regex for parsing array display syntax (e.g., "[$300, 10]" or "[Label, 5]").
	/// </summary>
	/// <remarks>
	/// Groups: 1=full address, 2=hex address, 3=decimal address, 4=label name, 5=element count.
	/// </remarks>
	private static Regex _arrayWatchRegex = new Regex(@"\[((\$[0-9A-Fa-f]+)|(\d+)|([@_a-zA-Z0-9]+))\s*,\s*(\d+)\]", RegexOptions.Compiled);

	/// <summary>
	/// Raised when the watch list changes (add, remove, update, or format change).
	/// </summary>
	public event WatchChangedEventHandler? WatchChanged;

	/// <summary>
	/// The list of watch expression strings.
	/// </summary>
	private List<string> _watchEntries = [];

	/// <summary>
	/// The CPU type this watch manager is associated with.
	/// </summary>
	private CpuType _cpuType;

	/// <summary>
	/// Per-CPU watch managers, created on demand.
	/// </summary>
	private static Dictionary<CpuType, WatchManager> _watchManagers = new Dictionary<CpuType, WatchManager>();

	/// <summary>
	/// Gets or creates the watch manager for a specific CPU type.
	/// </summary>
	/// <param name="cpuType">The CPU type to get the watch manager for.</param>
	/// <returns>The watch manager instance for the specified CPU.</returns>
	public static WatchManager GetWatchManager(CpuType cpuType) {
		WatchManager? manager;
		if (!_watchManagers.TryGetValue(cpuType, out manager)) {
			manager = new WatchManager(cpuType);
			_watchManagers[cpuType] = manager;
		}

		return manager;
	}

	/// <summary>
	/// Clears watch entries for all CPU types.
	/// </summary>
	/// <remarks>
	/// Called when loading a new ROM or resetting the debug workspace.
	/// </remarks>
	public static void ClearEntries() {
		foreach (WatchManager wm in _watchManagers.Values) {
			wm.WatchEntries = new();
		}
	}

	/// <summary>
	/// Initializes a new instance of the <see cref="WatchManager"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type this manager handles watches for.</param>
	public WatchManager(CpuType cpuType) {
		_cpuType = cpuType;
	}

	/// <summary>
	/// Gets or sets the list of watch expression strings.
	/// </summary>
	/// <remarks>
	/// Setting this property raises <see cref="WatchChanged"/> with <c>resetSelection=true</c>.
	/// </remarks>
	public List<string> WatchEntries {
		get { return _watchEntries; }
		set {
			_watchEntries = new List<string>(value);
			WatchChanged?.Invoke(true);
		}
	}

	/// <summary>
	/// Evaluates all watch expressions and returns their current values.
	/// </summary>
	/// <param name="previousValues">The previous watch values, used to detect changes.</param>
	/// <returns>
	/// A list of <see cref="WatchValueInfo"/> containing evaluated results,
	/// plus an empty entry at the end for adding new watches.
	/// </returns>
	/// <remarks>
	/// <para>
	/// Each expression is evaluated against the current emulator state using
	/// <see cref="DebugApi.EvaluateExpression"/>. Format specifiers are processed
	/// to control output formatting.
	/// </para>
	/// <para>
	/// Array display syntax (e.g., "[$300,10]") is handled specially to show
	/// multiple consecutive memory values.
	/// </para>
	/// </remarks>
	public List<WatchValueInfo> GetWatchContent(IList<WatchValueInfo> previousValues) {
		WatchFormatStyle defaultStyle = ConfigManager.Config.Debug.Debugger.WatchFormat;
		int defaultByteLength = 1;
		if (defaultStyle == WatchFormatStyle.Signed) {
			defaultByteLength = 4;
		}

		var list = new List<WatchValueInfo>();
		for (int i = 0; i < _watchEntries.Count; i++) {
			string expression = _watchEntries[i].Trim();
			string newValue = "";
			EvalResultType resultType;

			string exprToEvaluate = expression;
			WatchFormatStyle style = defaultStyle;
			int byteLength = defaultByteLength;
			if (expression.StartsWith("{") && expression.EndsWith("}")) {
				//Default to 2-byte values when using {} syntax
				byteLength = 2;
			}

			ProcessFormatSpecifier(ref exprToEvaluate, ref style, ref byteLength);

			Int64 numericValue = -1;

			bool forceHasChanged = false;
			Match match = _arrayWatchRegex.Match(expression);
			if (match.Success) {
				//Watch expression matches the array display syntax (e.g: [$300,10] = display 10 bytes starting from $300)
				newValue = ProcessArrayDisplaySyntax(style, ref forceHasChanged, match);
			} else {
				Int64 result = DebugApi.EvaluateExpression(exprToEvaluate, _cpuType, out resultType, true);
				switch (resultType) {
					case EvalResultType.Numeric:
						numericValue = result;
						newValue = FormatValue(result, style, byteLength);
						break;

					case EvalResultType.Boolean: newValue = result == 0 ? "false" : "true"; break;
					case EvalResultType.Invalid: newValue = "<invalid expression>"; forceHasChanged = true; break;
					case EvalResultType.DivideBy0: newValue = "<division by zero>"; forceHasChanged = true; break;
					case EvalResultType.OutOfScope: newValue = "<label out of scope>"; forceHasChanged = true; break;
				}
			}

			bool isChanged = forceHasChanged || (i < previousValues.Count && (previousValues[i].Value != newValue));
			list.Add(new WatchValueInfo() { Expression = expression, Value = newValue, IsChanged = isChanged, NumericValue = numericValue });
		}

		list.Add(new WatchValueInfo());

		return list;
	}

	/// <summary>
	/// Formats a numeric value according to the specified style.
	/// </summary>
	/// <param name="value">The value to format.</param>
	/// <param name="style">The display format style.</param>
	/// <param name="byteLength">The number of bytes to display (1-4).</param>
	/// <returns>The formatted value string.</returns>
	/// <remarks>
	/// Format examples:
	/// <list type="bullet">
	///   <item><description>Hex: "$00FF"</description></item>
	///   <item><description>Binary: "%1111.1111"</description></item>
	///   <item><description>Signed: "-128"</description></item>
	///   <item><description>Unsigned: "255"</description></item>
	/// </list>
	/// </remarks>
	private string FormatValue(Int64 value, WatchFormatStyle style, int byteLength) {
		switch (style) {
			case WatchFormatStyle.Unsigned: return ((UInt32)value).ToString();
			case WatchFormatStyle.Hex: return "$" + value.ToString("X" + (byteLength * 2));
			case WatchFormatStyle.Binary:
				string binary = Convert.ToString(value, 2).PadLeft(byteLength * 8, '0');
				for (int i = binary.Length - 4; i > 0; i -= 4) {
					binary = binary.Insert(i, ".");
				}

				return "%" + binary;
			case WatchFormatStyle.Signed:
				int bitCount = byteLength * 8;
				if (bitCount < 64) {
					if (((value >> (bitCount - 1)) & 0x01) == 0x01) {
						//Negative value
						return (value | (-(1L << bitCount))).ToString();
					} else {
						//Position value
						return value.ToString();
					}
				} else {
					return value.ToString();
				}

			default: throw new Exception("Unsupported format");
		}
	}

	/// <summary>
	/// Parses and removes format specifier from an expression.
	/// </summary>
	/// <param name="expression">
	/// The expression to process. Modified to remove the format specifier if found.
	/// </param>
	/// <param name="style">Updated with the parsed format style.</param>
	/// <param name="byteLength">Updated with the parsed byte length.</param>
	/// <returns>
	/// <c>true</c> if a format specifier was found and processed; otherwise, <c>false</c>.
	/// </returns>
	private bool ProcessFormatSpecifier(ref string expression, ref WatchFormatStyle style, ref int byteLength) {
		Match match = WatchManager.FormatSuffixRegex.Match(expression);
		if (!match.Success) {
			return false;
		}

		string format = match.Groups[2].Value.ToUpperInvariant();
		switch (format[0]) {
			case 'S': style = WatchFormatStyle.Signed; break;
			case 'H': style = WatchFormatStyle.Hex; break;
			case 'B': style = WatchFormatStyle.Binary; break;
			case 'U': style = WatchFormatStyle.Unsigned; break;
			default: throw new Exception("Invalid format");
		}

		byteLength = match.Groups[3].Success ? Math.Max(Math.Min(Int32.Parse(match.Groups[3].Value), 4), 1) : 1;

		expression = match.Groups[1].Value;
		return true;
	}

	/// <summary>
	/// Processes array display syntax to show multiple memory values.
	/// </summary>
	/// <param name="style">The display format style.</param>
	/// <param name="forceHasChanged">Set to <c>true</c> if an error occurs.</param>
	/// <param name="match">The regex match containing the array syntax components.</param>
	/// <returns>
	/// A space-separated list of formatted values, or an error message.
	/// </returns>
	/// <remarks>
	/// Supports address as hex ($300), decimal (768), or label name.
	/// </remarks>
	private string ProcessArrayDisplaySyntax(WatchFormatStyle style, ref bool forceHasChanged, Match match) {
		string newValue;
		Int64 address;
		if (match.Groups[2].Value.Length > 0) {
			address = Int64.Parse(match.Groups[2].Value.Substring(1), System.Globalization.NumberStyles.HexNumber);
		} else if (match.Groups[3].Value.Length > 0) {
			address = Int64.Parse(match.Groups[3].Value);
		} else if (match.Groups[4].Value.Length > 0) {
			string token = match.Groups[4].Value.Trim();
			CodeLabel? label = LabelManager.GetLabel(token);
			if (label is null) {
				Int64 value = DebugApi.EvaluateExpression(token, _cpuType, out EvalResultType result, true);
				if (result == EvalResultType.Numeric) {
					address = value;
				} else {
					forceHasChanged = true;
					return "<invalid label>";
				}
			} else {
				address = label.GetRelativeAddress(_cpuType).Address;
			}
		} else {
			return "<invalid expression>";
		}

		int elemCount = int.Parse(match.Groups[5].Value);

		if (address >= 0) {
			List<string> values = new List<string>(elemCount);
			MemoryType memType = _cpuType.ToMemoryType();
			for (Int64 j = address, end = address + elemCount; j < end; j++) {
				if (j > UInt32.MaxValue) {
					break;
				}

				int memValue = DebugApi.GetMemoryValue(memType, (UInt32)j);
				values.Add(FormatValue(memValue, style, 1));
			}

			newValue = string.Join(" ", values);
		} else {
			newValue = "<label out of scope>";
			forceHasChanged = true;
		}

		return newValue;
	}

	/// <summary>
	/// Adds one or more watch expressions to the list.
	/// </summary>
	/// <param name="expressions">The expressions to add.</param>
	public void AddWatch(params string[] expressions) {
		foreach (string expression in expressions) {
			_watchEntries.Add(expression);
		}

		WatchChanged?.Invoke(false);
		DebugWorkspaceManager.AutoSave();
	}

	/// <summary>
	/// Updates or adds a watch expression at the specified index.
	/// </summary>
	/// <param name="index">The index to update, or a value >= count to append.</param>
	/// <param name="expression">
	/// The new expression. If empty/whitespace, the entry at the index is removed.
	/// </param>
	public void UpdateWatch(int index, string expression) {
		if (string.IsNullOrWhiteSpace(expression)) {
			if (index < _watchEntries.Count) {
				RemoveWatch(index);
			}
		} else {
			if (index >= _watchEntries.Count) {
				_watchEntries.Add(expression);
			} else {
				if (_watchEntries[index] == expression) {
					return;
				}

				_watchEntries[index] = expression;
			}

			WatchChanged?.Invoke(false);
		}

		DebugWorkspaceManager.AutoSave();
	}

	/// <summary>
	/// Removes watch entries at the specified indexes.
	/// </summary>
	/// <param name="indexes">The indexes of entries to remove.</param>
	public void RemoveWatch(params int[] indexes) {
		HashSet<int> set = new HashSet<int>(indexes);
		_watchEntries = _watchEntries.Where((el, index) => !set.Contains(index)).ToList();
		WatchChanged?.Invoke(true);
		DebugWorkspaceManager.AutoSave();
	}

	/// <summary>
	/// Imports watch expressions from a text file (one expression per line).
	/// </summary>
	/// <param name="filename">The path to the file to import.</param>
	public void Import(string filename) {
		if (File.Exists(filename)) {
			WatchEntries = new List<string>(File.ReadAllLines(filename));
		}
	}

	/// <summary>
	/// Exports watch expressions to a text file (one expression per line).
	/// </summary>
	/// <param name="filename">The path to the file to export to.</param>
	public void Export(string filename) {
		File.WriteAllLines(filename, WatchEntries);
	}

	/// <summary>
	/// Builds a format specifier string from format style and byte length.
	/// </summary>
	/// <param name="format">The format style.</param>
	/// <param name="byteLength">The byte length (1-4).</param>
	/// <returns>A format specifier string like ", H" or ", S4".</returns>
	private string GetFormatString(WatchFormatStyle format, int byteLength) {
		string formatString = ", ";
		switch (format) {
			case WatchFormatStyle.Binary: formatString += "B"; break;
			case WatchFormatStyle.Hex: formatString += "H"; break;
			case WatchFormatStyle.Signed: formatString += "S"; break;
			case WatchFormatStyle.Unsigned: formatString += "U"; break;
			default: throw new Exception("Unsupported type");
		}

		if (byteLength > 1) {
			formatString += byteLength.ToString();
		}

		return formatString;
	}

	/// <summary>
	/// Clears format specifiers from the selected watch entries.
	/// </summary>
	/// <param name="indexes">The indexes of entries to clear formatting from.</param>
	internal void ClearSelectionFormat(int[] indexes) {
		SetSelectionFormat("", indexes);
	}

	/// <summary>
	/// Applies a format specifier to the selected watch entries.
	/// </summary>
	/// <param name="format">The format style to apply.</param>
	/// <param name="byteLength">The byte length to use.</param>
	/// <param name="indexes">The indexes of entries to format.</param>
	public void SetSelectionFormat(WatchFormatStyle format, int byteLength, int[] indexes) {
		string formatString = GetFormatString(format, byteLength);
		SetSelectionFormat(formatString, indexes);
	}

	/// <summary>
	/// Sets or clears format specifiers on the selected watch entries.
	/// </summary>
	/// <param name="formatString">The format string to append, or empty to clear.</param>
	/// <param name="indexes">The indexes of entries to update.</param>
	private void SetSelectionFormat(string formatString, int[] indexes) {
		foreach (int i in indexes) {
			if (i < _watchEntries.Count) {
				Match match = WatchManager.FormatSuffixRegex.Match(_watchEntries[i]);
				if (match.Success) {
					UpdateWatch(i, match.Groups[1].Value + formatString);
				} else {
					UpdateWatch(i, _watchEntries[i] + formatString);
				}
			}
		}
	}
}

/// <summary>
/// Holds the evaluated result of a watch expression.
/// </summary>
/// <remarks>
/// Used for data binding in the watch list UI. Uses ReactiveUI for property notifications.
/// </remarks>
public sealed class WatchValueInfo : ReactiveObject {
	/// <summary>
	/// Gets or sets the formatted string representation of the value.
	/// </summary>
	[Reactive] public string Value { get; set; } = "";

	/// <summary>
	/// Gets or sets the original watch expression.
	/// </summary>
	[Reactive] public string Expression { get; set; } = "";

	/// <summary>
	/// Gets or sets whether the value changed since the last evaluation.
	/// </summary>
	/// <remarks>
	/// Used to highlight changed values in the UI.
	/// </remarks>
	[Reactive] public bool IsChanged { get; set; } = false;

	/// <summary>
	/// Gets or sets the raw numeric value of the expression, or -1 if invalid/non-numeric.
	/// </summary>
	public Int64 NumericValue { get; set; } = -1;
}

/// <summary>
/// Specifies how watch values should be displayed.
/// </summary>
public enum WatchFormatStyle {
	/// <summary>
	/// Display as unsigned decimal (e.g., "255").
	/// </summary>
	Unsigned,

	/// <summary>
	/// Display as signed decimal (e.g., "-128").
	/// </summary>
	Signed,

	/// <summary>
	/// Display as hexadecimal with $ prefix (e.g., "$FF").
	/// </summary>
	Hex,

	/// <summary>
	/// Display as binary with % prefix (e.g., "%1111.1111").
	/// </summary>
	Binary
}
