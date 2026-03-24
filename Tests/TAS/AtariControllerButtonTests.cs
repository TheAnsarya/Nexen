using System.Reflection;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for Atari 2600 TAS piano roll button definitions.
/// Verifies all 5 controller types return correct button counts,
/// names, and grid positions.
/// </summary>
public class AtariControllerButtonTests {
	private static List<ControllerButtonInfo> InvokeButtonMethod(string methodName) {
		MethodInfo? method = typeof(TasEditorViewModel).GetMethod(
			methodName, BindingFlags.Static | BindingFlags.NonPublic);
		Assert.NotNull(method);
		var result = method!.Invoke(null, null) as List<ControllerButtonInfo>;
		Assert.NotNull(result);
		return result!;
	}

	[Fact]
	public void GetAtari2600JoystickButtons_Returns5Buttons() {
		var buttons = InvokeButtonMethod("GetAtari2600JoystickButtons");
		Assert.Equal(5, buttons.Count);
	}

	[Fact]
	public void GetAtari2600JoystickButtons_ContainsFireAndDirections() {
		var buttons = InvokeButtonMethod("GetAtari2600JoystickButtons");
		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Contains("FIRE", ids);
		Assert.Contains("UP", ids);
		Assert.Contains("DOWN", ids);
		Assert.Contains("LEFT", ids);
		Assert.Contains("RIGHT", ids);
	}

	[Fact]
	public void GetAtari2600PaddleButtons_Returns1Button() {
		var buttons = InvokeButtonMethod("GetAtari2600PaddleButtons");
		Assert.Single(buttons);
		Assert.Equal("FIRE", buttons[0].ButtonId);
	}

	[Fact]
	public void GetAtari2600KeypadButtons_Returns12Buttons() {
		var buttons = InvokeButtonMethod("GetAtari2600KeypadButtons");
		Assert.Equal(12, buttons.Count);
	}

	[Fact]
	public void GetAtari2600KeypadButtons_ContainsAll12Keys() {
		var buttons = InvokeButtonMethod("GetAtari2600KeypadButtons");
		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Contains("1", ids);
		Assert.Contains("2", ids);
		Assert.Contains("3", ids);
		Assert.Contains("4", ids);
		Assert.Contains("5", ids);
		Assert.Contains("6", ids);
		Assert.Contains("7", ids);
		Assert.Contains("8", ids);
		Assert.Contains("9", ids);
		Assert.Contains("0", ids);
		Assert.Contains("STAR", ids);
		Assert.Contains("POUND", ids);
	}

	[Fact]
	public void GetAtari2600DrivingButtons_Returns3Buttons() {
		var buttons = InvokeButtonMethod("GetAtari2600DrivingButtons");
		Assert.Equal(3, buttons.Count);
	}

	[Fact]
	public void GetAtari2600DrivingButtons_ContainsFireAndDirections() {
		var buttons = InvokeButtonMethod("GetAtari2600DrivingButtons");
		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Contains("FIRE", ids);
		Assert.Contains("LEFT", ids);
		Assert.Contains("RIGHT", ids);
	}

	[Fact]
	public void GetAtari2600BoosterGripButtons_Returns7Buttons() {
		var buttons = InvokeButtonMethod("GetAtari2600BoosterGripButtons");
		Assert.Equal(7, buttons.Count);
	}

	[Fact]
	public void GetAtari2600BoosterGripButtons_ContainsAllInputs() {
		var buttons = InvokeButtonMethod("GetAtari2600BoosterGripButtons");
		var ids = buttons.Select(b => b.ButtonId).ToList();
		Assert.Contains("FIRE", ids);
		Assert.Contains("TRIGGER", ids);
		Assert.Contains("BOOSTER", ids);
		Assert.Contains("UP", ids);
		Assert.Contains("DOWN", ids);
		Assert.Contains("LEFT", ids);
		Assert.Contains("RIGHT", ids);
	}

	[Fact]
	public void AllAtariButtonSets_HaveUniqueButtonIds() {
		var methodNames = new[] {
			"GetAtari2600JoystickButtons",
			"GetAtari2600PaddleButtons",
			"GetAtari2600KeypadButtons",
			"GetAtari2600DrivingButtons",
			"GetAtari2600BoosterGripButtons"
		};

		foreach (string name in methodNames) {
			var buttons = InvokeButtonMethod(name);
			var ids = buttons.Select(b => b.ButtonId).ToList();
			Assert.Equal(ids.Count, ids.Distinct().Count());
		}
	}

	[Fact]
	public void AllAtariButtonSets_HaveNonNegativeGridPositions() {
		var methodNames = new[] {
			"GetAtari2600JoystickButtons",
			"GetAtari2600PaddleButtons",
			"GetAtari2600KeypadButtons",
			"GetAtari2600DrivingButtons",
			"GetAtari2600BoosterGripButtons"
		};

		foreach (string name in methodNames) {
			var buttons = InvokeButtonMethod(name);
			Assert.All(buttons, b => {
				Assert.True(b.Column >= 0, $"{name}: {b.ButtonId} has negative Column");
				Assert.True(b.Row >= 0, $"{name}: {b.ButtonId} has negative Row");
			});
		}
	}

	[Theory]
	[InlineData("GetAtari2600JoystickButtons", "FIRE", "Fire")]
	[InlineData("GetAtari2600PaddleButtons", "FIRE", "Fire")]
	[InlineData("GetAtari2600KeypadButtons", "STAR", "*")]
	[InlineData("GetAtari2600KeypadButtons", "POUND", "#")]
	[InlineData("GetAtari2600DrivingButtons", "FIRE", "Fire")]
	[InlineData("GetAtari2600BoosterGripButtons", "TRIGGER", "Trg")]
	[InlineData("GetAtari2600BoosterGripButtons", "BOOSTER", "Bst")]
	public void AtariButtons_HaveCorrectLabels(string methodName, string buttonId, string expectedLabel) {
		var buttons = InvokeButtonMethod(methodName);
		var button = buttons.FirstOrDefault(b => b.ButtonId == buttonId);
		Assert.NotNull(button);
		Assert.Equal(expectedLabel, button!.Label);
	}
}
