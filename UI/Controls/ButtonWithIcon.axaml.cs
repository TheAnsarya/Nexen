using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using Avalonia.Styling;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.Windows;

namespace Nexen.Controls;
public partial class ButtonWithIcon : Button {
	protected override Type StyleKeyOverride => typeof(Button);

	public static readonly StyledProperty<string> TextProperty = AvaloniaProperty.Register<ButtonWithIcon, string>(nameof(Text), "");
	public static readonly StyledProperty<string> IconProperty = AvaloniaProperty.Register<ButtonWithIcon, string>(nameof(Icon), "");
	public static readonly StyledProperty<bool> ShowIconProperty = AvaloniaProperty.Register<ButtonWithIcon, bool>(nameof(ShowIcon), true);

	public string Text {
		get { return GetValue(TextProperty); }
		set { SetValue(TextProperty, value); }
	}

	public string Icon {
		get { return GetValue(IconProperty); }
		set { SetValue(IconProperty, value); }
	}

	public bool ShowIcon {
		get { return GetValue(ShowIconProperty); }
		set { SetValue(ShowIconProperty, value); }
	}

	static ButtonWithIcon() {
		IconProperty.Changed.AddClassHandler<ButtonWithIcon>((x, e) => x.GetControl<Image>("IconImage").Source = ImageUtilities.ImageFromAsset(x.Icon));
	}

	public ButtonWithIcon() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnClick() {
		base.OnClick();
		ToolTip.SetIsOpen(this, false);
	}
}
