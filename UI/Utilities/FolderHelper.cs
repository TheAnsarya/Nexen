using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Nexen.Config;

namespace Nexen.Utilities;
public static class FolderHelper {
	private static HashSet<string> _romExtensions = [
			".sfc", ".smc", ".fig", ".swc", ".bs", ".st",
	".gb", ".gbc", ".gbx",
	".nes", ".unif", ".unf", ".fds", ".qd", ".studybox",
	".pce", ".sgx", ".cue",
	".sms", ".gg", ".sg", ".col",
	".md", ".gen", ".smd",
	".gba",
	".ws", ".wsc"
		];

	public static bool IsRomFile(string path) {
		string ext = Path.GetExtension(path).ToLower();
		return _romExtensions.Contains(ext);
	}

	public static bool IsArchiveFile(string path) {
		string ext = Path.GetExtension(path).ToLower();
		return ext is ".7z" or ".zip";
	}

	public static bool CheckFolderPermissions(string folder, bool checkWritePermission = true) {
		if (!Directory.Exists(folder)) {
			try {
				if (string.IsNullOrWhiteSpace(folder)) {
					return false;
				}

				Directory.CreateDirectory(folder);
			} catch (Exception ex) {
				Debug.WriteLine($"FolderHelper.CheckFolderPermissions: Create failed for '{folder}': {ex.Message}");
				return false;
			}
		}

		if (checkWritePermission) {
			try {
				string fileName = Guid.NewGuid().ToString() + ".txt";
				File.WriteAllText(Path.Combine(folder, fileName), "");
				File.Delete(Path.Combine(folder, fileName));
			} catch (Exception ex) {
				Debug.WriteLine($"FolderHelper.CheckFolderPermissions: Write check failed for '{folder}': {ex.Message}");
				return false;
			}
		}

		return true;
	}
}
