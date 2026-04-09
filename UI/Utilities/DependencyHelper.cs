using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace Nexen.Utilities;

class DependencyHelper {
	public static void ExtractNativeDependencies(string dest) {
		// When extracting to the app's own directory (portable mode), skip overwriting
		// native libraries that are already loaded in memory. On Linux, overwriting a
		// .so file while it's mmap'd causes segfaults in the dynamic linker.
		bool isAppBaseDir = string.Equals(
			Path.GetFullPath(dest).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar),
			Path.GetFullPath(AppContext.BaseDirectory).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar),
			OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal
		);

		using (Stream? depStream = Assembly.GetExecutingAssembly().GetManifestResourceStream("Nexen.Dependencies.zip")) {
			if (depStream is null) {
				throw new Exception("Missing dependencies.zip");
			}

			using ZipArchive zip = new(depStream);
			foreach (ZipArchiveEntry entry in zip.Entries) {
				try {
					if (entry.FullName.StartsWith("Internal")) {
						continue;
					}

					// Skip directory entries — they have no data and cause crashes on Linux
					// when ExtractToFile tries to open a directory path as a file.
					if (entry.FullName.EndsWith('/') || entry.FullName.EndsWith('\\')) {
						string dirPath = Path.Combine(dest, entry.FullName);
						if (!Directory.Exists(dirPath)) {
							Directory.CreateDirectory(dirPath);
						}
						continue;
					}

					string path = Path.Combine(dest, entry.FullName);
					string extension = Path.GetExtension(path)?.ToLowerInvariant() ?? string.Empty;
					entry.ExternalAttributes = 0;

					if (File.Exists(path)) {
						if (extension == ".bin") {
							// Don't overwrite BS-X bin files if they already exist on the disk
							continue;
						}

						FileInfo fileInfo = new(path);
						if (fileInfo.LastWriteTime == entry.LastWriteTime && fileInfo.Length == entry.Length) {
							// File unchanged — skip regardless of type
							continue;
						}

						bool isNativeLib = extension is ".dll" or ".so" or ".dylib" or ".exe";
						if (isNativeLib && isAppBaseDir && !OperatingSystem.IsWindows()) {
							// On Linux/macOS in portable mode, directly overwriting a mmap'd
							// shared library causes segfaults in the dynamic linker. Delete
							// first so the kernel keeps the old inode alive for the running
							// process, then extract to a fresh inode.
							File.Delete(path);
							entry.ExtractToFile(path, false);
						} else {
							entry.ExtractToFile(path, true);
						}
					} else {
						string? folderName = Path.GetDirectoryName(path);
						if (folderName is not null && !Directory.Exists(folderName)) {
							// Create any missing directory (e.g Satellaview)
							Directory.CreateDirectory(folderName);
						}

						entry.ExtractToFile(path, true);
					}
				} catch (Exception ex) {
					Log.Error(ex, $"[DependencyHelper] Failed to extract dependency: {entry.FullName}");
				}
			}
		}
	}

	public static string? GetFileContent(string filename) {
		using Stream? depStream = Assembly.GetExecutingAssembly().GetManifestResourceStream("Nexen.Dependencies.zip");
		if (depStream is not null) {
			using ZipArchive zip = new(depStream);
			foreach (ZipArchiveEntry entry in zip.Entries) {
				if (entry.Name == filename) {
					using Stream entryStream = entry.Open();
					using StreamReader reader = new StreamReader(entryStream);
					return reader.ReadToEnd();
				}
			}
		}

		return null;
	}
}
