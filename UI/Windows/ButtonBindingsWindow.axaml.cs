using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Mesen.Localization;
using Mesen.Utilities;
using System;

namespace Mesen.Windows
{
	public class ButtonBindingsWindow : MesenWindow
	{
		[Obsolete("For designer only")]
		public ButtonBindingsWindow() : this("")
		{
		}

		public ButtonBindingsWindow(string buttonName)
		{
			InitializeComponent();
			Title = ResourceHelper.GetMessage("ButtonBindingsTitle", buttonName);
		}

		private void InitializeComponent()
		{
			AvaloniaXamlLoader.Load(this);
		}

		protected override void OnKeyDown(KeyEventArgs e)
		{
			base.OnKeyDown(e);
			if(e.Key == Key.Escape) {
				Close();
			}
		}

		private void btnClose_OnClick(object? sender, RoutedEventArgs e)
		{
			Close();
		}
	}
}
