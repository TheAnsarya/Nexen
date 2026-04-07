using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Controls.Platform;
using Avalonia.Markup.Xaml;
using Avalonia.Styling;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Windows;

namespace Nexen;
public class App : Application {
	public static bool ShowConfigWindow { get; set; }
	public static bool ShowAlreadyRunningDialog { get; set; }
	public static bool AlreadyRunningCloseAndRestart { get; set; }

	public override void Initialize() {
		ThemeVariant theme = ThemeVariant.Light;
		if (!Design.IsDesignMode && !ShowConfigWindow) {
			try {
				theme = ConfigManager.Config.Preferences.Theme == NexenTheme.Dark ? ThemeVariant.Dark : ThemeVariant.Light;
			} catch (Exception ex) {
				Log.Error(ex, "[App] Failed to read theme preference, defaulting to Light");
			}
		}
		RequestedThemeVariant = theme;

		Dispatcher.UIThread.UnhandledException += (s, e) => {
			NexenMsgBox.ShowException(e.Exception);
			e.Handled = true;
		};

		AvaloniaXamlLoader.Load(this);
		ResourceHelper.LoadResources();
	}

	public override void OnFrameworkInitializationCompleted() {
		if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop) {
			if (ShowAlreadyRunningDialog) {
				string message = ResourceHelper.GetMessage("AlreadyRunningMessage");
				string title = ResourceHelper.GetMessage("AlreadyRunningTitle");

				MessageBox msgbox = new MessageBox() { Title = title };
				msgbox.GetControl<TextBlock>("Text").Text = message;
				msgbox.GetControl<Image>("imgQuestion").IsVisible = true;

				StackPanel buttonPanel = msgbox.GetControl<StackPanel>("pnlButtons");

				Button restartBtn = new Button {
					Content = ResourceHelper.GetMessage("AlreadyRunningRestart"),
					Width = double.NaN,
					Padding = new Thickness(12, 5),
					Margin = new Thickness(5)
				};
				restartBtn.Click += (_, _) => { AlreadyRunningCloseAndRestart = true; msgbox.Close(); };
				buttonPanel.Children.Add(restartBtn);

				Button leaveBtn = new Button {
					Content = ResourceHelper.GetMessage("AlreadyRunningLeave"),
					Width = double.NaN,
					Padding = new Thickness(12, 5),
					Margin = new Thickness(5),
					IsCancel = true
				};
				leaveBtn.Click += (_, _) => { AlreadyRunningCloseAndRestart = false; msgbox.Close(); };
				buttonPanel.Children.Add(leaveBtn);

				desktop.MainWindow = msgbox;
			} else if (ShowConfigWindow) {
				new PreferencesConfig().InitializeFontDefaults();
				desktop.MainWindow = new SetupWizardWindow();
			} else {
				// Show splash screen while the main window initializes
				var splash = new SplashWindow();
				var splashTimer = Stopwatch.StartNew();
				splash.Show();

				// Use InvokeAsync so the splash window renders before we do heavy init
				Dispatcher.UIThread.InvokeAsync(() => {
					InitializeMainWindow(desktop, splash, splashTimer);
				}, DispatcherPriority.Background);
			}
		}

		base.OnFrameworkInitializationCompleted();
	}

	private static void InitializeMainWindow(IClassicDesktopStyleApplicationLifetime desktop, SplashWindow splash, Stopwatch splashTimer) {
		//Test if the core can be loaded, and display an error message popup if not
		try {
			EmuApi.TestDll();
		} catch (Exception ex) {
			splash.Close();

			bool sdlMissing = ex.Message.Contains("SDL2", StringComparison.InvariantCultureIgnoreCase);

			string errorMessage = sdlMissing
				? ResourceHelper.GetMessage("UnableToStartMissingSdl", ex.Message)
				: ResourceHelper.GetMessage("UnableToStartMissingDependencies", ex.Message + Environment.NewLine + ex.StackTrace);
			MessageBox.Show(null, errorMessage, "Nexen", MessageBoxButtons.OK, MessageBoxIcon.Error, out MessageBox msgbox);
			desktop.MainWindow = msgbox;
			return;
		}

		splash.SetStatus("Initializing...");

		MainWindow mainWindow;
		try {
			mainWindow = new MainWindow();
		} catch {
			// Settings file might be invalid/broken, try to reset them
			Configuration.BackupSettings(ConfigManager.ConfigFile);
			ConfigManager.ResetSettings(false);
			mainWindow = new MainWindow();
		}

		// Close the splash screen once the main window has opened (minimum 2.5 seconds)
		mainWindow.Opened += async (_, _) => {
			splashTimer.Stop();
			int remaining = 2500 - (int)splashTimer.ElapsedMilliseconds;
			if (remaining > 0) {
				await Task.Delay(remaining);
			}
			splash.Close();
		};

		desktop.MainWindow = mainWindow;
		mainWindow.Show();
	}
}
