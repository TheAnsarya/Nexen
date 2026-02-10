using Xunit;
using System;
using System.IO;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Tests.Utilities;

/// <summary>
/// Unit tests for <see cref="GameDataManager"/> path resolution,
/// sanitization, and folder creation logic.
/// </summary>
public class GameDataManagerTests
{
	#region GetSystemFolderName Tests

	[Theory]
	[InlineData(ConsoleType.Snes, "SNES")]
	[InlineData(ConsoleType.Nes, "NES")]
	[InlineData(ConsoleType.Gameboy, "GB")]
	[InlineData(ConsoleType.Gba, "GBA")]
	[InlineData(ConsoleType.PcEngine, "PCE")]
	[InlineData(ConsoleType.Sms, "SMS")]
	[InlineData(ConsoleType.Ws, "WS")]
	public void GetSystemFolderName_ReturnsCorrectAbbreviation(ConsoleType system, string expected)
	{
		string result = GameDataManager.GetSystemFolderName(system);
		Assert.Equal(expected, result);
	}

	[Fact]
	public void GetSystemFolderName_AllEnumValues_ReturnNonEmpty()
	{
		foreach (ConsoleType system in Enum.GetValues<ConsoleType>()) {
			string result = GameDataManager.GetSystemFolderName(system);
			Assert.False(string.IsNullOrWhiteSpace(result), $"System {system} returned empty folder name");
		}
	}

	#endregion

	#region SanitizeRomName Tests

	[Theory]
	[InlineData("Super Mario World", "Super Mario World")]
	[InlineData("Game.sfc", "Game")]
	[InlineData("My Game (USA).nes", "My Game (USA)")]
	[InlineData("Test.Game.v1.gba", "Test.Game.v1")]
	public void SanitizeRomName_BasicNames_ReturnsCleanName(string input, string expected)
	{
		string result = GameDataManager.SanitizeRomName(input);
		Assert.Equal(expected, result);
	}

	[Theory]
	[InlineData(null, "Unknown")]
	[InlineData("", "Unknown")]
	[InlineData("   ", "Unknown")]
	public void SanitizeRomName_EmptyOrNull_ReturnsUnknown(string? input, string expected)
	{
		string result = GameDataManager.SanitizeRomName(input!);
		Assert.Equal(expected, result);
	}

	[Fact]
	public void SanitizeRomName_InvalidChars_ReplacedWithUnderscore()
	{
		// Characters like : * ? " < > | are invalid on Windows
		string result = GameDataManager.SanitizeRomName("Game:Name*Bad?Chars");
		Assert.DoesNotContain(":", result);
		Assert.DoesNotContain("*", result);
		Assert.DoesNotContain("?", result);
	}

	[Fact]
	public void SanitizeRomName_MultipleInvalidChars_CollapsedToSingleUnderscore()
	{
		string result = GameDataManager.SanitizeRomName("Game:::Name");
		Assert.DoesNotContain("___", result);
	}

	[Fact]
	public void SanitizeRomName_LongName_TruncatedToMaxLength()
	{
		string longName = new string('A', 200) + ".sfc";
		string result = GameDataManager.SanitizeRomName(longName);
		Assert.True(result.Length <= GameDataManager.MaxRomNameLength,
			$"Name length {result.Length} exceeds max {GameDataManager.MaxRomNameLength}");
	}

	[Fact]
	public void SanitizeRomName_LeadingTrailingUnderscores_Trimmed()
	{
		// Create input that would produce leading/trailing underscores after sanitization
		string result = GameDataManager.SanitizeRomName("___Game___");
		Assert.False(result.StartsWith('_'), "Should not start with underscore");
		Assert.False(result.EndsWith('_'), "Should not end with underscore");
		Assert.Equal("Game", result);
	}

	#endregion

	#region FormatGameFolderName Tests

	[Theory]
	[InlineData("SuperMario", 0xDEADBEEFu, "SuperMario_deadbeef")]
	[InlineData("Zelda", 0x12345678u, "Zelda_12345678")]
	[InlineData("Game", 0x00000001u, "Game_00000001")]
	[InlineData("Test", 0xFFFFFFFFu, "Test_ffffffff")]
	[InlineData("Rom", 0x00000000u, "Rom_00000000")]
	public void FormatGameFolderName_ReturnsCorrectFormat(string name, uint crc32, string expected)
	{
		string result = GameDataManager.FormatGameFolderName(name, crc32);
		Assert.Equal(expected, result);
	}

	[Fact]
	public void FormatGameFolderName_CrcAlwaysLowercase8Hex()
	{
		string result = GameDataManager.FormatGameFolderName("Test", 0xABCDEF01);
		Assert.EndsWith("_abcdef01", result);
	}

	#endregion

	#region BuildPath Tests

