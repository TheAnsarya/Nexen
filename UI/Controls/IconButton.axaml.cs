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
public partial class IconButton : Button {
	protected override Type StyleKeyOverride => typeof(Button);

	public static readonly StyledProperty<string> IconProperty = AvaloniaProperty.Register<KeyBindingButton, string>(nameof(Icon), "");

	public string Icon {
		get { return GetValue(IconProperty); }
		set { SetValue(IconProperty, value); }
	}

	static IconButton() {
		IconProperty.Changed.AddClassHandler<IconButton>((x, e) => x.GetControl<Image>("IconImage").Source = ImageUtilities.ImageFromAsset(x.Icon));
	}

	public IconButton() {
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
