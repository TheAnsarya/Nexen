using System;
using System.ComponentModel;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Data;
using Avalonia.Data.Converters;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Controls.DataGridExtensions;
using Nexen.Interop;
using Nexen.ViewModels;

namespace Nexen.Windows;
public class CheatListWindow : NexenWindow {
	private CheatListWindowViewModel _model;
	private NotificationListener _listener;

	public CheatListWindow() {
		InitializeComponent();

		_listener = new NotificationListener();
		_listener.OnNotification += OnNotification;

		_model = new CheatListWindowViewModel();
		_model.InitActions(this);
		DataContext = _model;

		// Subscribe to DataGrid CellClick/CellDoubleClick routed events
		this.AddHandler(DataGridCellClickBehavior.CellClickEvent, OnCellClick);
		this.AddHandler(DataGridCellClickBehavior.CellDoubleClickEvent, OnCellDoubleClick);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		ConfigManager.Config.Cheats.LoadWindowSettings(this);
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		_listener.Dispose();
		ConfigManager.Config.Cheats.SaveWindowSettings(this);
		CheatCodes.ApplyCheats();
		base.OnClosing(e);
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		_model.SaveCheats();
		Close();
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close();
	}

	private void OnCellClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (e.RowItem is CheatCode && e.Column?.SortMemberPath == "Enabled") {
			var selectedItems = _model.Selection.SelectedItems;
			bool anyEnabled = false;
			foreach (CheatCode? cheat in selectedItems) {
				if (cheat?.Enabled == true) {
					anyEnabled = true;
					break;
				}
			}
			bool newValue = !anyEnabled;
			foreach (CheatCode? cheat in selectedItems) {
				if (cheat != null) {
					cheat.Enabled = newValue;
				}
			}

			_model.Sort();
			_model.ApplyCheats();
		}
	}

	private async void OnCellDoubleClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (e.RowItem is CheatCode cheat) {
			await CheatEditWindow.EditCheat(cheat, this);
			_model.Sort();
			_model.ApplyCheats();
		}
	}

	public void OnNotification(NotificationEventArgs e) {
		switch (e.NotificationType) {
			case ConsoleNotificationType.BeforeGameLoad:
			case ConsoleNotificationType.BeforeGameUnload:
				_model.SaveCheats();
				break;

			case ConsoleNotificationType.GameLoaded:
				Dispatcher.UIThread.Post(() => _model.LoadCheats());
				break;

			case ConsoleNotificationType.BeforeEmulationStop:
				_model.SaveCheats();
				Dispatcher.UIThread.Post(() => Close());
				break;
		}
	}
}

public class CodeStringConverter : IValueConverter {
	public object Convert(object? value, Type targetType, object? parameter, System.Globalization.CultureInfo culture) {
		if (targetType == typeof(string) && value is string str) {
			return string.Join(", ", str.Split(Environment.NewLine, StringSplitOptions.RemoveEmptyEntries));
		}

		throw new NotSupportedException();
	}

	public object ConvertBack(object? value, Type targetType, object? parameter, System.Globalization.CultureInfo culture) {
		throw new NotSupportedException();
	}
}
