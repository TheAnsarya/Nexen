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
using Avalonia.ReactiveUI;
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
		// Initialize logging first thing
		Log.Initialize();
		Log.Info($"Nexen starting with args: {string.Join(" ", args)}");

		// Set up global exception handlers
		AppDomain.CurrentDomain.UnhandledException += (sender, e) => {
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

			Log.Info($"Home folder: {ConfigManager.HomeFolder}");
			Environment.CurrentDirectory = ConfigManager.HomeFolder;

			if (!File.Exists(ConfigManager.GetConfigFile())) {
				Log.Info("No config file found, showing setup wizard");
				//Could not find configuration file, show wizard
				DependencyHelper.ExtractNativeDependencies(ConfigManager.HomeFolder);
				SyncNativeDependenciesFromAppBase();
				App.ShowConfigWindow = true;
				BuildAvaloniaApp().StartWithClassicDesktopLifetime(args, ShutdownMode.OnMainWindowClose);
				if (File.Exists(ConfigManager.GetConfigFile())) {
					//Configuration done, restart process
					Log.Info("Configuration complete, restarting");
					Process.Start(Program.ExePath);
				}

				return 0;
			}

			Log.Info("Loading configuration...");
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
				Log.Info("Another instance is already running");
			}

			Log.Info("Nexen shutting down normally");
			return 0;
		} catch (Exception ex) {
			Log.Fatal(ex, "Fatal exception in Main");
			throw;
		} finally {
			Log.CloseAndFlush();
		}
	}

	private static IntPtr DllImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? searchPath) {
		if (libraryName.Contains("Nexen") || libraryName.Contains("SkiaSharp") || libraryName.Contains("HarfBuzz")) {
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

			string homePath = Path.Combine(ConfigManager.HomeFolder, libraryName);
			LogNativeLibraryLoad(libraryName, homePath);
			return NativeLibrary.Load(homePath);
		}

		return IntPtr.Zero;
	}

	private static void SyncNativeDependenciesFromAppBase() {
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
		GC.KeepAlive(typeof(Avalonia.Svg.Skia.SvgImageExtension).Assembly);

		return AppBuilder.Configure<App>()
				.UseReactiveUI()
				.UsePlatformDetect()
				.With(new Win32PlatformOptions { })
				.With(new X11PlatformOptions {
					EnableInputFocusProxy = Environment.GetEnvironmentVariable("XDG_CURRENT_DESKTOP") == "gamescope",
				})
				.With(new AvaloniaNativePlatformOptions { RenderingMode = new AvaloniaNativeRenderingMode[] { AvaloniaNativeRenderingMode.OpenGl, AvaloniaNativeRenderingMode.Software } })
				.LogToTrace();
	}
}
