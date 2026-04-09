using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Data;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Localization;

namespace Nexen.Controls; 
public partial class OverscanInput : UserControl {
	public static readonly StyledProperty<OverscanConfig> OverscanProperty = AvaloniaProperty.Register<OverscanInput, OverscanConfig>(nameof(Overscan), new OverscanConfig(), defaultBindingMode: BindingMode.TwoWay);

	public OverscanConfig Overscan {
		get { return GetValue(OverscanProperty); }
		set { SetValue(OverscanProperty, value); }
	}

	public OverscanInput() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
