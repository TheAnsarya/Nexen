using System;
using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Threading;
using Nexen.Debugger.Utilities;

namespace Nexen.Controls;

/// <summary>
/// A text box specialized for numeric input with support for hex display,
/// min/max clamping, and type-safe value conversion.
/// </summary>
public sealed class NexenNumericTextBox : TextBox {
	protected override Type StyleKeyOverride => typeof(TextBox);

	private static readonly HexConverter _hexConverter = new();

	public static readonly StyledProperty<bool> TrimProperty = AvaloniaProperty.Register<NexenNumericTextBox, bool>(nameof(Trim));
	public static readonly StyledProperty<bool> HexProperty = AvaloniaProperty.Register<NexenNumericTextBox, bool>(nameof(Hex));
	public static readonly StyledProperty<IComparable> ValueProperty = AvaloniaProperty.Register<NexenNumericTextBox, IComparable>(nameof(Value), defaultBindingMode: Avalonia.Data.BindingMode.TwoWay);
	public static readonly StyledProperty<string?> MinProperty = AvaloniaProperty.Register<NexenNumericTextBox, string?>(nameof(Min), null);
	public static readonly StyledProperty<string?> MaxProperty = AvaloniaProperty.Register<NexenNumericTextBox, string?>(nameof(Max), null);

	private bool _preventTextUpdate;

	public bool Hex {
		get { return GetValue(HexProperty); }
		set { SetValue(HexProperty, value); }
	}

	public bool Trim {
		get { return GetValue(TrimProperty); }
		set { SetValue(TrimProperty, value); }
	}

	public IComparable Value {
		get { return GetValue(ValueProperty); }
		set { SetValue(ValueProperty, value); }
	}

	public string? Min {
		get { return GetValue(MinProperty); }
		set { SetValue(MinProperty, value); }
	}

	public string? Max {
		get { return GetValue(MaxProperty); }
		set { SetValue(MaxProperty, value); }
	}

	static NexenNumericTextBox() {
		ValueProperty.Changed.AddClassHandler<NexenNumericTextBox>((x, e) => {
			if (!x.IsInitialized) {
				return;
			}

			//This seems to sometimes cause a stack overflow when the code tries to update
			//value based on the min/max values, which seems to trigger an infinite loop
			//of value updates (unsure if this is an Avalonia bug?) - updating after the event
			//prevents the stack overflow/crash.
			Dispatcher.UIThread.Post(() => {
				x.SetNewValue(x.Value);
				x.UpdateText();
				x.MaxLength = x.GetMaxLength();
			});
		});

		MaxProperty.Changed.AddClassHandler<NexenNumericTextBox>((x, e) => {
			x.MaxLength = x.GetMaxLength();
			x.UpdateText(true);
		});


		TextProperty.Changed.AddClassHandler<NexenNumericTextBox>((x, e) => {
			if (!x.IsInitialized) {
				return;
			}

			//Only update internal value while user is actively editing the text
			//Text will be update to its "standard" representation when focus is lost
			x._preventTextUpdate = true;
			x.UpdateValueFromText();
			x._preventTextUpdate = false;
		});
	}

	public NexenNumericTextBox() {
	}

	protected override void OnInitialized() {
		base.OnInitialized();
		MaxLength = GetMaxLength();
		UpdateText(true);
	}

	long? GetMin() {
		return GetConvertedMinMaxValue(Min);
	}

	long? GetMax() {
		return GetConvertedMinMaxValue(Max);
	}

	private long? GetConvertedMinMaxValue(string? valStr) {
		if (valStr is not null) {
			NumberStyles styles = NumberStyles.Integer;
			if (valStr.StartsWith("0x")) {
				valStr = valStr.Substring(2);
				styles = NumberStyles.HexNumber;
			}

			if (long.TryParse(valStr, styles, null, out long val)) {
				return val;
			}
		}

		return null;
	}

