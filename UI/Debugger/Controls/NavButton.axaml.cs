using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Nexen.Debugger.ViewModels;
using Nexen.Localization;
using Nexen.Utilities;

namespace Nexen.Debugger.Controls;

public class NavButton : UserControl {
	public static readonly StyledProperty<NavType> NavProperty = AvaloniaProperty.Register<NavButton, NavType>(nameof(NavProperty));
	public static readonly StyledProperty<string> TooltipTextProperty = AvaloniaProperty.Register<NavButton, string>(nameof(TooltipText), "");
	public static readonly StyledProperty<IImage?> IconProperty = AvaloniaProperty.Register<NavButton, IImage?>(nameof(Icon));

	public IImage? Icon {
		get { return GetValue(IconProperty); }
		set { SetValue(IconProperty, value); }
	}

	public NavType Nav {
		get { return GetValue(NavProperty); }
		set { SetValue(NavProperty, value); }
	}

	public string TooltipText {
		get { return GetValue(TooltipTextProperty); }
		set { SetValue(TooltipTextProperty, value); }
	}

	static NavButton() {
		NavProperty.Changed.AddClassHandler<NavButton>((x, e) => {
			x.Icon = ImageUtilities.ImageFromAsset("Assets/" + x.Nav.ToString() + ".png");
			x.TooltipText = ResourceHelper.GetEnumText(x.Nav);
		});
	}

	public NavButton() {
		InitializeComponent();
		Icon = ImageUtilities.ImageFromAsset("Assets/" + Nav.ToString() + ".png");
		TooltipText = ResourceHelper.GetEnumText(Nav);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void OnClick(object sender, RoutedEventArgs e) {
		if (DataContext is MemoryToolsDisplayOptionsViewModel model) {
			model.MemoryTools.NavigateTo(Nav);
		}
	}
}

public enum NavType {
	NextCode,
	NextData,
	NextExec,
	NextRead,
	NextUnknown,
	NextWrite,

	PrevCode,
	PrevData,
	PrevExec,
	PrevRead,
	PrevUnknown,
	PrevWrite
}
