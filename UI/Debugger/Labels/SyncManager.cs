using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Interop;

namespace Nexen.Debugger.Labels;

/// <summary>
/// Change type for sync events.
/// </summary>
public enum SyncChangeType {
	Created,
	Modified,
	Deleted,
	Renamed
}

/// <summary>
/// Information about a file change.
/// </summary>
public sealed class SyncChangeInfo {
	public SyncChangeType ChangeType { get; init; }
	public string FilePath { get; init; } = "";
	public string? OldPath { get; init; }
	public DateTime Timestamp { get; init; }
	public string FileName => Path.GetFileName(FilePath);
}

/// <summary>
/// Conflict resolution strategy.
/// </summary>
public enum ConflictResolution {
	KeepLocal,    // Keep local changes, discard external
	AcceptExternal, // Accept external changes, discard local
	Merge,        // Attempt to merge both
	Ask          // Prompt user for decision
}

/// <summary>
/// Manages bidirectional file synchronization between Pansy and Nexen native formats.
/// Uses FileSystemWatcher to detect external changes.
/// </summary>
public sealed class SyncManager : IDisposable {
	private readonly Dictionary<string, FileSystemWatcher> _watchers = new();
	private readonly ConcurrentQueue<SyncChangeInfo> _pendingChanges = new();
	private readonly object _syncLock = new();
	private readonly SemaphoreSlim _processingSemaphore = new(1, 1);

	private CancellationTokenSource? _processingCts;
	private RomInfo? _currentRom;
	private bool _disposed;

	/// <summary>
	/// Event fired when an external file change is detected.
	/// </summary>
	public event EventHandler<SyncChangeInfo>? ChangeDetected;

	/// <summary>
	/// Event fired when sync status changes.
	/// </summary>
	public event EventHandler<bool>? SyncStatusChanged;

	/// <summary>
	/// Event fired when a conflict needs resolution.
	/// Reserved for future interactive conflict resolution UI.
	/// </summary>
#pragma warning disable CS0067 // Event is never used - reserved for future conflict resolution feature
	public event EventHandler<SyncConflictEventArgs>? ConflictDetected;
#pragma warning restore CS0067

	/// <summary>
	/// Whether sync is currently enabled.
	/// </summary>
	public bool IsEnabled { get; private set; }

	/// <summary>
	/// Whether a sync operation is in progress.
	/// </summary>
	public bool IsSyncing { get; private set; }

	/// <summary>
	/// Number of pending changes in the queue.
	/// </summary>
	public int PendingChangeCount => _pendingChanges.Count;

	/// <summary>
	/// Enable file watching for the given ROM.
	/// </summary>
	public void Enable(RomInfo romInfo) {
		if (_disposed) return;

		lock (_syncLock) {
			_currentRom = romInfo;
			IsEnabled = true;

			// Get the debug folder for this ROM
			string debugFolder = DebugFolderManager.GetRomDebugFolder(romInfo);

			if (Directory.Exists(debugFolder)) {
				StartWatching(debugFolder);
			}
		}

		System.Diagnostics.Debug.WriteLine($"[SyncManager] Enabled for: {romInfo.RomPath}");
	}

	/// <summary>
	/// Disable file watching.
	/// </summary>
	public void Disable() {
		lock (_syncLock) {
			IsEnabled = false;
			_currentRom = null;
			StopAllWatchers();
		}

		System.Diagnostics.Debug.WriteLine("[SyncManager] Disabled");
	}

	/// <summary>
	/// Start watching a folder for changes.
	/// </summary>
	private void StartWatching(string folderPath) {
		if (_watchers.ContainsKey(folderPath)) {
			return;
		}

		try {
			var watcher = new FileSystemWatcher(folderPath) {
				NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.FileName | NotifyFilters.CreationTime,
				EnableRaisingEvents = true,
				IncludeSubdirectories = false
			};

			// Watch for Pansy, label, and CDL files
			watcher.Filter = "*.*";

			watcher.Changed += OnFileChanged;
			watcher.Created += OnFileCreated;
			watcher.Deleted += OnFileDeleted;
			watcher.Renamed += OnFileRenamed;
			watcher.Error += OnWatcherError;

			_watchers[folderPath] = watcher;

			System.Diagnostics.Debug.WriteLine($"[SyncManager] Started watching: {folderPath}");
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[SyncManager] Failed to watch {folderPath}: {ex.Message}");
		}
	}

	/// <summary>
	/// Stop all file watchers.
	/// </summary>
	private void StopAllWatchers() {
		foreach (var watcher in _watchers.Values) {
			watcher.EnableRaisingEvents = false;
			watcher.Dispose();
		}

		_watchers.Clear();
	}

