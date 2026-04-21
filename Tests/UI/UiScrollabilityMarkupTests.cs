using System.IO;
using Xunit;

namespace Nexen.Tests.UI;

/// <summary>
/// Guards against accidental removal of explicit scroll containers from dense UI surfaces.
/// These are lightweight markup tests until broader UI automation coverage lands.
/// </summary>
public sealed class UiScrollabilityMarkupTests {
	[Theory]
	[InlineData("UI/Debugger/Windows/DebuggerConfigWindow.axaml")]
	[InlineData("UI/Windows/SetupWizardWindow.axaml")]
	public void DenseUiSurface_ContainsScrollViewer(string relativePath) {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, relativePath.Replace('/', Path.DirectorySeparatorChar));

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("<ScrollViewer", markup);
	}

	[Fact]
	public void SetupWizardWindow_IsResizableAndHasMinimumSize() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "SetupWizardWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"460\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
	}

	[Fact]
	public void ConfigWindow_IsResizableAndUsesLargerTouchTargets() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"700\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
		Assert.Contains("MinHeight=\"36\"", markup);
	}

	[Fact]
	public void DebuggerConfigWindow_IsResizableAndUsesLargerTouchTargets() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"700\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
		Assert.Contains("MinHeight=\"36\"", markup);
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
