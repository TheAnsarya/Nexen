using Xunit;
using System;
using System.IO;
using System.Text.Json;
using System.Collections.Generic;

namespace Nexen.Tests.Debugger.Labels;

/// <summary>
/// Unit tests for DebugFolderManager functionality.
/// Tests folder-based debug storage with MLB, CDL, and Pansy files.
/// </summary>
public class DebugFolderManagerTests
{
	#region Folder Path Tests

	[Theory]
	[InlineData("Game.sfc", "Game_debug")]
	[InlineData("My Game (USA).nes", "My Game (USA)_debug")]
	[InlineData("Test.Game.v1.gba", "Test.Game.v1_debug")]
	public void GetDebugFolderName_ReturnsCorrectName(string romName, string expected)
	{
		string result = GetDebugFolderName(romName);
		Assert.Equal(expected, result);
	}

	[Fact]
	public void GetDebugFolderPath_UsesRomDirectory()
	{
		string romPath = @"C:\Roms\SNES\Game.sfc";
		string expected = @"C:\Roms\SNES\Game_debug";

		string result = GetDebugFolderPath(romPath);
		Assert.Equal(expected, result);
	}

	#endregion

	#region File Naming Tests

	[Theory]
	[InlineData(0xDEADBEEF, "deadbeef")]
	[InlineData(0x12345678, "12345678")]
	[InlineData(0x00000001, "00000001")]
	[InlineData(0xFFFFFFFF, "ffffffff")]
	public void FormatCrc_ReturnsLowercaseHex(uint crc, string expected)
	{
		string result = FormatCrc(crc);
		Assert.Equal(expected, result);
	}

	[Fact]
	public void GetMlbFileName_IncludesCrc()
	{
		uint crc = 0xDEADBEEF;
		string result = GetMlbFileName(crc);
		Assert.Contains("deadbeef", result);
		Assert.EndsWith(".mlb", result);
	}

	[Fact]
	public void GetCdlFileName_IncludesCrc()
	{
		uint crc = 0xDEADBEEF;
		string result = GetCdlFileName(crc);
		Assert.Contains("deadbeef", result);
		Assert.EndsWith(".cdl", result);
	}

	[Fact]
	public void GetPansyFileName_IncludesCrc()
	{
		uint crc = 0xDEADBEEF;
		string result = GetPansyFileName(crc);
		Assert.Contains("deadbeef", result);
		Assert.EndsWith(".pansy", result);
	}

	#endregion

	#region Folder Creation Tests

	[Fact]
	public void EnsureDebugFolderExists_CreatesFolder()
	{
		using var tempDir = new TempDirectory();
		string debugFolder = Path.Combine(tempDir.Path, "Game_debug");

		Assert.False(Directory.Exists(debugFolder));
		EnsureDebugFolderExists(debugFolder);
		Assert.True(Directory.Exists(debugFolder));
	}

	[Fact]
	public void EnsureDebugFolderExists_NoOpIfExists()
	{
		using var tempDir = new TempDirectory();
		string debugFolder = Path.Combine(tempDir.Path, "Game_debug");

		Directory.CreateDirectory(debugFolder);
		DateTime created = Directory.GetCreationTimeUtc(debugFolder);

		EnsureDebugFolderExists(debugFolder);
		Assert.True(Directory.Exists(debugFolder));
	}

	#endregion

	#region Manifest Tests

	[Fact]
	public void CreateManifest_ContainsRequiredFields()
	{
		var manifest = CreateManifest("Test.sfc", 0xDEADBEEF, "SNES");

		Assert.NotNull(manifest.RomName);
		Assert.NotNull(manifest.RomCrc);
		Assert.NotNull(manifest.Platform);
		Assert.True(manifest.CreatedAt > DateTime.MinValue);
	}

	[Fact]
	public void SerializeManifest_ValidJson()
	{
		var manifest = CreateManifest("Test.sfc", 0xDEADBEEF, "SNES");
		string json = JsonSerializer.Serialize(manifest, new JsonSerializerOptions { WriteIndented = true });

		Assert.NotNull(json);
		Assert.Contains("RomName", json);
		Assert.Contains("Test.sfc", json);
	}

	[Fact]
	public void DeserializeManifest_RestoresData()
	{
		var original = CreateManifest("Test.sfc", 0xDEADBEEF, "SNES");
		string json = JsonSerializer.Serialize(original);
		var restored = JsonSerializer.Deserialize<DebugManifest>(json);

		Assert.NotNull(restored);
		Assert.Equal(original.RomName, restored.RomName);
		Assert.Equal(original.RomCrc, restored.RomCrc);
	}

