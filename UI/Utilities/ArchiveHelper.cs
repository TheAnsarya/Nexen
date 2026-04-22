using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using SharpCompress.Archives;
using SharpCompress.Common;
using Nexen.Utilities;

namespace Nexen.GUI.Utilities;
public sealed class ArchiveHelper {
	public static List<ArchiveRomEntry> GetArchiveRomList(string archivePath) {
		using IArchive archive = ArchiveFactory.Open(archivePath);

		return archive.Entries
			.Where(entry => !entry.IsDirectory && !string.IsNullOrWhiteSpace(entry.Key) && FolderHelper.IsRomFile(entry.Key))
			.Select(entry => new ArchiveRomEntry() {
				ArchiveKey = entry.Key ?? string.Empty,
				Filename = entry.Key ?? string.Empty,
				IsUtf8 = true
			})
			.ToList();
	}

	public static string? ExtractArchiveEntryToTempFile(ResourcePath resourcePath) {
		if (!resourcePath.Compressed || !FolderHelper.IsArchiveFile(resourcePath.Path)) {
			return resourcePath.Path;
		}

		using IArchive archive = ArchiveFactory.Open(resourcePath.Path);
		IArchiveEntry? entry = archive.Entries.FirstOrDefault(candidate =>
			!candidate.IsDirectory
			&& string.Equals(candidate.Key, resourcePath.InnerFile, StringComparison.Ordinal)
		);

		if (entry is null) {
			return null;
		}

		string cacheRoot = Path.Combine(Path.GetTempPath(), "Nexen", "archive-cache");
		Directory.CreateDirectory(cacheRoot);

		string uniqueDir = Path.Combine(cacheRoot, Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(uniqueDir);

		string outputName = Path.GetFileName(entry.Key ?? string.Empty);
		if (string.IsNullOrWhiteSpace(outputName)) {
			outputName = "rom.bin";
		}

		string outputPath = Path.Combine(uniqueDir, outputName);
		entry.WriteToFile(outputPath, new ExtractionOptions() {
			ExtractFullPath = false,
			Overwrite = true
		});

		return outputPath;
	}
}

public sealed class ArchiveRomEntry {
	public string ArchiveKey = "";
	public string Filename = "";
	public bool IsUtf8;

	public override string ToString() {
		return Filename;
	}
}
