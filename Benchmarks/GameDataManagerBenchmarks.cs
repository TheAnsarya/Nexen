using System.Text;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Order;
using Nexen.Utilities;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for GameDataManager path building and sanitization operations.
/// These run during ROM loading and save/load operations — not hot-path per-frame,
/// but important for perceived responsiveness.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
public sealed class GameDataManagerBenchmarks
{
	private string[] _romNames = null!;
	private string[] _pathSegments = null!;

	[GlobalSetup]
	public void Setup()
	{
		_romNames = [
			"Super Mario World (USA).sfc",
			"The Legend of Zelda - A Link to the Past (USA) (Rev 1).smc",
			"Metroid - Zero Mission (USA, Europe).gba",
			"Game<>With:Bad*Chars?.nes",
			"日本語ゲーム.gb",
			"",
			"   ",
			"A",
			new string('X', 260),
			"Test.Game.v1.2.3.sfc"
		];

		_pathSegments = [
			"C:\\Users\\Test\\Documents\\Nexen\\Saves",
			"/home/user/.config/nexen/saves",
			"Saves\\SNES\\Super Mario World",
			""
		];
	}

	[Benchmark]
	[BenchmarkCategory("Sanitize")]
	public string[] SanitizeRomName_AllVariants()
	{
		var results = new string[_romNames.Length];
		for (int i = 0; i < _romNames.Length; i++) {
			results[i] = GameDataManager.SanitizeRomName(_romNames[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("Sanitize")]
	public string SanitizeRomName_Typical()
	{
		return GameDataManager.SanitizeRomName("Super Mario World (USA).sfc");
	}

	[Benchmark]
	[BenchmarkCategory("Sanitize")]
	public string SanitizeRomName_Long()
	{
		return GameDataManager.SanitizeRomName(new string('X', 260) + ".sfc");
	}

	[Benchmark]
	[BenchmarkCategory("FolderName")]
	public string[] GetSystemFolderName_AllSystems()
	{
		var systems = Enum.GetValues<Nexen.Interop.ConsoleType>();
		var results = new string[systems.Length];
		for (int i = 0; i < systems.Length; i++) {
			results[i] = GameDataManager.GetSystemFolderName(systems[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("PathBuild")]
	public string FormatGameFolderName_Typical()
	{
		return GameDataManager.FormatGameFolderName("Super Mario World (USA)", 0xABCD1234);
	}

	[Benchmark]
	[BenchmarkCategory("PathBuild")]
	public string BuildPath_Typical()
	{
		return GameDataManager.BuildPath(
			"C:\\Users\\Test\\Documents\\Nexen\\Saves",
			Nexen.Interop.ConsoleType.Snes,
			"Super Mario World",
			0xABCD1234);
	}
}
