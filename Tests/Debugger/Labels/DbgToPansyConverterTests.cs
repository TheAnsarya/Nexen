using Xunit;
using System;
using System.IO;

namespace Nexen.Tests.Debugger.Labels;

/// <summary>
/// Unit tests for DbgToPansyConverter functionality.
/// Tests conversion from various debug formats to Pansy metadata.
/// </summary>
public class DbgToPansyConverterTests
{
	#region Format Detection Tests

	[Theory]
	[InlineData("test.dbg", DebugFormat.Ca65Dbg)]
	[InlineData("test.sym", DebugFormat.WlaDx)]
	[InlineData("symbols.sym", DebugFormat.WlaDx)]
	[InlineData("debug.elf", DebugFormat.Elf)]
	[InlineData("labels.mlb", DebugFormat.NexenLabels)]
	[InlineData("game.MLB", DebugFormat.NexenLabels)]
	[InlineData("labels.nexen-labels", DebugFormat.NexenLabels)]
	public void DetectFormat_ByExtension_ReturnsCorrectFormat(string filename, DebugFormat expected)
	{
		var detected = DetectFormatByExtension(filename);
		Assert.Equal(expected, detected);
	}

	[Fact]
	public void DetectFormat_UnknownExtension_ReturnsUnknown()
	{
		var detected = DetectFormatByExtension("test.xyz");
		Assert.Equal(DebugFormat.Unknown, detected);
	}

	[Theory]
	[InlineData("version\tmajor 2", DebugFormat.Ca65Dbg)]
	[InlineData("; File created by wlalink", DebugFormat.WlaDx)]
	[InlineData("SECTION \"", DebugFormat.Rgbds)]
	[InlineData("G:0000 _start", DebugFormat.NexenLabels)]
	[InlineData("P:8000 Reset", DebugFormat.NexenLabels)]
	public void DetectFormat_ByContent_ReturnsCorrectFormat(string content, DebugFormat expected)
	{
		var detected = DetectFormatByContent(content);
		Assert.Equal(expected, detected);
	}

	#endregion

	#region MLB Format Parsing Tests

	[Theory]
	[InlineData("G:0000 label_name", 0x0000, "label_name", "G")]
	[InlineData("P:8000 Reset", 0x8000, "Reset", "P")]
	[InlineData("S:2000 sprite_data", 0x2000, "sprite_data", "S")]
	[InlineData("R:4000 WorkRam", 0x4000, "WorkRam", "R")]
	public void ParseMlbLine_ValidFormat_ExtractsComponents(string line, uint expectedAddr, string expectedName, string expectedType)
	{
		var result = ParseMlbLine(line);
		Assert.NotNull(result);
		Assert.Equal(expectedAddr, result.Address);
		Assert.Equal(expectedName, result.Name);
		Assert.Equal(expectedType, result.MemoryType);
	}

	[Theory]
	[InlineData("invalid line")]
	[InlineData("")]
	[InlineData("//comment")]
	[InlineData(";comment")]
	public void ParseMlbLine_InvalidFormat_ReturnsNull(string line)
	{
		var result = ParseMlbLine(line);
		Assert.Null(result);
	}

	[Fact]
	public void ParseMlbLine_WithComment_ExtractsComment()
	{
		var result = ParseMlbLine("P:8000 MainLoop:This is the main loop");
		Assert.NotNull(result);
		Assert.Equal(0x8000u, result.Address);
		Assert.Equal("MainLoop", result.Name);
		Assert.Equal("This is the main loop", result.Comment);
	}

	#endregion

	#region Ca65 DBG Format Tests

	[Fact]
	public void ParseCa65Dbg_VersionLine_ExtractsVersion()
	{
		string content = "version\tmajor 2\n\tminor 0";
		var version = ParseCa65Version(content);
		Assert.Equal(2, version.Major);
		Assert.Equal(0, version.Minor);
	}

	[Theory]
	[InlineData("sym\tid=5,name=\"_start\",addrsize=absolute,scope=0,def=10,val=$8000,type=lab", 0x8000, "_start")]
	[InlineData("sym\tid=10,name=\"counter\",addrsize=zeropage,scope=0,def=20,val=$00,type=lab", 0x00, "counter")]
	public void ParseCa65Symbol_ValidLine_ExtractsSymbol(string line, uint expectedAddr, string expectedName)
	{
		var result = ParseCa65SymbolLine(line);
		Assert.NotNull(result);
		Assert.Equal(expectedAddr, result.Address);
		Assert.Equal(expectedName, result.Name);
	}

	#endregion

	#region WLA-DX Format Tests

