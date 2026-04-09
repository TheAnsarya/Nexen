using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Media;
using ReactiveUI.Avalonia;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen;
class Program {
	// Initialization code. Don't use any Avalonia, third-party APIs or any
	// SynchronizationContext-reliant code before AppMain is called: things aren't initialized
	// yet and stuff might break.
	private static bool _loggedNexenCoreLoadPath;

	public static string OriginalFolder { get; private set; }
	public static string[] CommandLineArgs { get; private set; } = [];

	public static string ExePath {
		get {
			// On Linux, Process.MainModule triggers procfs parsing which requires ICU/globalization
			// to be initialized. Use Environment.ProcessPath first (available since .NET 6) as it
			// reads /proc/self/exe directly without globalization dependencies.
			if (Environment.ProcessPath is { Length: > 0 } processPath) {
				return processPath;
			}

			try {
				if (Process.GetCurrentProcess().MainModule?.FileName is { Length: > 0 } mainModule) {
					return mainModule;
				}
			} catch {
				// Swallow — MainModule can throw on Linux if globalization is misconfigured
			}

			return Path.Join(Path.GetDirectoryName(AppContext.BaseDirectory), RuntimeInformation.IsOSPlatform(OSPlatform.Windows) ? "Nexen.exe" : "Nexen");
		}
	}

	static Program() {
		try {
			Program.OriginalFolder = Environment.CurrentDirectory;
		} catch {
			Program.OriginalFolder = Path.GetDirectoryName(ExePath) ?? "";
		}
	}

