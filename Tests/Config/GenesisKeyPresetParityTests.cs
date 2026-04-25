using System.IO;
using Xunit;

namespace Nexen.Tests.Config;

public class GenesisKeyPresetParityTests {
	[Fact]
	public void KeyPresets_SourceContainsGenesisWasdBranchWithCBinding() {
		string source = ReadKeyPresetsSource();

		Assert.Contains("} else if (type == ControllerType.GenesisController) {", source);
		Assert.Contains("m.X = InputApi.GetKeyCode(\"H\");", source);
		Assert.Contains("m.TurboX = InputApi.GetKeyCode(\"N\");", source);
	}

	[Fact]
	public void KeyPresets_SourceContainsGenesisArrowBranchWithCBinding() {
		string source = ReadKeyPresetsSource();

		Assert.Contains("m.X = InputApi.GetKeyCode(\"D\");", source);
		Assert.Contains("m.TurboX = InputApi.GetKeyCode(\"C\");", source);
	}

	[Fact]
	public void KeyPresets_SourceIncludesGenesisInXboxBranch() {
		string source = ReadKeyPresetsSource();

		Assert.Contains("ControllerType.GenesisController", source);
		Assert.Contains("m.X = InputApi.GetKeyCode(prefix + \"Y\");", source);
	}

	[Fact]
	public void KeyPresets_SourceIncludesGenesisInPs4Branch() {
		string source = ReadKeyPresetsSource();

		Assert.Contains("m.X = InputApi.GetKeyCode(prefix + \"But4\");", source);
	}

	private static string ReadKeyPresetsSource() {
		string repoRoot = GetRepositoryRoot();
		string path = Path.Combine(repoRoot, "UI", "Config", "KeyPresets.cs");

		Assert.True(File.Exists(path), $"Expected source file to exist: {path}");
		return File.ReadAllText(path);
	}

	private static string GetRepositoryRoot() {
		string? current = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));

		while (!string.IsNullOrEmpty(current)) {
			if (File.Exists(Path.Combine(current, "Nexen.sln"))) {
				return current;
			}

			string? parent = Directory.GetParent(current)?.FullName;
			if (string.Equals(parent, current, System.StringComparison.OrdinalIgnoreCase)) {
				break;
			}
			current = parent;
		}

		throw new DirectoryNotFoundException("Could not locate Nexen repository root.");
	}
}
