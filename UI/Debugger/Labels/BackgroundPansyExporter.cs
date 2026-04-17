using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Debugger.Labels;
/// <summary>
/// Manages background CDL recording and periodic Pansy file export.
/// This enables CDL collection without requiring the debugger window to be open.
/// </summary>
public static class BackgroundPansyExporter {
	private static Timer? _autoSaveTimer;
	private static RomInfo? _currentRomInfo;
	private static bool _cdlEnabled;
	private static bool _isUnloading;
	private static readonly object _stateLock = new();
	private static readonly SemaphoreSlim _lifecycleGate = new(1, 1);
	private static long _operationIdCounter;
	/// <summary>Cached ROM CRC32 — computed once at ROM load, never changes during a session.</summary>
	private static uint _cachedRomCrc32;

	private sealed class LifecycleGateToken : IDisposable {
		private readonly long _operationId;
		private readonly string _operationName;
		private readonly long _enteredAt;
		private int _disposed;

		public LifecycleGateToken(long operationId, string operationName) {
			_operationId = operationId;
			_operationName = operationName;
			_enteredAt = Stopwatch.GetTimestamp();
		}

		public void Dispose() {
			if (Interlocked.Exchange(ref _disposed, 1) != 0) {
				return;
			}

			double heldMs = (Stopwatch.GetTimestamp() - _enteredAt) * 1000.0 / Stopwatch.Frequency;
			Log.Info($"[BackgroundPansy] lifecycle gate release op#{_operationId} ({_operationName}, thread: {Environment.CurrentManagedThreadId}, held: {heldMs:0.000}ms)");
			_lifecycleGate.Release();
		}
	}

	private static IDisposable EnterLifecycleGate(string operationName, long operationId) {
		long waitStart = Stopwatch.GetTimestamp();
		Log.Info($"[BackgroundPansy] lifecycle gate wait op#{operationId} ({operationName}, thread: {Environment.CurrentManagedThreadId})");
		_lifecycleGate.Wait();
		double waitMs = (Stopwatch.GetTimestamp() - waitStart) * 1000.0 / Stopwatch.Frequency;
		Log.Info($"[BackgroundPansy] lifecycle gate acquire op#{operationId} ({operationName}, thread: {Environment.CurrentManagedThreadId}, wait: {waitMs:0.000}ms)");
		return new LifecycleGateToken(operationId, operationName);
	}

	private static long NextOperationId() {
		return Interlocked.Increment(ref _operationIdCounter);
	}

	/// <summary>
	/// Called when a ROM is loaded. Starts background CDL recording if enabled.
	/// </summary>
	public static void OnRomLoaded(RomInfo romInfo) {
		long operationId = NextOperationId();
		using IDisposable gate = EnterLifecycleGate("OnRomLoaded", operationId);
		Log.Info($"[BackgroundPansy] OnRomLoaded op#{operationId}: {romInfo.GetRomName()}");

		lock (_stateLock) {
			_currentRomInfo = romInfo;
			_isUnloading = false;

			// Cache ROM CRC32 once at load time — avoids re-hashing the entire ROM on every auto-save
			_cachedRomCrc32 = PansyExporter.CalculateRomCrc32(romInfo);
		}

		// Start Pansy file watching for hot reload
		PansyFileWatcher.StartWatching(romInfo);

		if (!ConfigManager.Config.Debug.Integration.BackgroundCdlRecording) {
			Log.Info($"[BackgroundPansy] Background recording disabled in config (op#{operationId})");
			return;
		}

		StartCdlRecording(operationId);
		StartAutoSaveTimer(operationId);
	}

	/// <summary>
	/// Called when a ROM is unloaded. Saves final Pansy file and stops recording.
	/// </summary>
	public static void OnRomUnloaded() {
		long operationId = NextOperationId();
		using IDisposable gate = EnterLifecycleGate("OnRomUnloaded", operationId);
		Log.Info($"[BackgroundPansy] OnRomUnloaded op#{operationId} (thread: {Environment.CurrentManagedThreadId})");

		RomInfo? romInfoSnapshot;
		bool shouldExportOnUnload;
		lock (_stateLock) {
			_isUnloading = true;
			romInfoSnapshot = _currentRomInfo;
			shouldExportOnUnload = romInfoSnapshot is not null && ConfigManager.Config.Debug.Integration.SavePansyOnRomUnload;
		}

		StopAutoSaveTimer(operationId);

		// Stop Pansy file watching
		PansyFileWatcher.StopWatching();

		if (shouldExportOnUnload) {
			ExportPansy("OnRomUnloaded", operationId, lifecycleGateHeld: true);
		}

		StopCdlRecording(operationId);

		lock (_stateLock) {
			_currentRomInfo = null;
			_cachedRomCrc32 = 0;
			_isUnloading = false;
		}
	}

