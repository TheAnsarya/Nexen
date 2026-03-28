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

	public static string OriginalFolder { get; private set; }
	public static string[] CommandLineArgs { get; private set; } = [];

	public static string ExePath => Process.GetCurrentProcess().MainModule?.FileName ?? Path.Join(Path.GetDirectoryName(AppContext.BaseDirectory), "Nexen.exe");

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
				return NativeLibrary.Load(localPath);
			}

			return NativeLibrary.Load(Path.Combine(ConfigManager.HomeFolder, libraryName));
		}

		return IntPtr.Zero;
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