	[STAThread]
	public static int Main(string[] args) {
		// Initialize console-only logging first (no HomeFolder access, no files created)
		Log.Initialize();
		Log.Info($"Nexen starting with args: {string.Join(" ", args)}");

		// Set up global exception handlers
		AppDomain.CurrentDomain.UnhandledException += (sender, e) => {
			// On Linux, Tmds.DBus.Protocol's background handler can race against the
			// already-disposed Avalonia dispatcher during exit, throwing TaskCanceledException.
			// This is benign — log it as a warning and exit cleanly instead of aborting.
			if (e.ExceptionObject is TaskCanceledException tce &&
				tce.StackTrace?.Contains("Tmds.DBus") == true) {
				Log.Warn("Suppressing TaskCanceledException during shutdown (Avalonia/DBus cleanup race)");
				Log.CloseAndFlush();
				Environment.Exit(0);
				return;
			}

			Log.Fatal(e.ExceptionObject as Exception ?? new Exception($"Non-Exception thrown: {e.ExceptionObject}"), "Unhandled exception in AppDomain");
			Log.CloseAndFlush();
		};

		TaskScheduler.UnobservedTaskException += (sender, e) => {
			Log.Error(e.Exception, "Unobserved task exception");
			e.SetObserved(); // Prevent crash
		};

		try {
			if (!System.Diagnostics.Debugger.IsAttached) {
				NativeLibrary.SetDllImportResolver(Assembly.GetExecutingAssembly(), DllImportResolver);
				NativeLibrary.SetDllImportResolver(typeof(SkiaSharp.SKGraphics).Assembly, DllImportResolver);
				NativeLibrary.SetDllImportResolver(typeof(HarfBuzzSharp.Blob).Assembly, DllImportResolver);
			}

			if (args.Length >= 4 && args[0] == "--update") {
				UpdateHelper.AttemptUpdate(args[1], args[2], args[3], args.Contains("admin"));
				return 0;
			}

			// Check for first-run WITHOUT triggering HomeFolder creation.
			// HomeFolder auto-detection creates ~/.config/Nexen/ and we don't want that
			// before the setup wizard lets the user choose their preferred location.
			string portableConfig = Path.Combine(ConfigManager.DefaultPortableFolder, "settings.json");
			string documentsConfig = Path.Combine(ConfigManager.DefaultDocumentsFolder, "settings.json");
			bool isFirstRun = !File.Exists(portableConfig) && !File.Exists(documentsConfig);

			if (isFirstRun) {
				Log.Info("No config file found, showing setup wizard");
				// First-run: DO NOT access ConfigManager.HomeFolder or extract dependencies.
				// Native libs are available from the app's own directory (AppContext.BaseDirectory)
				// via the DllImportResolver, so the wizard UI can render without extracting deps.
				App.ShowConfigWindow = true;
				BuildAvaloniaApp().StartWithClassicDesktopLifetime(args, ShutdownMode.OnMainWindowClose);

				// Check if the wizard created a config file by checking both locations
				// directly — do NOT call GetConfigFile() which accesses HomeFolder and
				// would create ~/.config/Nexen/ if the wizard was cancelled.
				if (File.Exists(portableConfig) || File.Exists(documentsConfig)) {
					//Configuration done, restart process
					Log.Info("Configuration complete, restarting");
					try {
						Process.Start(new ProcessStartInfo(Program.ExePath) { UseShellExecute = true });
					} catch (Exception ex) {
						Log.Error(ex, "Failed to restart Nexen after setup wizard");
					}
				}

				return 0;
			}

			// Not first-run: HomeFolder is determined from existing settings.json
			Log.InitializeFileLogging();
			Log.Info($"Home folder: {ConfigManager.HomeFolder}");
			Log.Info($"OS: {RuntimeInformation.OSDescription} ({RuntimeInformation.OSArchitecture})");
			Log.Info($".NET: {RuntimeInformation.FrameworkDescription}");
			Log.Info($"App base: {AppContext.BaseDirectory}");
			//Start loading config file in a separate thread
			Task.Run(() => ConfigManager.LoadConfig());

			Log.Info("Extracting native dependencies...");
			//Extract core dll & other native dependencies
			DependencyHelper.ExtractNativeDependencies(ConfigManager.HomeFolder);
			SyncNativeDependenciesFromAppBase();

			if (CommandLineHelper.IsTestRunner(args)) {
				Log.Info("Running in test mode");
				return TestRunner.Run(args);
			}

			using SingleInstance instance = SingleInstance.Instance;
			instance.Init(args);
			if (instance.FirstInstance) {
				Log.Info("Starting main application...");
				Program.CommandLineArgs = (string[])args.Clone();
				BuildAvaloniaApp().StartWithClassicDesktopLifetime(args, ShutdownMode.OnMainWindowClose);
			} else {
				Log.Info("Another instance is already running, showing dialog");
				App.ShowAlreadyRunningDialog = true;
				BuildAvaloniaApp().StartWithClassicDesktopLifetime(args, ShutdownMode.OnMainWindowClose);

				if (App.AlreadyRunningCloseAndRestart) {
					Log.Info("User chose to close and restart Nexen");
					KillExistingInstances();
					try {
						Process.Start(new ProcessStartInfo(Program.ExePath) { UseShellExecute = true });
					} catch (Exception ex) {
						Log.Error(ex, "Failed to restart Nexen after closing existing instance");
					}
				} else {
					Log.Info("User chose to leave existing instance running");
				}
			}

			Log.Info("Nexen shutting down normally");

			// On Linux, Tmds.DBus.Protocol's background handler can race against
			// the already-disposed Avalonia dispatcher, causing TaskCanceledException.
			// Exit cleanly here before that cleanup runs.
			if (OperatingSystem.IsLinux()) {
				Log.CloseAndFlush();
				Environment.Exit(0);
			}

			return 0;
		} catch (TypeInitializationException ex) {
			// TypeInitializationException wraps the real cause — the inner exception has
			// the actual stack trace and error.  Log both so users can report useful info.
			Log.Fatal(ex.InnerException ?? ex, $"Fatal TypeInitializationException in Main (type: {ex.TypeName})");
			Log.Fatal(ex, "Full TypeInitializationException details");
			throw;
		} catch (Exception ex) {
			Log.Fatal(ex, "Fatal exception in Main");
			throw;
		} finally {
			Log.CloseAndFlush();
		}
	}

	private static void KillExistingInstances() {
		string currentProcessName = Process.GetCurrentProcess().ProcessName;
		int currentPid = Environment.ProcessId;

		foreach (Process process in Process.GetProcessesByName(currentProcessName)) {
			if (process.Id != currentPid) {
				try {
					Log.Info($"Killing existing Nexen process (PID {process.Id})");
					process.Kill();
					process.WaitForExit(5000);
				} catch (Exception ex) {
					Log.Error(ex, $"Failed to kill Nexen process (PID {process.Id})");
				} finally {
					process.Dispose();
				}
			} else {
				process.Dispose();
			}
		}
	}

