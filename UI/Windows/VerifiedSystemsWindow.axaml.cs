using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Utilities;

namespace Nexen.Windows;

public partial class VerifiedSystemsWindow : NexenWindow {
	public VerifiedSystemInfo? SelectedSystem { get; private set; }

	public VerifiedSystemsWindow() {
		DataContext = new VerifiedSystemsViewModel();
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	public static async Task<VerifiedSystemInfo?> ShowDialogAsync(Window parent) {
		VerifiedSystemsWindow wnd = new VerifiedSystemsWindow {
			WindowStartupLocation = WindowStartupLocation.CenterOwner
		};

		await ((Window)wnd).ShowDialog(parent);
		return wnd.SelectedSystem;
	}

	private void OnSystemClick(object? sender, RoutedEventArgs e) {
		if (sender is not Button btn || btn.Tag is not string systemId || DataContext is not VerifiedSystemsViewModel vm) {
			return;
		}

		SelectedSystem = vm.Systems.FirstOrDefault(s => string.Equals(s.Id, systemId, StringComparison.OrdinalIgnoreCase));
		Close();
	}

	private void OnCancelClick(object? sender, RoutedEventArgs e) {
		SelectedSystem = null;
		Close();
	}
}

public sealed class VerifiedSystemsViewModel {
	public List<VerifiedSystemInfo> Systems { get; } = VerifiedGamesCatalog.GetSystems().ToList();
}
