using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.GUI.Utilities;
using Nexen.Utilities;

namespace Nexen.Windows;

public partial class VerifiedGamesWindow : NexenWindow {
	public VerifiedGameEntry? SelectedGame { get; private set; }
	private Button _loadButton = null!;

	public VerifiedGamesWindow() {
		InitializeComponent();
		_loadButton = this.GetControl<Button>("LoadButton");
		UpdateLoadButtonState();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	public static async Task<VerifiedGameEntry?> ShowDialogAsync(Window parent, VerifiedSystemInfo system) {
		VerifiedGamesViewModel viewModel = new VerifiedGamesViewModel(system);
		VerifiedGamesWindow wnd = new VerifiedGamesWindow {
			DataContext = viewModel,
			WindowStartupLocation = WindowStartupLocation.CenterOwner
		};

		await ((Window)wnd).ShowDialog(parent);
		return wnd.SelectedGame;
	}

	private void OnLoadClick(object? sender, RoutedEventArgs e) {
		if (DataContext is not VerifiedGamesViewModel vm || vm.SelectedGame is null) {
			return;
		}

		if (!vm.SelectedGame.RomExists) {
			DisplayMessageHelper.DisplayMessage("ROM not found", vm.SelectedGame.RomPath);
			return;
		}

		SelectedGame = vm.SelectedGame;
		Close();
	}

	private void OnCancelClick(object? sender, RoutedEventArgs e) {
		SelectedGame = null;
		Close();
	}

	private void OnDoubleTapped(object? sender, RoutedEventArgs e) {
		OnLoadClick(sender, e);
	}

	private void OnSelectionChanged(object? sender, SelectionChangedEventArgs e) {
		UpdateLoadButtonState();
	}

	private void UpdateLoadButtonState() {
		if (_loadButton is null) {
			return;
		}

		if (DataContext is VerifiedGamesViewModel vm) {
			_loadButton.IsEnabled = vm.SelectedGame?.RomExists == true;
		}
	}
}

public sealed class VerifiedGamesViewModel {
	public string Title { get; }
	public List<VerifiedGameEntry> Games { get; }
	public VerifiedGameEntry? SelectedGame { get; set; }

	public VerifiedGamesViewModel(VerifiedSystemInfo system) {
		Title = $"Verified Games - {system.Name}";
		Games = VerifiedGamesCatalog.LoadGames(system);
		SelectedGame = Games.FirstOrDefault(g => g.RomExists) ?? Games.FirstOrDefault();
	}
}
