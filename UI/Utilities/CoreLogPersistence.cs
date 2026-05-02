using System;
using System.IO;
using Nexen.Config;
using Nexen.Interop;

namespace Nexen.Utilities;

public static class CoreLogPersistence {
	private static readonly object _lock = new();
	private static string _lastSnapshot = string.Empty;

	public static void MirrorSnapshot(string reason) {
		lock (_lock) {
			try {
				string snapshot = EmuApi.GetLog();
				if (snapshot == _lastSnapshot) {
					return;
				}

				string coreLogPath = Path.Combine(ConfigManager.HomeFolder, "nexen-core-log.txt");
				File.WriteAllText(coreLogPath, snapshot);
				_lastSnapshot = snapshot;
				Log.Debug($"[CoreLogPersistence] Wrote core log snapshot ({reason})");
			} catch (Exception ex) {
				Log.Error(ex, $"[CoreLogPersistence] Failed to write core log snapshot ({reason})");
			}
		}
	}
}