	/// <summary>
	/// Start lightweight CDL recording without the full debugger.
	/// Uses LightweightCdlRecorder (~15ns per instruction) instead of
	/// InitializeDebugger() (~200-700ns per instruction with 30-50% overhead).
	/// </summary>
	private static void StartCdlRecording(long parentOperationId) {
		bool hasRom;
		lock (_stateLock) {
			hasRom = _currentRomInfo is not null;
		}

		if (!hasRom) return;

		try {
			Log.Info($"[BackgroundPansy] Starting lightweight CDL recording (parent op#{parentOperationId}, thread: {Environment.CurrentManagedThreadId})");
			DebugApi.StartLightweightCdl();
			_cdlEnabled = true;
			Log.Info($"[BackgroundPansy] Started lightweight CDL recording (parent op#{parentOperationId})");
		} catch (Exception ex) {
			Log.Error(ex, "[BackgroundPansy] Failed to start lightweight CDL recording");
		}
	}

	/// <summary>
	/// Stop lightweight CDL recording.
	/// </summary>
	private static void StopCdlRecording(long parentOperationId) {
		if (!_cdlEnabled) return;

		try {
			Log.Info($"[BackgroundPansy] Stopping lightweight CDL recording (parent op#{parentOperationId}, thread: {Environment.CurrentManagedThreadId})");
			DebugApi.StopLightweightCdl();
			_cdlEnabled = false;
			Log.Info($"[BackgroundPansy] Stopped lightweight CDL recording (parent op#{parentOperationId})");
		} catch (Exception ex) {
			Log.Error(ex, "[BackgroundPansy] Failed to stop lightweight CDL recording");
		}
	}

	/// <summary>
	/// Start the periodic auto-save timer.
	/// </summary>
	private static void StartAutoSaveTimer(long parentOperationId) {
		int intervalMinutes = ConfigManager.Config.Debug.Integration.AutoSaveIntervalMinutes;
		if (intervalMinutes <= 0) {
			Log.Info($"[BackgroundPansy] Auto-save disabled (interval = 0, parent op#{parentOperationId})");
			return;
		}

		int intervalMs = intervalMinutes * 60 * 1000;
		Log.Info($"[BackgroundPansy] Starting auto-save timer: {intervalMinutes} minutes (parent op#{parentOperationId})");

		_autoSaveTimer?.Dispose();
		_autoSaveTimer = new Timer(OnAutoSaveTick, null, intervalMs, intervalMs);
	}

	/// <summary>
	/// Stop the periodic auto-save timer.
	/// </summary>
	private static void StopAutoSaveTimer(long parentOperationId) {
		if (_autoSaveTimer is not null) {
			Log.Info($"[BackgroundPansy] Stopping auto-save timer (parent op#{parentOperationId})");
			_autoSaveTimer.Dispose();
			_autoSaveTimer = null;
		}
	}

	/// <summary>
	/// Timer callback - runs Pansy export on threadpool thread (not UI thread).
	/// File I/O should never block the UI thread.
	/// </summary>
	private static void OnAutoSaveTick(object? state) {
		long operationId = NextOperationId();
		Log.Info($"[BackgroundPansy] Auto-save tick op#{operationId}");
		Task.Run(() => ExportPansy("AutoSaveTick", operationId));
	}

	/// <summary>
	/// Export the current Pansy file.
	/// </summary>
	private static void ExportPansy(string source, long parentOperationId, bool lifecycleGateHeld = false) {
		long operationId = NextOperationId();
		IDisposable? gate = null;
		if (!lifecycleGateHeld) {
			if (!_lifecycleGate.Wait(0)) {
				Log.Info($"[BackgroundPansy] Export deferred op#{operationId} (source: {source}, parent op#{parentOperationId}) - lifecycle gate busy");
				_lifecycleGate.Wait();
			}

			gate = new LifecycleGateToken(operationId, $"ExportPansy:{source}");
		}

		RomInfo? romInfoSnapshot;
		uint romCrc32Snapshot;
		bool unloadingSnapshot;
		lock (_stateLock) {
			romInfoSnapshot = _currentRomInfo;
			romCrc32Snapshot = _cachedRomCrc32;
			unloadingSnapshot = _isUnloading;
		}

		if (romInfoSnapshot is null || string.IsNullOrEmpty(romInfoSnapshot.RomPath)) {
			Log.Info($"[BackgroundPansy] Cannot export op#{operationId} (source: {source}) - no ROM loaded");
			return;
		}

		try {
			var memoryType = romInfoSnapshot.ConsoleType.GetMainCpuType().GetPrgRomMemoryType();
			Log.Info($"[BackgroundPansy] Exporting Pansy op#{operationId} (source: {source}, parent op#{parentOperationId}, unloading: {unloadingSnapshot}, memory type: {memoryType}, thread: {Environment.CurrentManagedThreadId})");
			PansyExporter.AutoExport(romInfoSnapshot, memoryType, romCrc32Snapshot);
			Log.Info($"[BackgroundPansy] Export complete op#{operationId} (source: {source})");
		} catch (Exception ex) {
			Log.Error(ex, $"[BackgroundPansy] Export failed op#{operationId} (source: {source})");
		} finally {
			gate?.Dispose();
		}
	}

	/// <summary>
	/// Force an immediate Pansy export (for manual trigger).
	/// </summary>
	public static void ForceExport() {
		long operationId = NextOperationId();
		ExportPansy("ForceExport", operationId);
	}
}
