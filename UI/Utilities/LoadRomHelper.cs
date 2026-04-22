using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;
using Avalonia.Controls;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.GUI.Utilities;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.ViewModels;
using Nexen.Windows;

namespace Nexen.Utilities;
public static class LoadRomHelper {
	private static int _romLoadInProgress;
	private static readonly SemaphoreSlim _serializedLoadGate = new(1, 1);
	private static long _loadOperationIdCounter;

	public static bool IsRomLoadInProgress => Interlocked.CompareExchange(ref _romLoadInProgress, 0, 0) != 0;

	private sealed class RomLoadOperationToken : IDisposable {
		private int _disposed;
		private readonly string _operationName;

		public RomLoadOperationToken(string operationName) {
			_operationName = operationName;
		}

		public void Dispose() {
			if (Interlocked.Exchange(ref _disposed, 1) != 0) {
				return;
			}

			int count = Interlocked.Decrement(ref _romLoadInProgress);
			Log.Info($"[LoadRomHelper] {_operationName} end (in-progress count: {count})");
		}
	}

	private sealed class SerializedLoadGateToken : IDisposable {
		private int _disposed;
		private readonly long _operationId;
		private readonly string _operationName;
		private readonly string _target;
		private readonly long _acquiredAtTicks;

		public SerializedLoadGateToken(long operationId, string operationName, string target) {
			_operationId = operationId;
			_operationName = operationName;
			_target = target;
			_acquiredAtTicks = Stopwatch.GetTimestamp();
		}

		public void Dispose() {
			if (Interlocked.Exchange(ref _disposed, 1) != 0) {
				return;
			}

			double heldMs = (Stopwatch.GetTimestamp() - _acquiredAtTicks) * 1000.0 / Stopwatch.Frequency;
			Log.Info($"[LoadRomHelper] serialized load gate releasing op#{_operationId} ({_operationName}, target: {_target}, thread: {Environment.CurrentManagedThreadId}, held: {heldMs:0.000}ms)");
			_serializedLoadGate.Release();
		}
	}

	private static IDisposable BeginRomLoadOperation(string operationName) {
		int count = Interlocked.Increment(ref _romLoadInProgress);
		Log.Info($"[LoadRomHelper] {operationName} begin (in-progress count: {count})");
		return new RomLoadOperationToken(operationName);
	}

	private static async Task<IDisposable> EnterSerializedLoadGateAsync(string operationName, string target) {
		long operationId = Interlocked.Increment(ref _loadOperationIdCounter);
		long waitStartedTicks = Stopwatch.GetTimestamp();
		Log.Info($"[LoadRomHelper] waiting for serialized load gate op#{operationId} ({operationName}, target: {target}, thread: {Environment.CurrentManagedThreadId})");
		await _serializedLoadGate.WaitAsync().ConfigureAwait(false);
		double waitMs = (Stopwatch.GetTimestamp() - waitStartedTicks) * 1000.0 / Stopwatch.Frequency;
		Log.Info($"[LoadRomHelper] acquired serialized load gate op#{operationId} ({operationName}, target: {target}, thread: {Environment.CurrentManagedThreadId}, wait: {waitMs:0.000}ms)");
		return new SerializedLoadGateToken(operationId, operationName, target);
	}

	internal static IDisposable BeginRomLoadOperationForTests(string operationName) {
		return BeginRomLoadOperation(operationName);
	}

	internal static int GetRomLoadInProgressCountForTests() {
		return Interlocked.CompareExchange(ref _romLoadInProgress, 0, 0);
	}

	internal static void ResetRomLoadInProgressForTests() {
		Interlocked.Exchange(ref _romLoadInProgress, 0);
	}

	internal static int GetSerializedLoadGateCountForTests() {
		return _serializedLoadGate.CurrentCount;
	}

	internal static async Task<IDisposable> AcquireSerializedLoadGateForTests(string operationName) {
		return await EnterSerializedLoadGateAsync(operationName, "test").ConfigureAwait(false);
	}

	public static async Task LoadRom(ResourcePath romPath, ResourcePath? patchPath = null) {
		Log.Info($"[LoadRomHelper] LoadRom called with: {romPath}");

		if (FolderHelper.IsArchiveFile(romPath)) {
			Log.Info($"[LoadRomHelper] File is archive, showing SelectRomWindow");
			ResourcePath? selectedRom = await SelectRomWindow.Show(romPath);
			if (selectedRom is null) {
				Log.Info($"[LoadRomHelper] No ROM selected from archive, aborting");
				return;
			}

			romPath = selectedRom.Value;
			Log.Info($"[LoadRomHelper] Selected ROM from archive: {romPath}");
		}

		if (patchPath is null && ConfigManager.Config.Preferences.AutoLoadPatches) {
			string[] extensions = new string[3] { ".ips", ".ups", ".bps" };
			foreach (string ext in extensions) {
				string file = Path.Combine(romPath.Folder, Path.GetFileNameWithoutExtension(romPath.FileName)) + ext;
				if (File.Exists(file)) {
					patchPath = file;
					Log.Info($"[LoadRomHelper] Auto-loading patch: {patchPath}");
					break;
				}
			}
		}

		InternalLoadRom(romPath, patchPath);
	}

