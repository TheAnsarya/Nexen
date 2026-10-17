using System.Reflection;
using Nexen.ViewModels;
using Nexen.Windows;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Regression tests for Channel F TAS gesture widget lanes.
/// Validates gesture-capable button definitions and label-to-button mapping.
/// </summary>
public sealed class ChannelFGestureWidgetTests {
	private static List<ControllerButtonInfo> InvokeChannelFButtons() {
		MethodInfo? method = typeof(TasEditorViewModel).GetMethod(
			"GetChannelFButtons", BindingFlags.Static | BindingFlags.NonPublic);
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
	public void GetChannelFButtons_ReturnsExpectedGestureSet() {
		var buttons = InvokeChannelFButtons();
		Assert.Equal(8, buttons.Count);

		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Contains("RIGHT", ids);
		Assert.Contains("LEFT", ids);
		Assert.Contains("BACK", ids);
		Assert.Contains("FORWARD", ids);
		Assert.Contains("TWISTCCW", ids);
		Assert.Contains("TWISTCW", ids);
		Assert.Contains("PULL", ids);
		Assert.Contains("PUSH", ids);

		Assert.Contains(buttons, b => b.ButtonId == "RIGHT" && b.Label == "Right");
		Assert.Contains(buttons, b => b.ButtonId == "LEFT" && b.Label == "Left");
		Assert.Contains(buttons, b => b.ButtonId == "BACK" && b.Label == "Back");
		Assert.Contains(buttons, b => b.ButtonId == "FORWARD" && b.Label == "Fwd");
		Assert.Contains(buttons, b => b.ButtonId == "TWISTCCW" && b.Label == "TwL");
		Assert.Contains(buttons, b => b.ButtonId == "TWISTCW" && b.Label == "TwR");
		Assert.Contains(buttons, b => b.ButtonId == "PULL" && b.Label == "Pull");
		Assert.Contains(buttons, b => b.ButtonId == "PUSH" && b.Label == "Push");
	}

	[Fact]
	public void GetChannelFButtons_HasUniqueButtonIds() {
		var buttons = InvokeChannelFButtons();
		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Equal(ids.Count, ids.Distinct().Count());
	}

	[Theory]
	[InlineData("↺", "TWISTCCW")]
	[InlineData("↻", "TWISTCW")]
	[InlineData("TwL", "TWISTCCW")]
	[InlineData("TwR", "TWISTCW")]
	[InlineData("PL", "PULL")]
	[InlineData("Pull", "PULL")]
	[InlineData("PS", "PUSH")]
	[InlineData("Push", "PUSH")]
	[InlineData("BK", "BACK")]
	[InlineData("Back", "BACK")]
	[InlineData("FW", "FORWARD")]
	[InlineData("Fwd", "FORWARD")]
	[InlineData("Right", "RIGHT")]
	[InlineData("Left", "LEFT")]
	public void MapButtonLabelToName_ChannelFGesturesResolve(string label, string expected) {
		Assert.Equal(expected, InvokeMapButtonLabelToName(label));
	}
}
