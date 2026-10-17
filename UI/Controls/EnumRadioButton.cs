using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Data;
using Avalonia.Interactivity;
using Nexen.Localization;

namespace Nexen.Controls;

/// <summary>
/// A radio button that binds to an enum value, automatically checking itself
/// when the bound value matches its <see cref="CheckedWhen"/> property.
/// </summary>
public sealed class EnumRadioButton : RadioButton {
	protected override Type StyleKeyOverride => typeof(RadioButton);

	public static readonly StyledProperty<Enum> ValueProperty = AvaloniaProperty.Register<EnumRadioButton, Enum>(nameof(Value), defaultBindingMode: BindingMode.TwoWay);
	public static readonly StyledProperty<Enum> CheckedWhenProperty = AvaloniaProperty.Register<EnumRadioButton, Enum>(nameof(CheckedWhen));

	public Enum Value {
		get { return GetValue(ValueProperty); }
		set { SetValue(ValueProperty, value); }
	}

	public Enum CheckedWhen {
		get { return GetValue(CheckedWhenProperty); }
		set { SetValue(CheckedWhenProperty, value); }
	}

	static EnumRadioButton() {
		ValueProperty.Changed.AddClassHandler<EnumRadioButton>((x, e) => x.IsChecked = x.Value?.Equals(x.CheckedWhen) == true);

		CheckedWhenProperty.Changed.AddClassHandler<EnumRadioButton>((x, e) => x.IsChecked = x.Value?.Equals(x.CheckedWhen) == true);
	}

	public EnumRadioButton() {
	}

	protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e) {
		base.OnAttachedToVisualTree(e);
		IsChecked = Value.Equals(CheckedWhen);
		Content ??= ResourceHelper.GetEnumText(CheckedWhen);
	}

	protected override void OnIsCheckedChanged(RoutedEventArgs e) {
		base.OnIsCheckedChanged(e);
		if (IsChecked == true) {
			Value = CheckedWhen;
		}
	}
}