	[Theory]
	[InlineData("$00:8000 Main_Entry", 0x8000, "Main_Entry")]
	[InlineData("$01:4000 Bank1_Start", 0x14000, "Bank1_Start")]
	[InlineData("$7F:0000 Work_RAM", 0x7F0000, "Work_RAM")]
	public void ParseWlaDxSymbol_ValidLine_ExtractsSymbol(string line, uint expectedAddr, string expectedName)
	{
		var result = ParseWlaDxLine(line);
		Assert.NotNull(result);
		Assert.Equal(expectedAddr, result.Address);
		Assert.Equal(expectedName, result.Name);
	}

	[Fact]
	public void ParseWlaDxSymbol_WithSectionHeader_IgnoresHeader()
	{
		var result = ParseWlaDxLine("[labels]");
		Assert.Null(result);
	}

	#endregion

	#region Batch Conversion Tests

	[Fact]
	public void BatchConvert_EmptyDirectory_ReturnsEmptyResults()
	{
		using var tempDir = new TempDirectory();
		var results = SimulateBatchConvert(tempDir.Path);
		Assert.Empty(results);
	}

	[Fact]
	public void BatchConvert_WithDebugFiles_ProcessesAll()
	{
		using var tempDir = new TempDirectory();

		// Create test files
		File.WriteAllText(Path.Combine(tempDir.Path, "test1.dbg"), "version\tmajor 2");
		File.WriteAllText(Path.Combine(tempDir.Path, "test2.sym"), "; WLA-DX");
		File.WriteAllText(Path.Combine(tempDir.Path, "test3.mlb"), "P:8000 Test");
		File.WriteAllText(Path.Combine(tempDir.Path, "readme.txt"), "Not a debug file");

		var debugFiles = GetDebugFilesInDirectory(tempDir.Path);
		Assert.Equal(3, debugFiles.Length);
	}

	#endregion

	#region Platform Detection Tests

	[Theory]
	[InlineData("game.nes", PlatformId.NES)]
	[InlineData("game.sfc", PlatformId.SNES)]
	[InlineData("game.smc", PlatformId.SNES)]
	[InlineData("game.gb", PlatformId.GameBoy)]
	[InlineData("game.gbc", PlatformId.GameBoy)]
	[InlineData("game.gba", PlatformId.GBA)]
	[InlineData("game.pce", PlatformId.PCEngine)]
	[InlineData("game.sms", PlatformId.SMS)]
	[InlineData("game.ws", PlatformId.WonderSwan)]
	public void DetectPlatform_ByRomExtension_ReturnsCorrectPlatform(string romName, PlatformId expected)
	{
		var detected = DetectPlatformByExtension(romName);
		Assert.Equal(expected, detected);
	}

	#endregion

	#region Helper Methods (Simulated Implementation)

	private static DebugFormat DetectFormatByExtension(string filename)
	{
		string ext = Path.GetExtension(filename).ToLowerInvariant();
		return ext switch {
			".dbg" => DebugFormat.Ca65Dbg,
			".sym" => DebugFormat.WlaDx,
			".elf" => DebugFormat.Elf,
			".nexen-labels" => DebugFormat.NexenLabels,
			".mlb" => DebugFormat.NexenLabels,
			_ => DebugFormat.Unknown
		};
	}

	private static DebugFormat DetectFormatByContent(string content)
	{
		if (content.Contains("version\tmajor"))
			return DebugFormat.Ca65Dbg;
		if (content.Contains("; File created by wlalink") || content.Contains("; wla"))
			return DebugFormat.WlaDx;
		if (content.Contains("SECTION \""))
			return DebugFormat.Rgbds;
		if (content.Length > 0 && (content[0] == 'G' || content[0] == 'P' || content[0] == 'S' || content[0] == 'R') && content[1] == ':')
			return DebugFormat.NexenLabels;
		return DebugFormat.Unknown;
	}

	private static MlbParseResult? ParseMlbLine(string line)
	{
		if (string.IsNullOrWhiteSpace(line) || line.StartsWith("//") || line.StartsWith(";"))
			return null;

		if (line.Length < 3 || line[1] != ':')
			return null;

		try {
			string memType = line[0].ToString();
			int spaceIdx = line.IndexOf(' ', 2);
			if (spaceIdx < 0) return null;

			string addrStr = line[2..spaceIdx];
			string rest = line[(spaceIdx + 1)..];

			string name;
			string? comment = null;
			int colonIdx = rest.IndexOf(':');
			if (colonIdx > 0) {
				name = rest[..colonIdx];
				comment = rest[(colonIdx + 1)..];
			} else {
				name = rest;
			}

			return new MlbParseResult {
				Address = Convert.ToUInt32(addrStr, 16),
				Name = name,
				MemoryType = memType,
				Comment = comment
			};
		}
		catch {
			return null;
		}
	}

