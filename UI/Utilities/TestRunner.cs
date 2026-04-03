using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Interop;

namespace Nexen.Utilities;
internal sealed class TestRunner {
	private const int ErrorMissingRom = -1;
	private const int ErrorLoadRomFailed = -2;
	private const int ErrorTimeout = -3;

	internal static int Run(string[] args) {
		ConfigManager.DisableSaveSettings = true;
		CommandLineHelper commandLineHelper = new(args, true);

		if (commandLineHelper.FilesToLoad.Count != 1) {
			Console.Error.WriteLine("TestRunner: expected exactly one ROM argument.");
			return ErrorMissingRom;
		}

		EmuApi.InitDll();

		int timeout = commandLineHelper.TestRunnerTimeout;
		ConfigManager.Config.ApplyConfig();
		string romPath = commandLineHelper.FilesToLoad[0];

		EmuApi.InitializeEmu(ConfigManager.HomeFolder, IntPtr.Zero, IntPtr.Zero, true, true, true, true);
		try {
			EmuApi.Pause();

			ConfigApi.SetEmulationFlag(EmulationFlags.ConsoleMode, true);

			if (!EmuApi.LoadRom(romPath, string.Empty)) {
				Console.Error.WriteLine($"TestRunner: failed to load ROM '{romPath}'.");
				return ErrorLoadRomFailed;
			}

			DebugWorkspaceManager.Load();

			foreach (string luaScript in commandLineHelper.LuaScriptsToLoad) {
				try {
					string script = File.ReadAllText(luaScript);
					DebugApi.LoadScript(luaScript, Path.GetDirectoryName(luaScript) ?? Program.OriginalFolder, script);
				} catch (Exception ex) {
					Console.Error.WriteLine($"TestRunner: failed to load Lua script '{luaScript}': {ex.Message}");
				}
			}

			ConfigApi.SetEmulationFlag(EmulationFlags.MaximumSpeed, true);
			EmuApi.Resume();

			int result = ErrorTimeout;
			Stopwatch sw = Stopwatch.StartNew();
			while (sw.ElapsedMilliseconds < timeout * 1000) {
				System.Threading.Thread.Sleep(100);

				if (!EmuApi.IsRunning()) {
					result = EmuApi.GetStopCode();
					break;
				}
			}

			if (result == ErrorTimeout) {
				Console.Error.WriteLine($"TestRunner: timed out after {timeout}s (ROM '{romPath}').");
			}

			return result;
		} finally {
			EmuApi.Stop();
			EmuApi.Release();
		}
	}
}
