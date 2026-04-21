using System;
using System.ComponentModel;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.ViewModels;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Windows;

namespace Nexen.Debugger.Windows; 
public partial class DebuggerConfigWindow : NexenWindow {
	private DebuggerConfigWindowViewModel _model;
	private bool _promptToSave = true;

	[Obsolete("For designer only")]
	public DebuggerConfigWindow() : this(new()) {
	}

	public DebuggerConfigWindow(DebuggerConfigWindowViewModel model) {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif

		_model = model;
		DataContext = model;
		ConfigManager.Config.Debug.DebuggerConfigWindow.LoadWindowSettings(this);
	}

	public static void Open(DebugConfigWindowTab tab, Visual? parent) {
		new DebuggerConfigWindow(new DebuggerConfigWindowViewModel(tab)).ShowCenteredDialog(parent as Visual);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnClosed(EventArgs e) {
		base.OnClosed(e);
		_model.Dispose();
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		_promptToSave = false;
		Close();
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		_promptToSave = false;
		_model.RevertChanges();
		Close();
	}

	protected override void OnKeyDown(KeyEventArgs e) {
		base.OnKeyDown(e);
		if (e.Key == Key.Escape) {
			Close();
		}
	}

	private async void DisplaySaveChangesPrompt() {
		DialogResult result = await NexenMsgBox.Show(this, "PromptSaveChanges", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
		switch (result) {
			case DialogResult.Yes: _promptToSave = false; Close(); break;
			case DialogResult.No: _promptToSave = false; _model.RevertChanges(); Close(); break;
			default: break;
		}
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		base.OnClosing(e);
		if (Design.IsDesignMode) {
			return;
		}

		if (_promptToSave && _model.IsDirty()) {
			e.Cancel = true;
			DisplaySaveChangesPrompt();
			return;
		}

		ConfigManager.Config.Debug.DebuggerConfigWindow.SaveWindowSettings(this);
	}
}
