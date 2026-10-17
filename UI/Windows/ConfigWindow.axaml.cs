using System;
using System.ComponentModel;
using System.IO;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Styling;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Windows;
public partial class ConfigWindow : NexenWindow {
	// Keep the default/min settings window in left-tab mode.
	// On Linux, top-tab mode with many tabs can consume most vertical space.
	private const double CompactTabBreakpointWidth = 700;
	private readonly TabControl _settingsTabControl;
	private ConfigViewModel _model;
	private bool _promptToSave = true;

	[Obsolete("For designer only")]
	public ConfigWindow() : this(ConfigWindowTab.Audio) { }

	public ConfigWindow(ConfigWindowTab tab) {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif

		_model = new ConfigViewModel(tab);
		DataContext = _model;
		ConfigManager.Config.ConfigWindow.LoadWindowSettings(this);
		_settingsTabControl = this.GetControl<TabControl>("SettingsTabControl");
		this.GetPropertyChangedObservable(ClientSizeProperty).Subscribe(_ => UpdateResponsiveTabPlacement());
		UpdateResponsiveTabPlacement();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		_promptToSave = false;
		_model.SaveConfig();
		Close();
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		_promptToSave = false;
		_model.RevertConfig();
		Close();
	}

	protected override void OnKeyDown(KeyEventArgs e) {
		base.OnKeyDown(e);
		if (e.Key == Key.Escape) {
			Close();
		}
	}

	private void OpenNexenFolder(object sender, RoutedEventArgs e) {
		System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo() {
			FileName = ConfigManager.HomeFolder + Path.DirectorySeparatorChar,
			UseShellExecute = true,
			Verb = "open"
		});
	}

	private async void ResetAllSettings(object sender, RoutedEventArgs e) {
		if (await NexenMsgBox.Show(this, "ResetSettingsConfirmation", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK) {
			ConfigManager.ResetSettings();
			_promptToSave = false;
			Close();
		}
	}

	private async void DisplaySaveChangesPrompt() {
		DialogResult result = await NexenMsgBox.Show(this, "PromptSaveChanges", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
		switch (result) {
			case DialogResult.Yes: _promptToSave = false; _model.SaveConfig(); Close(); break;
			case DialogResult.No: _promptToSave = false; _model.RevertConfig(); Close(); break;
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

		ConfigManager.Config.ConfigWindow.SaveWindowSettings(this);

		ConfigManager.Config.ApplyConfig();
		_model.Dispose();

		PreferencesConfig.UpdateTheme();

		//Ensure config isn't modified by the UI while closing
		DataContext = null;
	}

	private void UpdateResponsiveTabPlacement() {
		_settingsTabControl.TabStripPlacement = ClientSize.Width < CompactTabBreakpointWidth ? Avalonia.Controls.Dock.Top : Avalonia.Controls.Dock.Left;
	}
}
