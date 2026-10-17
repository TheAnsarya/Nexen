using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Avalonia.VisualTree;
using Nexen.GUI.Utilities;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Windows; 
public partial class CheatDatabaseWindow : NexenWindow {
	private CheatDatabaseViewModel _model;
	private bool _cancelled = true;
	private ListBox _listBox;
	private TextBox _searchBox;

	[Obsolete("For designer only")]
	public CheatDatabaseWindow() : this(ConsoleType.Snes) { }

	public CheatDatabaseWindow(ConsoleType consoleType) {
		_model = new CheatDatabaseViewModel(consoleType);
		DataContext = _model;

		InitializeComponent();

		_searchBox = this.GetControl<TextBox>("Search");
		_listBox = this.GetControl<ListBox>("ListBox");
	}

	protected override void OnClosed(EventArgs e) {
		base.OnClosed(e);
		_model.Dispose();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);

		Dispatcher.UIThread.Post(() => {
			Activate();
			_searchBox.Focus();
		});
	}

	protected override void OnKeyDown(KeyEventArgs e) {
		if (e.Key is Key.Down or Key.Up) {
			if (_searchBox.IsKeyboardFocusWithin) {
				var firstTwo = _model.FilteredEntries.Take(2).ToList();
				if (firstTwo.Count > 1) {
					_model.SelectionModel.SelectedItem = firstTwo[1];
					_listBox.ContainerFromIndex(1)?.Focus();
				} else if (firstTwo.Count > 0) {
					_model.SelectionModel.SelectedItem = firstTwo[0];
					_listBox.ContainerFromIndex(1)?.Focus();
				}
			}
		} else if (e.Key == Key.Enter && _model.SelectionModel.SelectedItem != null) {
			_cancelled = false;
			Close();
		} else if (e.Key == Key.Escape) {
			Close();
		}

		base.OnKeyDown(e);
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		_cancelled = false;
		Close();
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close();
	}

	bool _isDoubleTap = false;
	private void OnPointerReleased(object? sender, PointerReleasedEventArgs e) {
		if (_isDoubleTap) {
			if (DataContext is CheatDatabaseViewModel model && model.SelectionModel != null) {
				_cancelled = false;
				Close();
			}

			_isDoubleTap = false;
		}
	}

	private void OnDoubleTapped(object sender, TappedEventArgs e) {
		_isDoubleTap = true;
	}

	public static async Task<CheatDbGameEntry?> Show(ConsoleType consoleType, Visual? parent) {
		CheatDatabaseWindow wnd = new CheatDatabaseWindow(consoleType);
		await wnd.ShowCenteredDialog(parent);
		if (wnd._cancelled) {
			return null;
		}

		return wnd._model.SelectionModel.SelectedItem;
	}
}