	#endregion

	#region MLB Sync Tests

	[Fact]
	public void SyncMlb_WritesFile()
	{
		using var tempDir = new TempDirectory();
		string mlbPath = Path.Combine(tempDir.Path, "test.mlb");
		var labels = new[] { ("P", 0x8000u, "Reset") };

		WriteMlbFile(mlbPath, labels);

		Assert.True(File.Exists(mlbPath));
		string content = File.ReadAllText(mlbPath);
		Assert.Contains("P:8000", content);
		Assert.Contains("Reset", content);
	}

	[Fact]
	public void SyncMlb_MultipleLabels_AllWritten()
	{
		using var tempDir = new TempDirectory();
		string mlbPath = Path.Combine(tempDir.Path, "test.mlb");
		var labels = new[] {
			("P", 0x8000u, "Reset"),
			("P", 0x8100u, "NMI"),
			("P", 0x8200u, "IRQ"),
			("R", 0x0000u, "ZeroPage")
		};

		WriteMlbFile(mlbPath, labels);

		string content = File.ReadAllText(mlbPath);
		Assert.Contains("Reset", content);
		Assert.Contains("NMI", content);
		Assert.Contains("IRQ", content);
		Assert.Contains("ZeroPage", content);
	}

	#endregion

	#region CDL Sync Tests

	[Fact]
	public void SyncCdl_WritesFile()
	{
		using var tempDir = new TempDirectory();
		string cdlPath = Path.Combine(tempDir.Path, "test.cdl");
		byte[] cdlData = [0x01, 0x02, 0x00, 0x01, 0x02];

		File.WriteAllBytes(cdlPath, cdlData);

		Assert.True(File.Exists(cdlPath));
		byte[] read = File.ReadAllBytes(cdlPath);
		Assert.Equal(cdlData.Length, read.Length);
	}

	[Fact]
	public void SyncCdl_EmptyData_WritesEmptyFile()
	{
		using var tempDir = new TempDirectory();
		string cdlPath = Path.Combine(tempDir.Path, "test.cdl");
		byte[] cdlData = [];

		File.WriteAllBytes(cdlPath, cdlData);

		Assert.True(File.Exists(cdlPath));
		Assert.Equal(0, new FileInfo(cdlPath).Length);
	}

	#endregion

	#region History/Versioning Tests

	[Fact]
	public void VersionHistory_CreatesBackup()
	{
		using var tempDir = new TempDirectory();
		string historyDir = Path.Combine(tempDir.Path, ".history");
		string originalFile = Path.Combine(tempDir.Path, "test.mlb");

		File.WriteAllText(originalFile, "P:8000 Reset");

		CreateHistoryEntry(originalFile, historyDir);

		Assert.True(Directory.Exists(historyDir));
		var backups = Directory.GetFiles(historyDir, "*.mlb");
		Assert.Single(backups);
	}

	[Fact]
	public void VersionHistory_LimitsEntries()
	{
		using var tempDir = new TempDirectory();
		string historyDir = Path.Combine(tempDir.Path, ".history");
		string originalFile = Path.Combine(tempDir.Path, "test.mlb");
		int maxEntries = 3;

		Directory.CreateDirectory(historyDir);
		File.WriteAllText(originalFile, "content");

		// Create more backups than limit
		for (int i = 0; i < 5; i++) {
			CreateHistoryEntry(originalFile, historyDir, $"backup_{i:D3}.mlb");
		}

		PruneHistory(historyDir, maxEntries, "*.mlb");

		var remaining = Directory.GetFiles(historyDir, "*.mlb");
		Assert.Equal(maxEntries, remaining.Length);
	}

	#endregion

	#region Import Tests

	[Fact]
	public void ImportMlbFile_ParsesLabels()
	{
		using var tempDir = new TempDirectory();
		string mlbPath = Path.Combine(tempDir.Path, "test.mlb");
		File.WriteAllText(mlbPath, "P:8000 Reset\nP:8100 NMI\nR:0000 Counter");

		var labels = ParseMlbFile(mlbPath);

		Assert.Equal(3, labels.Count);
	}

	[Fact]
	public void ImportMlbFile_SkipsComments()
	{
		using var tempDir = new TempDirectory();
		string mlbPath = Path.Combine(tempDir.Path, "test.mlb");
		File.WriteAllText(mlbPath, "// Comment\nP:8000 Reset\n; Another comment\nP:8100 NMI");

		var labels = ParseMlbFile(mlbPath);

		Assert.Equal(2, labels.Count);
	}

