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
	private static readonly Dictionary<string, string[]> _systemRomExtensions = new(StringComparer.OrdinalIgnoreCase) {
		["snes"] = [".sfc", ".fig", ".smc", ".swc", ".bs", ".st", ".spc"],
		["nes"] = [".nes", ".fds", ".qd", ".unif", ".unf", ".studybox", ".nsf", ".nsfe"],
		["gb"] = [".gb", ".gbc", ".gbx", ".gbs"],
		["gba"] = [".gba"],
		["genesis"] = [".md", ".gen", ".smd", ".bin"],
		["pce"] = [".pce", ".sgx", ".cue", ".hes"],
		["sms"] = [".sms", ".gg"],
		["sg1000"] = [".sg"],
		["coleco"] = [".col"],
		["lynx"] = [".lnx", ".lyx", ".o", ".atari-lynx"],
		["ws"] = [".ws", ".wsc"],
		["channelf"] = [".chf", ".bin"]
	};

	private static readonly HashSet<string> _romExtensions = new(
		_systemRomExtensions.Values.SelectMany(v => v),
		StringComparer.OrdinalIgnoreCase
	);

	public static bool IsRomFile(string path) {
		string ext = Path.GetExtension(path).ToLowerInvariant();
		return _romExtensions.Contains(ext);
	}

	public static string[] GetRomExtensions() {
		return _romExtensions.OrderBy(ext => ext, StringComparer.OrdinalIgnoreCase).ToArray();
	}

	public static string[] GetRomExtensionsForSystem(string systemId) {
		return _systemRomExtensions.TryGetValue(systemId, out string[]? extensions)
			? extensions.ToArray()
			: [];
	}

	public static string[] GetRomFilePatterns() {
		return GetRomExtensions().Select(ext => "*" + ext).ToArray();
	}

	public static string[] GetRomFilePatternsForSystem(string systemId) {
		return GetRomExtensionsForSystem(systemId).Select(ext => "*" + ext).ToArray();
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
