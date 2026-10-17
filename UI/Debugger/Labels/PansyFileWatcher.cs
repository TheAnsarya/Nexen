using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Debugger.Integration;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Debugger.Labels;

/// <summary>
/// Watches a Pansy metadata file for external changes and triggers automatic re-import.
/// Used for hot reload: when external tools (Peony, Pansy UI, CLI merge) modify
/// the .pansy file, Nexen detects the change and re-imports without restart.
/// </summary>
/// <remarks>
/// Controlled by <see cref="IntegrationConfig.EnableFileWatching"/> and
/// <see cref="IntegrationConfig.AutoReloadOnExternalChange"/>.
/// Changes are debounced with a 500ms window to avoid rapid re-imports.
/// </remarks>
public static class PansyFileWatcher {
	private static FileSystemWatcher? _watcher;
	private static Timer? _debounceTimer;
	private static RomInfo? _currentRomInfo;
	private static string? _watchedPath;
	private static readonly object _lock = new();

	/// <summary>Debounce interval to avoid rapid re-imports from multiple file events.</summary>
	private const int DebounceMs = 500;

	/// <summary>
	/// Start watching the Pansy file for the loaded ROM.
	/// Call from ROM load path (after BackgroundPansyExporter.OnRomLoaded).
	/// </summary>
	public static void StartWatching(RomInfo romInfo) {
		StopWatching();

		if (!ConfigManager.Config.Debug.Integration.EnableFileWatching) {
			Log.Info("[PansyFileWatcher] File watching disabled in config");
			return;
		}

		_currentRomInfo = romInfo;
		_watchedPath = DebugFolderManager.GetPansyPath(romInfo);

		string? directory = Path.GetDirectoryName(_watchedPath);
		string fileName = Path.GetFileName(_watchedPath);

		if (string.IsNullOrEmpty(directory)) {
			Log.Info("[PansyFileWatcher] Cannot watch — no directory for Pansy path");
			return;
		}

		// Ensure directory exists so the watcher can attach
		if (!Directory.Exists(directory)) {
			Directory.CreateDirectory(directory);
		}

		try {
			_watcher = new FileSystemWatcher(directory, fileName) {
				NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.Size,
				EnableRaisingEvents = true
			};

			_watcher.Changed += OnFileChanged;
			_watcher.Created += OnFileChanged;

			Log.Info($"[PansyFileWatcher] Watching: {_watchedPath}");
		} catch (Exception ex) {
			Log.Error(ex, "[PansyFileWatcher] Failed to start file watcher");
			_watcher?.Dispose();
			_watcher = null;
		}
	}

	/// <summary>
	/// Stop watching and dispose resources. Call on ROM unload.
	/// </summary>
	public static void StopWatching() {
		lock (_lock) {
			_debounceTimer?.Dispose();
			_debounceTimer = null;
		}

		if (_watcher is not null) {
			Log.Info("[PansyFileWatcher] Stopping file watcher");
			_watcher.EnableRaisingEvents = false;
			_watcher.Changed -= OnFileChanged;
			_watcher.Created -= OnFileChanged;
			_watcher.Dispose();
			_watcher = null;
		}

		_currentRomInfo = null;
		_watchedPath = null;
	}

	/// <summary>
	/// Debounced handler for file system change events.
	/// Resets the timer on each event so rapid changes coalesce into one re-import.
	/// </summary>
	private static void OnFileChanged(object sender, FileSystemEventArgs e) {
		lock (_lock) {
			_debounceTimer?.Dispose();
			_debounceTimer = new Timer(OnDebounceElapsed, null, DebounceMs, Timeout.Infinite);
		}
	}

	/// <summary>
	/// Called after debounce interval expires. Triggers the actual re-import.
	/// </summary>
	private static void OnDebounceElapsed(object? state) {
		lock (_lock) {
			_debounceTimer?.Dispose();
			_debounceTimer = null;
		}

		if (!ConfigManager.Config.Debug.Integration.AutoReloadOnExternalChange) {
			Log.Info("[PansyFileWatcher] Auto-reload disabled, ignoring change");
			return;
		}

		if (_watchedPath is null || _currentRomInfo is null) {
			return;
		}

		Task.Run(() => ReimportPansy());
	}

	/// <summary>
	/// Re-import the Pansy file on a background thread. Posts notification to UI thread.
	/// </summary>
	private static void ReimportPansy() {
		if (_watchedPath is null || !File.Exists(_watchedPath)) {
			return;
		}

		try {
			Log.Info($"[PansyFileWatcher] Re-importing: {_watchedPath}");
			var result = PansyImporter.Import(_watchedPath, showResult: false);

			if (result.Success) {
				int total = result.SymbolsImported + result.CommentsImported
					+ result.CodeOffsetsImported + result.MemoryRegionsImported
					+ result.CrossReferencesImported;

				Log.Info($"[PansyFileWatcher] Re-import complete: {total} items "
					+ $"(symbols={result.SymbolsImported}, comments={result.CommentsImported})");
			} else {
				Log.Info($"[PansyFileWatcher] Re-import failed: {result.Error}");
			}
		} catch (Exception ex) {
			Log.Error(ex, "[PansyFileWatcher] Re-import failed");
		}
	}
}