	private static void InternalLoadRom(ResourcePath romPath, ResourcePath? patchPath) {
		Log.Info($"[LoadRomHelper] InternalLoadRom: {romPath}, patch: {patchPath?.ToString() ?? "none"}");

		//Temporarily hide selection screen to allow displaying error messages
		MainWindowViewModel.Instance.RecentGames.Visible = false;
		IDisposable loadOpToken = BeginRomLoadOperation("LoadRom");

		Task.Run(async () => {
			IDisposable? serializedGateToken = null;
			try {
				serializedGateToken = await EnterSerializedLoadGateAsync("LoadRom", romPath.ToString()).ConfigureAwait(false);

				ResourcePath romPathForCore = romPath;
				if (FolderHelper.IsArchiveFile(romPath.Path) && romPath.Compressed) {
					string? extractedPath = ArchiveHelper.ExtractArchiveEntryToTempFile(romPath);
					if (string.IsNullOrWhiteSpace(extractedPath) || !File.Exists(extractedPath)) {
						Log.Error($"[LoadRomHelper] Failed to extract archive entry: {romPath}");
						DisplayMessageHelper.DisplayMessage("Error", ResourceHelper.GetMessage("FileNotFound", romPath.ToString()));
						return;
					}

					romPathForCore = extractedPath;
					Log.Info($"[LoadRomHelper] Extracted archive entry to temp file: {romPathForCore}");
				}

				Log.Info($"[LoadRomHelper] Calling EmuApi.LoadRom...");
				//Run in another thread to prevent deadlocks etc. when emulator notifications are processed UI-side
				bool success = EmuApi.LoadRom(romPathForCore, patchPath);
				Log.Info($"[LoadRomHelper] EmuApi.LoadRom returned: {success}");

				if (success) {
					ConfigManager.Config.RecentFiles.AddRecentFile(romPath, patchPath);
					ConfigManager.Config.Save();
					Log.Info($"[LoadRomHelper] ROM loaded successfully, saved to recent files");
				} else {
					Log.Error($"[LoadRomHelper] EmuApi.LoadRom returned false - load failed");
				}
			} catch (Exception ex) {
				Log.Error(ex, $"[LoadRomHelper] EXCEPTION in LoadRom");
			} finally {
				serializedGateToken?.Dispose();
				loadOpToken.Dispose();
			}

			ShowSelectionOnScreenAfterError();
		});
	}

	public static void LoadRecentGame(string filename, bool forceLoadState) {
		Log.Info($"[LoadRomHelper] LoadRecentGame called with: {filename}, forceLoadState: {forceLoadState}");
		//Temporarily hide selection screen to allow displaying error messages
		MainWindowViewModel.Instance.RecentGames.Visible = false;
		IDisposable loadOpToken = BeginRomLoadOperation("LoadRecentGame");

		Task.Run(async () => {
			IDisposable? serializedGateToken = null;
			try {
				serializedGateToken = await EnterSerializedLoadGateAsync("LoadRecentGame", filename).ConfigureAwait(false);
				//Run in another thread to prevent deadlocks etc. when emulator notifications are processed UI-side
				if (File.Exists(filename)) {
					bool resetGame = !forceLoadState && ConfigManager.Config.Preferences.GameSelectionScreenMode == GameSelectionMode.PowerOn;
					Log.Info($"[LoadRomHelper] Calling EmuApi.LoadRecentGame (resetGame: {resetGame})");
					EmuApi.LoadRecentGame(filename, resetGame);
				} else {
					Log.Error($"[LoadRomHelper] ERROR: Recent game file not found: {filename}");
				}
			} catch (Exception ex) {
				Log.Error(ex, "[LoadRomHelper] EXCEPTION in LoadRecentGame");
			} finally {
				serializedGateToken?.Dispose();
				loadOpToken.Dispose();
			}

			ShowSelectionOnScreenAfterError();
		});
	}

	private static void ShowSelectionOnScreenAfterError() {
		if (ConfigManager.Config.Preferences.GameSelectionScreenMode != GameSelectionMode.Disabled) {
			Thread.Sleep(3100);
			if (!EmuApi.IsRunning()) {
				//No game was loaded, show game selection screen again after ~3 seconds
				//This allows error messages to be visible to the user
				Dispatcher.UIThread.Post(() => MainWindowViewModel.Instance.RecentGames.Visible = true);
			}
		}
	}

