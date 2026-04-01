using System.Collections.Generic;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Menus;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

/// <summary>
/// Regression tests for UI config clone semantics and menu enable/command behavior.
/// </summary>
public class UiConfigAndMenuRegressionTests {
	[Fact]
	public void Atari2600Config_Clone_IsDeepCopy() {
		var original = new Atari2600Config {
			P0DifficultyB = false,
			ColorMode = false,
			Channel0Vol = 55,
			HidePlayer0 = true
		};

		Atari2600Config clone = original.Clone();

		Assert.NotSame(original, clone);
		Assert.False(clone.P0DifficultyB);
		Assert.False(clone.ColorMode);
		Assert.Equal((uint)55, clone.Channel0Vol);
		Assert.True(clone.HidePlayer0);

		clone.P0DifficultyB = true;
		clone.ColorMode = true;
		clone.Channel0Vol = 90;
		clone.HidePlayer0 = false;

		Assert.False(original.P0DifficultyB);
		Assert.False(original.ColorMode);
		Assert.Equal((uint)55, original.Channel0Vol);
		Assert.True(original.HidePlayer0);
	}

	[Fact]
	public void LynxConfig_Clone_IsDeepCopy() {
		var original = new LynxConfig {
			UseBootRom = true,
			AutoRotate = false,
			Channel4Vol = 42
		};

		LynxConfig clone = original.Clone();

		Assert.NotSame(original, clone);
		Assert.True(clone.UseBootRom);
		Assert.False(clone.AutoRotate);
		Assert.Equal((uint)42, clone.Channel4Vol);

		clone.UseBootRom = false;
		clone.AutoRotate = true;
		clone.Channel4Vol = 99;

		Assert.True(original.UseBootRom);
		Assert.False(original.AutoRotate);
		Assert.Equal((uint)42, original.Channel4Vol);
	}

	[Fact]
	public void LynxConfig_IsIdentical_ReflectsMutations() {
		var a = new LynxConfig();
		var b = a.Clone();

		Assert.True(a.IsIdentical(b));

		b.BlendFrames = !b.BlendFrames;
		Assert.False(a.IsIdentical(b));
	}

	[Fact]
	public void Atari2600Config_IsIdentical_ReflectsMutations() {
		var a = new Atari2600Config();
		var b = a.Clone();

		Assert.True(a.IsIdentical(b));

		b.HideBall = !b.HideBall;
		Assert.False(a.IsIdentical(b));
	}

	[Fact]
	public void SimpleMenuAction_ClickCommand_RespectsIsEnabledPredicate() {
		bool executed = false;
		var action = new SimpleMenuAction {
			ActionType = ActionType.Custom,
			CustomText = "Test",
			IsEnabled = () => false
		};
		action.OnClick = () => executed = true;
		action.Initialize();

		action.ClickCommand!.Execute(null);
		Assert.False(executed);

		action.IsEnabled = () => true;
		action.Update();
		action.ClickCommand!.Execute(null);
		Assert.True(executed);
	}

	[Fact]
	public void SimpleMenuAction_EnabledState_RequiresAnyEnabledSubAction() {
		var childA = new SimpleMenuAction {
			ActionType = ActionType.Custom,
			CustomText = "A",
			IsEnabled = () => false
		};
		var childB = new SimpleMenuAction {
			ActionType = ActionType.Custom,
			CustomText = "B",
			IsEnabled = () => false
		};
		childA.Initialize();
		childB.Initialize();

		var parent = new SimpleMenuAction {
			ActionType = ActionType.Custom,
			CustomText = "Parent",
			SubActions = new List<object> { childA, childB }
		};
		parent.Initialize();
		Assert.False(parent.Enabled);

		childB.IsEnabled = () => true;
		childB.Update();
		parent.Update();
		Assert.True(parent.Enabled);
	}
}