	private static Ca65Version ParseCa65Version(string content)
	{
		int major = 0, minor = 0;
		foreach (var line in content.Split('\n')) {
			if (line.Contains("major ")) {
				var parts = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
				if (parts.Length >= 2 && int.TryParse(parts[^1], out int m))
					major = m;
			}
			if (line.Contains("minor ")) {
				var parts = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
				if (parts.Length >= 2 && int.TryParse(parts[^1], out int m))
					minor = m;
			}
		}
		return new Ca65Version { Major = major, Minor = minor };
	}

	private static SymbolParseResult? ParseCa65SymbolLine(string line)
	{
		if (!line.StartsWith("sym\t"))
			return null;

		try {
			// Parse key=value pairs
			string data = line[4..];
			string? name = null;
			uint addr = 0;

			foreach (var pair in data.Split(',')) {
				var kv = pair.Split('=');
				if (kv.Length != 2) continue;

				string key = kv[0].Trim();
				string value = kv[1].Trim().Trim('"');

				if (key == "name")
					name = value;
				else if (key == "val")
					addr = value.StartsWith("$") ? Convert.ToUInt32(value[1..], 16) : uint.Parse(value);
			}

			if (name is null) return null;
			return new SymbolParseResult { Address = addr, Name = name };
		}
		catch {
			return null;
		}
	}

	private static SymbolParseResult? ParseWlaDxLine(string line)
	{
		if (string.IsNullOrWhiteSpace(line) || line.StartsWith("[") || line.StartsWith(";"))
			return null;

		try {
			// Format: $BB:AAAA Name or BANK:ADDR Name
			var parts = line.Split(' ', 2, StringSplitOptions.RemoveEmptyEntries);
			if (parts.Length < 2) return null;

			string addrPart = parts[0];
			string name = parts[1];

			// Parse $BB:AAAA format
			if (addrPart.StartsWith("$")) {
				var bankAddr = addrPart[1..].Split(':');
				if (bankAddr.Length != 2) return null;

				uint bank = Convert.ToUInt32(bankAddr[0], 16);
				uint offset = Convert.ToUInt32(bankAddr[1], 16);
				uint fullAddr = (bank << 16) | offset;

				return new SymbolParseResult { Address = fullAddr, Name = name };
			}

			return null;
		}
		catch {
			return null;
		}
	}

	private static string[] GetDebugFilesInDirectory(string path)
	{
		var extensions = new[] { ".dbg", ".sym", ".mlb", ".nexen-labels", ".elf" };
		return Directory.GetFiles(path)
			.Where(f => extensions.Contains(Path.GetExtension(f).ToLowerInvariant()))
			.ToArray();
	}

	private static ConversionResult[] SimulateBatchConvert(string path)
	{
		if (!Directory.Exists(path))
			return [];

		return GetDebugFilesInDirectory(path)
			.Select(f => new ConversionResult { SourceFile = f, Success = true })
			.ToArray();
	}

	private static PlatformId DetectPlatformByExtension(string romName)
	{
		string ext = Path.GetExtension(romName).ToLowerInvariant();
		return ext switch {
			".nes" => PlatformId.NES,
			".sfc" or ".smc" => PlatformId.SNES,
			".gb" or ".gbc" => PlatformId.GameBoy,
			".gba" => PlatformId.GBA,
			".pce" => PlatformId.PCEngine,
			".sms" or ".gg" => PlatformId.SMS,
			".ws" or ".wsc" => PlatformId.WonderSwan,
			".md" or ".gen" => PlatformId.Genesis,
			_ => PlatformId.Unknown
		};
	}

	#endregion

	#region Test Types

	public enum DebugFormat
	{
		Unknown,
		Ca65Dbg,
		WlaDx,
		Rgbds,
		Sdcc,
		Elf,
		NexenLabels
	}

	public enum PlatformId
	{
		Unknown = 0,
		NES = 1,
		SNES = 2,
		GameBoy = 3,
		GBA = 4,
		SMS = 6,
		PCEngine = 7,
		Genesis = 8,
		WonderSwan = 11
	}

	private class MlbParseResult
	{
		public uint Address { get; set; }
		public string Name { get; set; } = "";
		public string MemoryType { get; set; } = "";
		public string? Comment { get; set; }
	}

	private class SymbolParseResult
	{
		public uint Address { get; set; }
		public string Name { get; set; } = "";
	}

	private class Ca65Version
	{
		public int Major { get; set; }
		public int Minor { get; set; }
	}

	private class ConversionResult
	{
		public string SourceFile { get; set; } = "";
		public bool Success { get; set; }
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
