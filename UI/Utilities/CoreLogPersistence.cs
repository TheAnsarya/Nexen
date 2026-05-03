using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Interop;

namespace Nexen.Utilities;

public static class CoreLogPersistence {
	private static readonly object _lock = new();
	private static string _lastSnapshot = string.Empty;
	private static int _snapshotQueued;

	public static void MirrorSnapshot(string reason) {
		if (Interlocked.Exchange(ref _snapshotQueued, 1) == 1) {
			return;
		}

		Task.Run(() => {
			try {
				lock (_lock) {
					string snapshot = EmuApi.GetLog();
					if (snapshot == _lastSnapshot) {
						return;
					}

					string coreLogPath = Path.Combine(ConfigManager.HomeFolder, "nexen-core-log.txt");
					File.WriteAllText(coreLogPath, snapshot);
					_lastSnapshot = snapshot;
					Log.Debug($"[CoreLogPersistence] Wrote core log snapshot ({reason})");
				}
			} catch (Exception ex) {
				Log.Error(ex, $"[CoreLogPersistence] Failed to write core log snapshot ({reason})");
			} finally {
				Interlocked.Exchange(ref _snapshotQueued, 0);
			}
		});
	}
}
