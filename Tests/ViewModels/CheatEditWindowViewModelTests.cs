using System.IO;
using Xunit;

namespace Nexen.Tests.ViewModels;

public sealed class CheatEditWindowViewModelTests {
	[Fact]
	public void SourceContainsEmptyCompatibleTypeGuardBeforeTypeSelection() {
		string source = File.ReadAllText(Path.Combine("..", "..", "..", "..", "UI", "ViewModels", "CheatEditWindowViewModel.cs"));
		Assert.Contains("if (compatibleCheatTypes.Length == 0)", source);
		Assert.Contains("OkButtonEnabled = false;", source);
	}

	[Fact]
	public void SourceSelectsTypeFromCompatibleList() {
		string source = File.ReadAllText(Path.Combine("..", "..", "..", "..", "UI", "ViewModels", "CheatEditWindowViewModel.cs"));
		Assert.Contains("if (!compatibleCheatTypes.Contains(Cheat.Type))", source);
		Assert.Contains("Cheat.Type = (CheatType)compatibleCheatTypes[0];", source);
	}
}
