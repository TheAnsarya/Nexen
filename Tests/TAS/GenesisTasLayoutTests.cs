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

	[Theory]
	[InlineData("MOD", "MODE")]
	[InlineData("mode", "MODE")]
	[InlineData("MODE", "MODE")]
	public void MapButtonLabelToName_GenesisModeAliasesResolve(string label, string expected) {
		Assert.Equal(expected, InvokeMapButtonLabelToName(label));
	}
}
