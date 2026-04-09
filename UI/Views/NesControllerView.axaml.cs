using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Views; 
public partial class NesControllerView : UserControl {
	public bool ShowMicrophoneButton { get; }

	public NesControllerView() : this(false) {
	}

	public NesControllerView(bool showMicrophoneButton) {
		ShowMicrophoneButton = showMicrophoneButton;

		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
