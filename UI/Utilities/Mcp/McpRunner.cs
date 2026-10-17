using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using System;
using System.IO;

namespace Nexen.Utilities.Mcp;

/// <summary>
/// Entry for --mcp mode. Mirrors TestRunner.Run for ROM load / emulator
/// bring-up, then hands control to McpServer which blocks on stdin until
/// the client disconnects.
/// </summary>
internal static class McpRunner
{
	private const int DefaultPort = 7333;

	/// <summary>Parse an explicit --mcp-port=N flag; default otherwise.</summary>
	private static int ParsePort(string[] args)
	{
		foreach(string arg in args) {
			string a = arg.StartsWith("--") ? arg.Substring(2) : arg.StartsWith("-") || arg.StartsWith("/") ? arg.Substring(1) : arg;
			if(a.StartsWith("mcp-port=", StringComparison.OrdinalIgnoreCase)) {
				string v = a.Substring("mcp-port=".Length);
				if(int.TryParse(v, out int port) && port > 0 && port < 65536) {
					return port;
				}
			}
		}
		return DefaultPort;
	}

	internal static int Run(string[] args)
	{
		Log("run entered");
		ConfigManager.DisableSaveSettings = true;
		CommandLineHelper cli = new(args, true);

		if(cli.FilesToLoad.Count != 1) {
			Log($"expected one ROM, got {cli.FilesToLoad.Count}");
			return -1;
		}

		Log("InitDll");
		EmuApi.InitDll();
		// Attach a standard controller on port 1 so the MCP set_input tool (debugger
		// input overrides) and raw $4016 reads work headlessly. In headless mode the
		// config-load can race and leave the port type None, which makes $4016 return
		// open bus and leaves input overrides with no device to drive.
		ConfigManager.Config.Snes.Port1.Type = ControllerType.SnesController;
		Log("ApplyConfig");
		ConfigManager.Config.ApplyConfig();
		Log("InitializeEmu");
		// Enable audio (noAudio=false) so the SPC700/DSP run and record_audio
		// can capture a real waveform. We still pass noVideo=true (we don't
		// open an Avalonia window), and noInput=true (debugger overrides
		// only). Audio is emulated but not pushed to the speakers in
		// console mode unless the user explicitly wants it.
		EmuApi.InitializeEmu(ConfigManager.HomeFolder, IntPtr.Zero, IntPtr.Zero, true, false, true, true);
		Log("Pause");
		EmuApi.Pause();

		ConfigApi.SetEmulationFlag(EmulationFlags.ConsoleMode, true);
		// Tell the C++ side that stdout belongs to MCP; Debugger/Lua logs
		// divert to stderr.
		// EmulationFlags.McpMode (Mesen2-only log-routing flag) not ported — MCP runs over TCP.

		// Re-assert a standard controller on port 1 and push it to native RIGHT BEFORE
		// LoadRom (after the async config-load race), so the control manager attaches a
		// real device. Without a device $4016 returns open bus and set_input has nothing
		// to drive.
		ConfigManager.Config.Snes.Port1.Type = ControllerType.SnesController;
		ConfigManager.Config.Snes.ApplyConfig();
		Log("port1 controller = " + ConfigManager.Config.Snes.Port1.Type);

		Log("LoadRom");
		if(!EmuApi.LoadRom(cli.FilesToLoad[0], string.Empty)) {
			Log("failed to load ROM");
			return -1;
		}

		Log("DebugWorkspaceManager.Load");
		DebugWorkspaceManager.Load();

		// Load any Lua scripts specified on the command line — useful for
		// seeding state before the MCP session takes over.
		foreach(string luaScript in cli.LuaScriptsToLoad) {
			try {
				string script = File.ReadAllText(luaScript);
				DebugApi.LoadScript(luaScript, Path.GetDirectoryName(luaScript) ?? Program.OriginalFolder, script);
			} catch(Exception ex) {
				Console.Error.WriteLine($"mcp: failed to load Lua {luaScript}: {ex.Message}");
			}
		}

		ConfigApi.SetEmulationFlag(EmulationFlags.MaximumSpeed, true);
		EmuApi.Resume();

		// Hand control to the MCP server. Blocks until client disconnects
		// and accept() unblocks (we rely on emu.Stop from a tool for shutdown).
		int port = ParsePort(args);
		Log($"McpServer on port {port}");
		McpServer server = new(port);
		server.Run();
		Log("McpServer returned");

		EmuApi.Stop();
		EmuApi.Release();
		return 0;
	}

	// Diagnostic logging — writes to a file so WinExe stdio quirks don't hide
	// the trace. Also mirrors to stderr for console-launched usage.
	private static string LogPath = System.IO.Path.Combine(
		Config.ConfigManager.HomeFolder, "mcp_runner.log"
	);

	private static void Log(string message)
	{
		string line = $"[{DateTime.Now:HH:mm:ss.fff}] {message}";
		try {
			System.IO.File.AppendAllText(LogPath, line + Environment.NewLine);
		} catch { }
		try {
			Console.Error.WriteLine(line);
		} catch { }
	}
}
