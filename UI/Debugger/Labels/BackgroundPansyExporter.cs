using Mesen.Config;
using Mesen.Debugger.Labels;
using Mesen.Interop;
using Mesen.Utilities;
using System;
using System.Threading;
using Avalonia.Threading;

namespace Mesen.Debugger.Labels
{
	/// <summary>
	/// Manages background CDL recording and periodic Pansy file export.
	/// This enables CDL collection without requiring the debugger window to be open.
	/// </summary>
	public static class BackgroundPansyExporter
	{
		private static Timer? _autoSaveTimer;
		private static RomInfo? _currentRomInfo;
		private static bool _debuggerInitialized;
		private static bool _cdlEnabled;

		/// <summary>
		/// Called when a ROM is loaded. Starts background CDL recording if enabled.
		/// </summary>
		public static void OnRomLoaded(RomInfo romInfo)
		{
			System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] OnRomLoaded: {romInfo.GetRomName()}");
			_currentRomInfo = romInfo;

			if (!ConfigManager.Config.Debug.Integration.BackgroundCdlRecording) {
				System.Diagnostics.Debug.WriteLine("[BackgroundPansy] Background recording disabled");
				return;
			}

			StartCdlRecording();
			StartAutoSaveTimer();
		}

		/// <summary>
		/// Called when a ROM is unloaded. Saves final Pansy file and stops recording.
		/// </summary>
		public static void OnRomUnloaded()
		{
			System.Diagnostics.Debug.WriteLine("[BackgroundPansy] OnRomUnloaded");
			StopAutoSaveTimer();

			if (_currentRomInfo != null && ConfigManager.Config.Debug.Integration.SavePansyOnRomUnload) {
				ExportPansy();
			}

			StopCdlRecording();
			_currentRomInfo = null;
		}

		/// <summary>
		/// Initialize the debugger and enable CDL logging for the main CPU.
		/// </summary>
		private static void StartCdlRecording()
		{
			if (_currentRomInfo == null) return;

			try {
				// Initialize debugger if not already done
				if (!_debuggerInitialized) {
					System.Diagnostics.Debug.WriteLine("[BackgroundPansy] Initializing debugger");
					DebugApi.InitializeDebugger();
					_debuggerInitialized = true;
				}

				// Enable CDL logging for the main CPU type
				var cpuType = _currentRomInfo.ConsoleType.GetMainCpuType();
				var debuggerFlag = cpuType.GetDebuggerFlag();
				System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] Enabling CDL for {cpuType} (flag: {debuggerFlag})");
				ConfigApi.SetDebuggerFlag(debuggerFlag, true);
				_cdlEnabled = true;
			} catch (Exception ex) {
				System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] Failed to start CDL recording: {ex.Message}");
			}
		}

		/// <summary>
		/// Disable CDL logging when done.
		/// </summary>
		private static void StopCdlRecording()
		{
			if (!_cdlEnabled || _currentRomInfo == null) return;

			try {
				var cpuType = _currentRomInfo.ConsoleType.GetMainCpuType();
				var debuggerFlag = cpuType.GetDebuggerFlag();
				System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] Disabling CDL for {cpuType}");
				ConfigApi.SetDebuggerFlag(debuggerFlag, false);
				_cdlEnabled = false;
			} catch (Exception ex) {
				System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] Failed to stop CDL recording: {ex.Message}");
			}
		}

		/// <summary>
		/// Start the periodic auto-save timer.
		/// </summary>
		private static void StartAutoSaveTimer()
		{
			int intervalMinutes = ConfigManager.Config.Debug.Integration.AutoSaveIntervalMinutes;
			if (intervalMinutes <= 0) {
				System.Diagnostics.Debug.WriteLine("[BackgroundPansy] Auto-save disabled (interval = 0)");
				return;
			}

			int intervalMs = intervalMinutes * 60 * 1000;
			System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] Starting auto-save timer: {intervalMinutes} minutes");
			
			_autoSaveTimer?.Dispose();
			_autoSaveTimer = new Timer(OnAutoSaveTick, null, intervalMs, intervalMs);
		}

		/// <summary>
		/// Stop the periodic auto-save timer.
		/// </summary>
		private static void StopAutoSaveTimer()
		{
			if (_autoSaveTimer != null) {
				System.Diagnostics.Debug.WriteLine("[BackgroundPansy] Stopping auto-save timer");
				_autoSaveTimer.Dispose();
				_autoSaveTimer = null;
			}
		}

		/// <summary>
		/// Timer callback - exports Pansy on UI thread.
		/// </summary>
		private static void OnAutoSaveTick(object? state)
		{
			System.Diagnostics.Debug.WriteLine("[BackgroundPansy] Auto-save tick");
			Dispatcher.UIThread.Post(() => ExportPansy());
		}

		/// <summary>
		/// Export the current Pansy file.
		/// </summary>
		private static void ExportPansy()
		{
			if (_currentRomInfo == null || string.IsNullOrEmpty(_currentRomInfo.RomPath)) {
				System.Diagnostics.Debug.WriteLine("[BackgroundPansy] Cannot export - no ROM loaded");
				return;
			}

			try {
				var memoryType = _currentRomInfo.ConsoleType.GetMainCpuType().GetPrgRomMemoryType();
				System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] Exporting Pansy for memory type: {memoryType}");
				PansyExporter.AutoExport(_currentRomInfo, memoryType);
			} catch (Exception ex) {
				System.Diagnostics.Debug.WriteLine($"[BackgroundPansy] Export failed: {ex.Message}");
			}
		}

		/// <summary>
		/// Force an immediate Pansy export (for manual trigger).
		/// </summary>
		public static void ForceExport()
		{
			ExportPansy();
		}
	}
}
