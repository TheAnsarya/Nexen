using System.Reflection;
using Nexen.ViewModels;
using Nexen.Windows;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Regression tests for Genesis TAS button layout parity.
/// Ensures 6-button controls include Mode and map label aliases correctly.
/// </summary>
public sealed class GenesisTasLayoutTests {
	private static List<ControllerButtonInfo> InvokeGenesisButtons() {
		MethodInfo? method = typeof(TasEditorViewModel).GetMethod(
			"GetGenesisButtons", BindingFlags.Static | BindingFlags.NonPublic);
		Assert.NotNull(method);

		var result = method!.Invoke(null, null) as List<ControllerButtonInfo>;
		Assert.NotNull(result);
		return result!;
	}

	private static string InvokeMapButtonLabelToName(string label) {
		MethodInfo? method = typeof(TasEditorWindow).GetMethod(
			"MapButtonLabelToName", BindingFlags.Static | BindingFlags.NonPublic);
		Assert.NotNull(method);

		var result = method!.Invoke(null, [label]) as string;
		Assert.NotNull(result);
		return result!;
	}

	[Fact]
	public void GetGenesisButtons_IncludesModeAndSixButtonSet() {
		var buttons = InvokeGenesisButtons();
		Assert.Equal(12, buttons.Count);

		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Contains("A", ids);
		Assert.Contains("B", ids);
		Assert.Contains("C", ids);
		Assert.Contains("X", ids);
		Assert.Contains("Y", ids);
		Assert.Contains("Z", ids);
		Assert.Contains("MODE", ids);
		Assert.Contains("START", ids);
		Assert.Contains("UP", ids);
		Assert.Contains("DOWN", ids);
		Assert.Contains("LEFT", ids);
		Assert.Contains("RIGHT", ids);

		Assert.Contains(buttons, b => b.ButtonId == "MODE" && b.Label == "MOD");
	}

	[Fact]
	public void GetGenesisButtons_HasUniqueButtonIds() {
		var buttons = InvokeGenesisButtons();
		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Equal(ids.Count, ids.Distinct().Count());
	}

	[Fact]
	public void GetGenesisButtons_MatchesExpectedLanePositions() {
		var buttons = InvokeGenesisButtons();

		var expected = new Dictionary<string, (int Column, int Row, string Label)> {
			["A"] = (0, 0, "A"),
			["B"] = (1, 0, "B"),
			["C"] = (2, 0, "C"),
			["MODE"] = (3, 0, "MOD"),
			["X"] = (0, 1, "X"),
			["Y"] = (1, 1, "Y"),
			["Z"] = (2, 1, "Z"),
			["START"] = (3, 1, "STA"),
			["UP"] = (0, 2, "↑"),
			["DOWN"] = (1, 2, "↓"),
			["LEFT"] = (2, 2, "←"),
			["RIGHT"] = (3, 2, "→")
		};

		Assert.Equal(expected.Count, buttons.Count);

		foreach (var button in buttons) {
			Assert.True(expected.TryGetValue(button.ButtonId, out var lane), $"Unexpected button id: {button.ButtonId}");
			Assert.Equal(lane.Column, button.Column);
			Assert.Equal(lane.Row, button.Row);
			Assert.Equal(lane.Label, button.Label);
		}
	}

	[Theory]
	[InlineData("MOD", "MODE")]
	[InlineData("mode", "MODE")]
	[InlineData("MODE", "MODE")]
	[InlineData("STA", "START")]
	[InlineData("sta", "START")]
	[InlineData("SEL", "SELECT")]
	[InlineData("sel", "SELECT")]
	public void MapButtonLabelToName_GenesisModeAliasesResolve(string label, string expected) {
		Assert.Equal(expected, InvokeMapButtonLabelToName(label));
	}
}