	/// <summary>
	/// Handle file changed event.
	/// </summary>
	private void OnFileChanged(object sender, FileSystemEventArgs e) {
		if (!IsRelevantFile(e.FullPath)) return;

		EnqueueChange(new SyncChangeInfo {
			ChangeType = SyncChangeType.Modified,
			FilePath = e.FullPath,
			Timestamp = DateTime.Now
		});
	}

	/// <summary>
	/// Handle file created event.
	/// </summary>
	private void OnFileCreated(object sender, FileSystemEventArgs e) {
		if (!IsRelevantFile(e.FullPath)) return;

		EnqueueChange(new SyncChangeInfo {
			ChangeType = SyncChangeType.Created,
			FilePath = e.FullPath,
			Timestamp = DateTime.Now
		});
	}

	/// <summary>
	/// Handle file deleted event.
	/// </summary>
	private void OnFileDeleted(object sender, FileSystemEventArgs e) {
		if (!IsRelevantFile(e.FullPath)) return;

		EnqueueChange(new SyncChangeInfo {
			ChangeType = SyncChangeType.Deleted,
			FilePath = e.FullPath,
			Timestamp = DateTime.Now
		});
	}

	/// <summary>
	/// Handle file renamed event.
	/// </summary>
	private void OnFileRenamed(object sender, RenamedEventArgs e) {
		if (!IsRelevantFile(e.FullPath)) return;

		EnqueueChange(new SyncChangeInfo {
			ChangeType = SyncChangeType.Renamed,
			FilePath = e.FullPath,
			OldPath = e.OldFullPath,
			Timestamp = DateTime.Now
		});
	}

	/// <summary>
	/// Handle watcher error.
	/// </summary>
	private void OnWatcherError(object sender, ErrorEventArgs e) {
		System.Diagnostics.Debug.WriteLine($"[SyncManager] Watcher error: {e.GetException().Message}");
	}

	/// <summary>
	/// Check if a file is relevant for sync.
	/// </summary>
	private static bool IsRelevantFile(string path) {
		string ext = Path.GetExtension(path).ToLowerInvariant();
		return ext is ".pansy" or ".nexen-labels" or ".mlb" or ".cdl";
	}

	/// <summary>
	/// Enqueue a change for processing.
	/// </summary>
	private void EnqueueChange(SyncChangeInfo change) {
		// Deduplicate rapid changes to the same file
		_pendingChanges.Enqueue(change);

		// Fire event on UI thread
		Dispatcher.UIThread.Post(() => ChangeDetected?.Invoke(this, change));

		// Start processing if not already
		StartProcessingChanges();
	}

	/// <summary>
	/// Start processing pending changes.
	/// </summary>
	private void StartProcessingChanges() {
		if (_processingCts is not null) return;

		_processingCts = new CancellationTokenSource();
		_ = ProcessChangesAsync(_processingCts.Token);
	}

	/// <summary>
	/// Process pending changes asynchronously.
	/// </summary>
	private async Task ProcessChangesAsync(CancellationToken cancellationToken) {
		// Debounce - wait a bit for rapid changes to settle
		await Task.Delay(500, cancellationToken);

		await _processingSemaphore.WaitAsync(cancellationToken);
		try {
			IsSyncing = true;
			SyncStatusChanged?.Invoke(this, true);

			while (_pendingChanges.TryDequeue(out var change)) {
				if (cancellationToken.IsCancellationRequested) break;

				await ProcessChangeAsync(change, cancellationToken);
			}
		} finally {
			IsSyncing = false;
			_processingCts = null;
			_processingSemaphore.Release();

			Dispatcher.UIThread.Post(() => SyncStatusChanged?.Invoke(this, false));
		}
	}

