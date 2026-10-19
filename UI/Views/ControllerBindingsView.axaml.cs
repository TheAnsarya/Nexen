using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Mesen.Views
{
	public class ControllerBindingsView : UserControl
	{
		public ControllerBindingsView()
		{
			InitializeComponent();
		}

		private void InitializeComponent()
		{
			AvaloniaXamlLoader.Load(this);
		}
	}
}
