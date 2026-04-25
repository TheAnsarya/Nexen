using System.IO;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Source-guard tests for TAS input override mapping.
/// Prevents reintroducing incorrect Genesis C/Z remap into generic U/D fields.
/// </summary>
public sealed class TasInputOverrideMappingParityTests {
	private static string GetRepoRoot() {
		string current = AppContext.BaseDirectory;
		for (int i = 0; i < 10; i++) {
			string candidate = Path.GetFullPath(Path.Combine(current, "..", "..", "..", ".."));
			if (Directory.Exists(Path.Combine(candidate, "UI")) && File.Exists(Path.Combine(candidate, "Nexen.sln"))) {
				return candidate;
			}
			current = candidate;
		}

		throw new DirectoryNotFoundException("Could not locate Nexen repository root.");
	}

	[Fact]
	public void FeedControllerInput_DoesNotMapGenesisCToGenericU() {
		string root = GetRepoRoot();
		string path = Path.Combine(root, "UI", "Windows", "TasEditorWindow.axaml.cs");
		string source = File.ReadAllText(path);

		Assert.DoesNotContain("U = input.C", source, StringComparison.Ordinal);
	}

	[Fact]
	public void FeedControllerInput_DoesNotMapGenesisZToGenericD() {
		string root = GetRepoRoot();
		string path = Path.Combine(root, "UI", "Windows", "TasEditorWindow.axaml.cs");
		string source = File.ReadAllText(path);

		Assert.DoesNotContain("D = input.Z", source, StringComparison.Ordinal);
	}
}