	/// <summary>
	/// Process a single change.
	/// </summary>
	private async Task ProcessChangeAsync(SyncChangeInfo change, CancellationToken cancellationToken) {
		if (_currentRom is null) return;

		try {
			string ext = Path.GetExtension(change.FilePath).ToLowerInvariant();

			switch (change.ChangeType) {
				case SyncChangeType.Modified:
				case SyncChangeType.Created:
					await HandleFileModified(change, ext, cancellationToken);
					break;

				case SyncChangeType.Deleted:
					// Log deletion but don't auto-delete from memory
					System.Diagnostics.Debug.WriteLine($"[SyncManager] File deleted: {change.FilePath}");
					break;

				case SyncChangeType.Renamed:
					System.Diagnostics.Debug.WriteLine($"[SyncManager] File renamed: {change.OldPath} -> {change.FilePath}");
					break;
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[SyncManager] Error processing change: {ex.Message}");
		}
	}

	/// <summary>
	/// Handle a file modification.
	/// </summary>
	private async Task HandleFileModified(SyncChangeInfo change, string ext, CancellationToken cancellationToken) {
		if (_currentRom is null) return;

		var config = ConfigManager.Config.Debug.Integration;

		switch (ext) {
			case ".nexen-labels" when config.AutoLoadLabelFiles:
			case ".mlb" when config.AutoLoadLabelFiles:
				// Wait for file to be released
				await WaitForFileAccess(change.FilePath, cancellationToken);
				await Dispatcher.UIThread.InvokeAsync(() => {
					NexenLabelFile.Import(change.FilePath, showResult: false);
					System.Diagnostics.Debug.WriteLine($"[SyncManager] Imported labels: {change.FilePath}");
				});
				break;

			case ".cdl" when config.AutoLoadCdlFiles:
				await WaitForFileAccess(change.FilePath, cancellationToken);
				await Dispatcher.UIThread.InvokeAsync(() => {
					ImportCdlFile(change.FilePath);
					System.Diagnostics.Debug.WriteLine($"[SyncManager] Imported CDL: {change.FilePath}");
				});
				break;

			case ".pansy":
				// Pansy import would need a dedicated importer
				// For now just log the change
				System.Diagnostics.Debug.WriteLine($"[SyncManager] Pansy file changed: {change.FilePath}");
				break;
		}
	}

	/// <summary>
	/// Import CDL data from a file.
	/// </summary>
	private void ImportCdlFile(string path) {
		if (_currentRom is null) return;

		try {
			var cpuType = _currentRom.ConsoleType.GetMainCpuType();
			var memType = cpuType.GetPrgRomMemoryType();

			byte[] cdlData = File.ReadAllBytes(path);
			int memSize = DebugApi.GetMemorySize(memType);

			if (cdlData.Length == memSize) {
				DebugApi.SetCdlData(memType, cdlData, cdlData.Length);
			}
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"[SyncManager] CDL import failed: {ex.Message}");
		}
	}

	/// <summary>
	/// Wait for a file to become accessible.
	/// </summary>
	private static async Task WaitForFileAccess(string path, CancellationToken cancellationToken, int maxAttempts = 10) {
		for (int i = 0; i < maxAttempts; i++) {
			cancellationToken.ThrowIfCancellationRequested();

			try {
				using var stream = File.Open(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
				return;
			} catch (IOException) {
				await Task.Delay(100, cancellationToken);
			}
		}
	}

	/// <summary>
	/// Force a sync of all files in the debug folder.
	/// </summary>
	public async Task ForceSyncAsync() {
		if (_currentRom is null || !IsEnabled) return;

		string debugFolder = DebugFolderManager.GetRomDebugFolder(_currentRom);
		if (!Directory.Exists(debugFolder)) return;

		IsSyncing = true;
		SyncStatusChanged?.Invoke(this, true);

		try {
			// Import labels if exists (nexen-labels first, then legacy MLB)
			string labelsPath = DebugFolderManager.GetLabelsPath(_currentRom);
			if (File.Exists(labelsPath)) {
				await Dispatcher.UIThread.InvokeAsync(() => NexenLabelFile.Import(labelsPath, showResult: false));
			} else {
				string mlbPath = DebugFolderManager.GetLegacyMlbPath(_currentRom);
				if (File.Exists(mlbPath)) {
					await Dispatcher.UIThread.InvokeAsync(() => NexenLabelFile.Import(mlbPath, showResult: false));
				}
			}

			// Import CDL if exists
			string cdlPath = DebugFolderManager.GetCdlPath(_currentRom);
			if (File.Exists(cdlPath)) {
				await Dispatcher.UIThread.InvokeAsync(() => ImportCdlFile(cdlPath));
			}
		} finally {
			IsSyncing = false;
			SyncStatusChanged?.Invoke(this, false);
		}
	}

	/// <summary>
	/// Export all current data to the debug folder.
	/// </summary>
	public void ExportAll() {
		if (_currentRom is null) return;

		var cpuType = _currentRom.ConsoleType.GetMainCpuType();
		var memType = cpuType.GetPrgRomMemoryType();

		DebugFolderManager.ExportAllFiles(_currentRom, memType);
	}

	/// <summary>
	/// Dispose resources.
	/// </summary>
	public void Dispose() {
		if (_disposed) return;
		_disposed = true;

		_processingCts?.Cancel();
		StopAllWatchers();
		_processingSemaphore.Dispose();
	}
}

/// <summary>
/// Event args for sync conflicts.
/// </summary>
public sealed class SyncConflictEventArgs : EventArgs {
	public string FilePath { get; init; } = "";
	public string LocalTimestamp { get; init; } = "";
	public string ExternalTimestamp { get; init; } = "";
	public ConflictResolution Resolution { get; set; } = ConflictResolution.Ask;
}
