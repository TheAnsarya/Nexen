using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.ViewModels;

namespace Nexen.Windows;

/// <summary>
/// TAS Editor window for frame-by-frame movie editing.
/// </summary>
public class TasEditorWindow : NexenWindow {
	private ListBox _frameList = null!;

	public TasEditorWindow() {
		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif
		DataContext = new TasEditorViewModel();

		if (DataContext is TasEditorViewModel vm) {
			vm.InitActions(this);
		}
	}

	public TasEditorWindow(TasEditorViewModel viewModel) {
		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif
		DataContext = viewModel;
		viewModel.InitActions(this);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
		_frameList = this.FindControl<ListBox>("FrameList")!;
	}

	/// <summary>
	/// Gets the view model for this window.
	/// </summary>
	public TasEditorViewModel? ViewModel => DataContext as TasEditorViewModel;

	/// <summary>
	/// Handles button clicks for controller input.
	/// Toggles the button state for the currently selected frame.
	/// </summary>
	private void OnButtonClick(object? sender, RoutedEventArgs e) {
		if (sender is Button button && button.Tag is string buttonName && ViewModel != null) {
			ViewModel.ToggleButton(0, buttonName);
		}
	}

	/// <summary>
	/// Called when the window is initialized.
	/// </summary>
	protected override void OnInitialized() {
		base.OnInitialized();

		// Set up keyboard shortcuts
		KeyDown += (s, e) => {
			// Frame navigation with arrow keys when ListBox doesn't have focus
			if (ViewModel != null && !_frameList.IsFocused) {
				switch (e.Key) {
					case Avalonia.Input.Key.PageUp:
						ViewModel.SelectedFrameIndex = Math.Max(0, ViewModel.SelectedFrameIndex - 10);
						e.Handled = true;
						break;
					case Avalonia.Input.Key.PageDown:
						ViewModel.SelectedFrameIndex = Math.Min(ViewModel.Frames.Count - 1, ViewModel.SelectedFrameIndex + 10);
						e.Handled = true;
						break;
					case Avalonia.Input.Key.Home:
						ViewModel.SelectedFrameIndex = 0;
						e.Handled = true;
						break;
					case Avalonia.Input.Key.End:
						ViewModel.SelectedFrameIndex = ViewModel.Frames.Count - 1;
						e.Handled = true;
						break;
				}
			}
		};
	}

	/// <summary>
	/// Called when the window is closing. Prompts to save unsaved changes.
	/// </summary>
	protected override void OnClosing(WindowClosingEventArgs e) {
		if (ViewModel?.HasUnsavedChanges == true) {
			// TODO: Show confirmation dialog
			// For now, allow close without prompting
		}
		base.OnClosing(e);
	}
}