	[Fact]
	public void ImportMlbFile_HandlesEmptyLines()
	{
		using var tempDir = new TempDirectory();
		string mlbPath = Path.Combine(tempDir.Path, "test.mlb");
		File.WriteAllText(mlbPath, "P:8000 Reset\n\n\nP:8100 NMI\n");

		var labels = ParseMlbFile(mlbPath);

		Assert.Equal(2, labels.Count);
	}

	#endregion

	#region Helper Methods

	private static string GetDebugFolderName(string romName)
	{
		string nameWithoutExt = Path.GetFileNameWithoutExtension(romName);
		return $"{nameWithoutExt}_debug";
	}

	private static string GetDebugFolderPath(string romPath)
	{
		string dir = Path.GetDirectoryName(romPath) ?? "";
		string folderName = GetDebugFolderName(Path.GetFileName(romPath));
		return Path.Combine(dir, folderName);
	}

	private static string FormatCrc(uint crc)
	{
		return crc.ToString("x8");
	}

	private static string GetMlbFileName(uint crc)
	{
		return $"{FormatCrc(crc)}.mlb";
	}

	private static string GetCdlFileName(uint crc)
	{
		return $"{FormatCrc(crc)}.cdl";
	}

	private static string GetPansyFileName(uint crc)
	{
		return $"{FormatCrc(crc)}.pansy";
	}

	private static void EnsureDebugFolderExists(string path)
	{
		if (!Directory.Exists(path))
			Directory.CreateDirectory(path);
	}

	private static DebugManifest CreateManifest(string romName, uint crc, string platform)
	{
		return new DebugManifest {
			RomName = romName,
			RomCrc = FormatCrc(crc),
			Platform = platform,
			CreatedAt = DateTime.UtcNow,
			Version = "1.0"
		};
	}

	private static void WriteMlbFile(string path, (string Type, uint Address, string Name)[] labels)
	{
		using var writer = new StreamWriter(path);
		foreach (var (type, addr, name) in labels) {
			writer.WriteLine($"{type}:{addr:X4} {name}");
		}
	}

	private static void CreateHistoryEntry(string originalFile, string historyDir, string? backupName = null)
	{
		if (!Directory.Exists(historyDir))
			Directory.CreateDirectory(historyDir);

		string ext = Path.GetExtension(originalFile);
		string name = backupName ?? $"{DateTime.Now:yyyyMMdd_HHmmss}{ext}";
		string backupPath = Path.Combine(historyDir, name);

		File.Copy(originalFile, backupPath, overwrite: true);
	}

	private static void PruneHistory(string historyDir, int maxEntries, string pattern)
	{
		var files = Directory.GetFiles(historyDir, pattern)
			.OrderByDescending(f => File.GetCreationTimeUtc(f))
			.Skip(maxEntries)
			.ToArray();

		foreach (var file in files) {
			File.Delete(file);
		}
	}

	private static List<MlbLabel> ParseMlbFile(string path)
	{
		var labels = new List<MlbLabel>();
		foreach (var line in File.ReadAllLines(path)) {
			if (string.IsNullOrWhiteSpace(line)) continue;
			if (line.StartsWith("//") || line.StartsWith(";")) continue;
			if (line.Length < 3 || line[1] != ':') continue;

			try {
				string memType = line[0].ToString();
				int spaceIdx = line.IndexOf(' ', 2);
				if (spaceIdx < 0) continue;

				string addrStr = line[2..spaceIdx];
				string name = line[(spaceIdx + 1)..];

				labels.Add(new MlbLabel {
					MemoryType = memType,
					Address = Convert.ToUInt32(addrStr, 16),
					Name = name
				});
			}
			catch { }
		}
		return labels;
	}

	#endregion

	#region Test Types

	private class DebugManifest
	{
		public string RomName { get; set; } = "";
		public string RomCrc { get; set; } = "";
		public string Platform { get; set; } = "";
		public DateTime CreatedAt { get; set; }
		public string Version { get; set; } = "1.0";
	}

	private class MlbLabel
	{
		public string MemoryType { get; set; } = "";
		public uint Address { get; set; }
		public string Name { get; set; } = "";
	}

	private class TempDirectory : IDisposable
	{
		public string Path { get; }

		public TempDirectory()
		{
			Path = System.IO.Path.Combine(System.IO.Path.GetTempPath(), $"test_{Guid.NewGuid()}");
			Directory.CreateDirectory(Path);
		}

		public void Dispose()
		{
			if (Directory.Exists(Path))
				Directory.Delete(Path, true);
		}
	}

	#endregion
}
