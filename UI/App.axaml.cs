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
public partial class App : Application {
	public static bool ShowConfigWindow { get; set; }
	public static bool ShowAlreadyRunningDialog { get; set; }
	public static bool AlreadyRunningCloseAndRestart { get; set; }

	public override void Initialize() {
		ThemeVariant theme = ThemeVariant.Light;
		string languageCode = StartupLanguageResolver.ResolveLanguageCode(Design.IsDesignMode, ShowConfigWindow, () => ConfigManager.Config.Preferences.UiLanguage);
		if (!Design.IsDesignMode && !ShowConfigWindow) {
			try {
				theme = ConfigManager.Config.Preferences.Theme == NexenTheme.Dark ? ThemeVariant.Dark : ThemeVariant.Light;
			} catch (Exception ex) {
				Log.Error(ex, "[App] Failed to read theme preference, defaulting to Light");
			}
		}
		RequestedThemeVariant = theme;

		Dispatcher.UIThread.UnhandledException += (s, e) => {
			Log.Error(e.Exception, "Unhandled UI thread exception");
			try {
				NexenMsgBox.ShowException(e.Exception);
			} catch (Exception dialogEx) {
				Log.Fatal(dialogEx, "Failed to display unhandled exception dialog");
			}
			e.Handled = true;
		};

		AvaloniaXamlLoader.Load(this);
		ResourceHelper.LoadResources(languageCode);
		Log.Info($"[App] Loaded UI language '{languageCode}'. Available languages: {string.Join(", ", ResourceHelper.GetAvailableLanguageCodes())}");
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
		try {
			InitializeMainWindowCore(desktop, splash, splashTimer);
		} catch (Exception ex) {
			Log.Error(ex, "[Startup] Unexpected failure during main window initialization");
			try { splash.Close(); } catch { /* splash may already be closed */ }
			ShowFatalStartupError(desktop, ex);
		}
	}

	private static void InitializeMainWindowCore(IClassicDesktopStyleApplicationLifetime desktop, SplashWindow splash, Stopwatch splashTimer) {
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
		} catch (Exception ex) {
			Log.Error(ex, "[Startup] MainWindow creation failed, resetting settings and retrying");
			// Settings file might be invalid/broken, try to reset them
			Configuration.BackupSettings(ConfigManager.ConfigFile);
			ConfigManager.ResetSettings(false);
			try {
				mainWindow = new MainWindow();
			} catch (Exception ex2) {
				Log.Fatal(ex2, "[Startup] MainWindow creation failed after settings reset — cannot start");
				splash.Close();
				ShowFatalStartupError(desktop, ex2);
				return;
			}
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

	/// <summary>
	/// Displays a fatal startup error dialog and ensures the application exits cleanly.
	/// Falls back to a native OS message box if Avalonia rendering is broken.
	/// </summary>
	private static void ShowFatalStartupError(IClassicDesktopStyleApplicationLifetime desktop, Exception ex) {
		string logPath = Log.LogFilePath;
		string message = $"Nexen failed to initialize.\n\n" +
			$"Error: {ex.Message}\n\n" +
			$"This may be caused by:\n" +
			$"  \u2022 Missing Visual C++ Redistributable\n" +
			$"  \u2022 Incompatible or missing GPU drivers\n" +
			$"  \u2022 Corrupted installation files\n" +
			$"  \u2022 Antivirus blocking NexenCore.dll\n\n" +
			$"Log file: {logPath}\n\n" +
			$"Technical details:\n{ex.GetType().Name}: {ex.Message}";

		try {
			MessageBox.Show(null, message, "Nexen - Startup Error", MessageBoxButtons.OK, MessageBoxIcon.Error, out MessageBox msgbox);
			desktop.MainWindow = msgbox;
		} catch (Exception dialogEx) {
			// Avalonia rendering is broken — try native fallback
			Log.Fatal(dialogEx, "[Startup] Failed to display Avalonia error dialog, trying native fallback");

			if (OperatingSystem.IsWindows()) {
				try {
					ShowWin32MessageBox(message);
				} catch (Exception win32Ex) {
					// Truly nothing left to try — error is in the log file
					Debug.WriteLine($"App: Win32 MessageBox fallback failed: {win32Ex.Message}");
				}
			}

			Log.CloseAndFlush();
			Environment.Exit(1);
		}
	}

	[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
	private static extern int MessageBoxW(IntPtr hWnd, string text, string caption, uint type);

	[System.Runtime.Versioning.SupportedOSPlatform("windows")]
	private static void ShowWin32MessageBox(string message) {
		// Win32 MessageBox as absolute fallback when Avalonia rendering is completely broken
		MessageBoxW(IntPtr.Zero, message, "Nexen - Startup Error", 0x00000010); // MB_ICONERROR
	}
}