	[Fact]
	public void BuildPath_ReturnsCorrectStructure()
	{
		string basePath = Path.Combine("C:", "Nexen", "GameData");
		string result = GameDataManager.BuildPath(basePath, ConsoleType.Snes, "Super Mario World.sfc", 0xA1B2C3D4);

		Assert.Contains("SNES", result);
		Assert.Contains("Super Mario World_a1b2c3d4", result);
		Assert.StartsWith(basePath, result);
	}

	[Fact]
	public void BuildPath_WithSubfolder_AppendsSubfolder()
	{
		string basePath = Path.Combine("C:", "Nexen", "GameData");
		string result = GameDataManager.BuildPath(basePath, ConsoleType.Nes, "Zelda.nes", 0x12345678, "SaveStates");

		Assert.EndsWith("SaveStates", result);
	}

	[Fact]
	public void BuildPath_WithoutSubfolder_ReturnsGameRoot()
	{
		string basePath = Path.Combine("C:", "Nexen", "GameData");
		string result = GameDataManager.BuildPath(basePath, ConsoleType.Gameboy, "Pokemon.gb", 0xAABBCCDD);

		Assert.Contains("GB", result);
		Assert.Contains("Pokemon_aabbccdd", result);
		Assert.DoesNotContain("SaveStates", result);
		Assert.DoesNotContain("Debug", result);
	}

	[Theory]
	[InlineData(ConsoleType.Snes, "SNES")]
	[InlineData(ConsoleType.Nes, "NES")]
	[InlineData(ConsoleType.Gameboy, "GB")]
	[InlineData(ConsoleType.Gba, "GBA")]
	[InlineData(ConsoleType.PcEngine, "PCE")]
	[InlineData(ConsoleType.Sms, "SMS")]
	[InlineData(ConsoleType.Ws, "WS")]
	public void BuildPath_SystemFolderInPath(ConsoleType system, string expectedFolder)
	{
		string basePath = Path.Combine("C:", "Data");
		string result = GameDataManager.BuildPath(basePath, system, "Test", 0x00000000);

		string separator = Path.DirectorySeparatorChar.ToString();
		Assert.Contains($"{separator}{expectedFolder}{separator}", result);
	}

	#endregion

	#region Folder Creation Tests

	[Fact]
	public void BuildPath_SanitizesRomNameInPath()
	{
		string basePath = Path.Combine("C:", "Data");
		string result = GameDataManager.BuildPath(basePath, ConsoleType.Snes, "Bad:Name*Here?.sfc", 0x11111111);

		// Should not contain printable invalid chars (skip control chars like \0)
		string folderPart = Path.GetFileName(result);
		foreach (char c in Path.GetInvalidFileNameChars()) {
			if (!char.IsControl(c)) {
				Assert.DoesNotContain(c.ToString(), folderPart);
			}
		}
	}

	[Theory]
	[InlineData("SaveStates")]
	[InlineData("Debug")]
	[InlineData("Saves")]
	public void BuildPath_SubfolderVariants_AppendCorrectly(string subfolder)
	{
		string basePath = Path.Combine("C:", "Data");
		string result = GameDataManager.BuildPath(basePath, ConsoleType.Nes, "Test", 0x12345678, subfolder);

		Assert.EndsWith(subfolder, result);
	}

	#endregion

	#region Integration Path Tests

	[Fact]
	public void BuildPath_FullPathStructure_MatchesExpected()
	{
		string basePath = Path.Combine("C:", "Users", "me", "Documents", "Nexen", "GameData");
		string result = GameDataManager.BuildPath(
			basePath,
			ConsoleType.Snes,
			"Super Mario World (USA).sfc",
			0xB19ED489,
			"SaveStates"
		);

		string expected = Path.Combine(
			basePath,
			"SNES",
			"Super Mario World (USA)_b19ed489",
			"SaveStates"
		);

		Assert.Equal(expected, result);
	}

	[Fact]
	public void BuildPath_DebugSubfolder_MatchesExpected()
	{
		string basePath = Path.Combine("C:", "Nexen", "GameData");
		string result = GameDataManager.BuildPath(
			basePath,
			ConsoleType.Gameboy,
			"Pokemon Red.gb",
			0xAABBCCDD,
			"Debug"
		);

		string expected = Path.Combine(
			basePath,
			"GB",
			"Pokemon Red_aabbccdd",
			"Debug"
		);

		Assert.Equal(expected, result);
	}

	[Fact]
	public void BuildPath_SavesSubfolder_MatchesExpected()
	{
		string basePath = Path.Combine("C:", "Nexen", "GameData");
		string result = GameDataManager.BuildPath(
			basePath,
			ConsoleType.Gba,
			"Metroid Fusion.gba",
			0x11223344,
			"Saves"
		);

		string expected = Path.Combine(
			basePath,
			"GBA",
			"Metroid Fusion_11223344",
			"Saves"
		);

		Assert.Equal(expected, result);
	}

	#endregion
}