	private static IntPtr DllImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? searchPath) {
		if (libraryName.Contains("Nexen") || libraryName.Contains("SkiaSharp") || libraryName.Contains("HarfBuzz")) {
			string originalName = libraryName;
			if (libraryName.EndsWith(".dll")) {
				libraryName = libraryName.Substring(0, libraryName.Length - 4);
			}

			if (OperatingSystem.IsLinux()) {
				if (!libraryName.EndsWith(".so")) {
					libraryName += ".so";
				}
			} else if (OperatingSystem.IsWindows()) {
				if (!libraryName.EndsWith(".dll")) {
					libraryName += ".dll";
				}
			} else if (OperatingSystem.IsMacOS()) {
				if (!libraryName.EndsWith(".dylib")) {
					libraryName += ".dylib";
				}
			}

			// Prefer local build directory (dev workflow) over HomeFolder
			string localPath = Path.Combine(AppContext.BaseDirectory, libraryName);
			if (File.Exists(localPath)) {
				LogNativeLibraryLoad(libraryName, localPath);
				return NativeLibrary.Load(localPath);
			}

			// Only fall back to HomeFolder if it's already been initialized
			// (avoids creating ~/.config/Nexen during the setup wizard).
			if (!App.ShowConfigWindow) {
				try {
					string homePath = Path.Combine(ConfigManager.HomeFolder, libraryName);
					if (File.Exists(homePath)) {
						LogNativeLibraryLoad(libraryName, homePath);
						return NativeLibrary.Load(homePath);
					}
				} catch {
					// HomeFolder may not be usable yet during first-run
				}
			}

			// Library not found — log diagnostic info so users can report the issue
			string homeInfo = App.ShowConfigWindow ? "HomeFolder=<skipped during setup>" : $"HomeFolder='{ConfigManager.HomeFolder}'";
			Log.Error($"[DllImportResolver] Native library not found: '{originalName}' (resolved to '{libraryName}'). " +
				$"Searched: AppBase='{AppContext.BaseDirectory}' (exists={File.Exists(localPath)}), {homeInfo}");
		}

		return IntPtr.Zero;
	}

	private static void SyncNativeDependenciesFromAppBase() {
		// Skip when HomeFolder IS the app base directory (portable mode) — copying a file
		// onto itself is a no-op at best, and on Linux can corrupt mmap'd .so files.
		string appBase = Path.GetFullPath(AppContext.BaseDirectory).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		string home = Path.GetFullPath(ConfigManager.HomeFolder).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		if (string.Equals(appBase, home, OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal)) {
			return;
		}

		string[] nativeFiles = OperatingSystem.IsWindows()
			? ["NexenCore.dll", "libSkiaSharp.dll", "libHarfBuzzSharp.dll"]
			: OperatingSystem.IsLinux()
				? ["NexenCore.so", "libSkiaSharp.so", "libHarfBuzzSharp.so"]
				: ["NexenCore.dylib", "libSkiaSharp.dylib", "libHarfBuzzSharp.dylib"];

		Directory.CreateDirectory(ConfigManager.HomeFolder);

		foreach (string fileName in nativeFiles) {
			string sourcePath = Path.Combine(AppContext.BaseDirectory, fileName);
			if (!File.Exists(sourcePath)) {
				continue;
			}

			string destinationPath = Path.Combine(ConfigManager.HomeFolder, fileName);
			bool shouldCopy = !File.Exists(destinationPath);

			if (!shouldCopy) {
				FileInfo sourceInfo = new(sourcePath);
				FileInfo destinationInfo = new(destinationPath);
				shouldCopy = sourceInfo.Length != destinationInfo.Length || sourceInfo.LastWriteTimeUtc > destinationInfo.LastWriteTimeUtc;
			}

			if (!shouldCopy) {
				continue;
			}

			try {
				File.Copy(sourcePath, destinationPath, true);
				Log.Info($"Synchronized native dependency: {fileName} ({new FileInfo(sourcePath).Length} bytes)");
			} catch (Exception ex) {
				Log.Error(ex, $"Failed to synchronize native dependency: {fileName}");
			}
		}
	}

	private static void LogNativeLibraryLoad(string libraryName, string path) {
		if (_loggedNexenCoreLoadPath || !libraryName.Equals("NexenCore.dll", StringComparison.OrdinalIgnoreCase)) {
			return;
		}

		_loggedNexenCoreLoadPath = true;

		if (File.Exists(path)) {
			FileInfo info = new(path);
			Log.Info($"Loading NexenCore from '{path}' ({info.Length} bytes, mtime {info.LastWriteTimeUtc:O})");
		} else {
			Log.Warn($"Attempting to load NexenCore from missing path '{path}'");
		}
	}

	// Avalonia configuration, don't remove; also used by visual designer.
	public static AppBuilder BuildAvaloniaApp() {
		// Ensure SVG support assembly is preserved by the trimmer/AOT
		GC.KeepAlive(typeof(Svg.Skia.SKSvg).Assembly);

		return AppBuilder.Configure<App>()
				.UseReactiveUI(_ => { })
				.UsePlatformDetect()
				.With(new Win32PlatformOptions { })
				.With(new X11PlatformOptions {
					EnableInputFocusProxy = Environment.GetEnvironmentVariable("XDG_CURRENT_DESKTOP") == "gamescope",
				})
				.With(new AvaloniaNativePlatformOptions { RenderingMode = new AvaloniaNativeRenderingMode[] { AvaloniaNativeRenderingMode.OpenGl, AvaloniaNativeRenderingMode.Software } })
				.LogToTrace();
	}
}