	public static async Task LoadPatchFile(string patchFile) {
		string? patchFolder = Path.GetDirectoryName(patchFile);
		if (patchFolder is null) {
			return;
		}

		List<string> romsInFolder = [];
		foreach (string filepath in Directory.EnumerateFiles(patchFolder)) {
			if (FolderHelper.IsRomFile(filepath)) {
				romsInFolder.Add(filepath);
			}
		}

		if (romsInFolder.Count == 1) {
			//There is a single rom in the same folder as the IPS/BPS patch, use it automatically
			await LoadRom(romsInFolder[0], patchFile);
		} else {
			Window? wnd = ApplicationHelper.GetMainWindow();
			if (!EmuApi.IsRunning()) {
				//Prompt the user for a rom to load
				if (await NexenMsgBox.Show(wnd, "SelectRomIps", MessageBoxButtons.OKCancel, MessageBoxIcon.Question) == DialogResult.OK) {
					string? filename = await FileDialogHelper.OpenFile(null, wnd, FileDialogHelper.RomExt);
					if (filename is not null) {
						await LoadRom(filename, patchFile);
					}
				}
			} else if (await NexenMsgBox.Show(wnd, "PatchAndReset", MessageBoxButtons.OKCancel, MessageBoxIcon.Question) == DialogResult.OK) {
				//Confirm that the user wants to patch the current rom and reset
				await LoadRom(EmuApi.GetRomInfo().RomPath, patchFile);
			}
		}
	}

	private static bool IsPatchFile(string filename) {
		using (FileStream? stream = FileHelper.OpenRead(filename)) {
			if (stream is not null) {
				byte[] header = new byte[5];
				stream.ReadExactly(header, 0, 5);
				if (header[0] == 'P' && header[1] == 'A' && header[2] == 'T' && header[3] == 'C' && header[4] == 'H') {
					return true;
				} else if ((header[0] == 'U' || header[0] == 'B') && header[1] == 'P' && header[2] == 'S' && header[3] == '1') {
					return true;
				}
			}
		}

		return false;
	}

	public static void LoadFile(string filename) {
		Log.Info($"[LoadRomHelper] LoadFile called with: {filename}");

		if (File.Exists(filename)) {
			string ext = Path.GetExtension(filename).ToLowerInvariant();
			Log.Info($"[LoadRomHelper] File exists, extension: {ext}");

			if (IsPatchFile(filename)) {
				Log.Info($"[LoadRomHelper] Detected patch file, calling LoadPatchFile");
				_ = LoadPatchFile(filename);
			} else if (ext is ("." + FileDialogHelper.NexenSaveStateExt) or ("." + FileDialogHelper.MesenSaveStateExt)) {
				Log.Info($"[LoadRomHelper] Detected save state, calling LoadStateFile");
				EmuApi.LoadStateFile(filename);
			} else if (EmuApi.IsRunning() && (ext == "." + FileDialogHelper.NexenMovieExt || ext == "." + FileDialogHelper.MesenMovieExt)) {
				Log.Info($"[LoadRomHelper] Detected movie file, calling MoviePlay");
				RecordApi.MoviePlay(filename);
			} else {
				Log.Info($"[LoadRomHelper] Treating as ROM file, calling LoadRom");
				_ = LoadRom(filename);
			}
		} else {
			Log.Error($"[LoadRomHelper] ERROR: File not found: {filename}");
			DisplayMessageHelper.DisplayMessage("Error", ResourceHelper.GetMessage("FileNotFound", filename));
		}
	}

	private static int _reloadRequestCounter = 0;
	public static void ResetReloadCounter() {
		//Reload/etc. operation is done, allow other calls
		Interlocked.Exchange(ref _reloadRequestCounter, 0);
	}

	private static void RunReloadShortcut(EmulatorShortcut shortcut) {
		//Block power cycle/power off/reload rom operations until the previous operation is done
		//This helps prevent a lot of edge cases that could happen in the UI when e.g spamming reload rom
		if (Interlocked.Increment(ref _reloadRequestCounter) == 1) {
			Task.Run(() => EmuApi.ExecuteShortcut(new ExecuteShortcutParams() { Shortcut = shortcut }));
		}
	}

	public static void Reset() {
		Task.Run(() => {
			EmuApi.ExecuteShortcut(new ExecuteShortcutParams() { Shortcut = EmulatorShortcut.ExecReset });
		});
	}
	public static void PowerCycle() { RunReloadShortcut(EmulatorShortcut.ExecPowerCycle); }
	public static void PowerOff() { RunReloadShortcut(EmulatorShortcut.ExecPowerOff); }
	public static void ReloadRom() { RunReloadShortcut(EmulatorShortcut.ExecReloadRom); }
}
