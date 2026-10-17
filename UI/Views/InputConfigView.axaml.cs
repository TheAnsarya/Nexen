using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Utilities;

namespace Nexen.Views; 
public partial class InputConfigView : UserControl {
	public InputConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