	protected override void OnTextInput(TextInputEventArgs e) {
		if (e.Text is null) {
			e.Handled = true;
			return;
		}

		long? min = GetMin();
		bool allowNegative = min is not null && min.Value < 0;

		if (Hex) {
			foreach (char c in e.Text.ToLowerInvariant()) {
				if (c is not ((>= '0' and <= '9') or (>= 'a' and <= 'f'))) {
					//not hex
					e.Handled = true;
					return;
				}
			}
		} else {
			foreach (char c in e.Text) {
				if (c is < '0' or > '9') {
					//not a number
					if (c == '-' && allowNegative) {
						//Allow negative sign
						continue;
					}

					e.Handled = true;
					return;
				}
			}
		}

		base.OnTextInput(e);
	}

	private void UpdateValueFromText() {
		if (string.IsNullOrWhiteSpace(Text)) {
			long? min = GetMin();
			if (min is not null && min.Value > 0) {
				SetNewValue(ConvertToSameType(min.Value, Value));
			} else {
				SetNewValue(ConvertToSameType(0, Value));
			}
		}

		IComparable? val;
		if (Hex) {
			val = (IComparable?)_hexConverter.ConvertBack(Text, Value.GetType(), null, CultureInfo.InvariantCulture);
		} else {
			if (!long.TryParse(Text, out long parsedValue)) {
				val = Value;
			} else {
				if (parsedValue == 0 && Text.StartsWith('-')) {
					//Allow typing minus before a 0, turn value into -1
					val = -1;
				} else {
					val = parsedValue;
				}
			}
		}

		if (val is not null) {
			SetNewValue(val);
		}
	}

	private int GetMaxLength() {
		IFormattable max;
		long? maxProp = GetMax();
		max = maxProp is not null
			? maxProp.Value
			: Value switch {
				byte => byte.MaxValue,
				sbyte => sbyte.MaxValue,
				short => short.MaxValue,
				ushort => ushort.MaxValue,
				int => int.MaxValue,
				uint => uint.MaxValue,
				long => long.MaxValue,
				ulong => ulong.MaxValue,
				_ => int.MaxValue
			};

		//Increase max length by 1 if minus signs are allowed
		long? min = GetMin();
		bool allowNegative = !Hex && min is not null && min.Value < 0;
		return max.ToString(Hex ? "X" : null, null).Length + (allowNegative ? 1 : 0);
	}

	private void SetNewValue(IComparable val) {
		if (val is null) {
			return;
		}

		long? max = GetMax();
		long? min = GetMin();

		if (max is not null && val.CompareTo(ConvertToSameType(max.Value, val)) > 0) {
			val = ConvertToSameType(max.Value, val);
		} else if (min is not null && val.CompareTo(ConvertToSameType(min.Value, val)) < 0) {
			val = ConvertToSameType(min.Value, val);
		} else if (min is null && val.CompareTo(ConvertToSameType(0, val)) < 0) {
			val = ConvertToSameType(0, val);
		}

		if (!object.Equals(Value, val)) {
			Value = val;
		}
	}

	/// <summary>
	/// Converts a long value to the same numeric type as the reference value.
	/// AOT-safe replacement for Convert.ChangeType.
	/// </summary>
	private static IComparable ConvertToSameType(long value, IComparable referenceValue) {
		return referenceValue switch {
			byte => (byte)value,
			sbyte => (sbyte)value,
			short => (short)value,
			ushort => (ushort)value,
			int => (int)value,
			uint => (uint)value,
			long => value,
			ulong => (ulong)value,
			_ => value
		};
	}

	private void UpdateText(bool force = false) {
		if (Value is null || _preventTextUpdate) {
			return;
		}

		string? text;
		if (Hex) {
			string format = "X" + MaxLength;
			text = (string?)_hexConverter.Convert(Value, typeof(string), format, CultureInfo.InvariantCulture);
		} else {
			text = Value.ToString();
		}

		if (force || text?.TrimStart('0', ' ').ToLowerInvariant() != Text?.TrimStart('0', ' ').ToLowerInvariant()) {
			if (Trim) {
				text = text?.TrimStart('0', ' ');
			}

			if (text?.Length == 0) {
				text = "0";
			}

			Text = text;
		}
	}

	protected override void OnGotFocus(GotFocusEventArgs e) {
		base.OnGotFocus(e);
		this.SelectAll();
	}

	protected override void OnLostFocus(RoutedEventArgs e) {
		base.OnLostFocus(e);
		UpdateValueFromText();
		UpdateText(true);
	}
}
