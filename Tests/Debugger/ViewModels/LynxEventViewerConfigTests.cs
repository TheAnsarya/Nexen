using System.Reflection;
using Nexen.Config;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

/// <summary>
/// Tests for the Lynx event viewer configuration.
/// Verifies all 11 event categories are present, have correct default colors,
/// and the ToInterop() conversion works correctly.
/// </summary>
public class LynxEventViewerConfigTests {
	#region Property Count

	[Fact]
	public void Config_Has11EventCategories() {
		var config = new LynxEventViewerConfig();
		var categoryProps = config.GetType()
			.GetProperties(BindingFlags.Public | BindingFlags.Instance)
			.Where(p => p.PropertyType == typeof(EventViewerCategoryCfg))
			.ToList();

		Assert.Equal(11, categoryProps.Count);
	}

	[Fact]
	public void Config_HasShowPreviousFrameEventsProperty() {
		var config = new LynxEventViewerConfig();
		Assert.True(config.ShowPreviousFrameEvents);
	}

	#endregion

	#region Default Values

	[Fact]
	public void Config_AllCategoriesVisibleByDefault() {
		var config = new LynxEventViewerConfig();
		Assert.True(config.Irq.Visible);
		Assert.True(config.MarkedBreakpoints.Visible);
		Assert.True(config.MikeyRegisterWrite.Visible);
		Assert.True(config.MikeyRegisterRead.Visible);
		Assert.True(config.SuzyRegisterWrite.Visible);
		Assert.True(config.SuzyRegisterRead.Visible);
		Assert.True(config.AudioRegisterWrite.Visible);
		Assert.True(config.AudioRegisterRead.Visible);
		Assert.True(config.PaletteWrite.Visible);
		Assert.True(config.TimerWrite.Visible);
		Assert.True(config.TimerRead.Visible);
	}

	[Fact]
	public void Config_AllCategoriesHaveDistinctColors() {
		var config = new LynxEventViewerConfig();
		var colors = new HashSet<uint> {
			config.Irq.Color,
			config.MarkedBreakpoints.Color,
			config.MikeyRegisterWrite.Color,
			config.MikeyRegisterRead.Color,
			config.SuzyRegisterWrite.Color,
			config.SuzyRegisterRead.Color,
			config.AudioRegisterWrite.Color,
			config.AudioRegisterRead.Color,
			config.PaletteWrite.Color,
			config.TimerWrite.Color,
			config.TimerRead.Color,
		};

		Assert.Equal(11, colors.Count);
	}

	#endregion

	#region ToInterop

	[Fact]
	public void ToInterop_PreservesAllCategories() {
		var config = new LynxEventViewerConfig();
		config.Irq.Visible = false;
		config.MikeyRegisterWrite.Visible = false;
		config.ShowPreviousFrameEvents = false;

		var interop = config.ToInterop();

		Assert.False(interop.Irq.Visible);
		Assert.False(interop.MikeyRegisterWrite.Visible);
		Assert.True(interop.MarkedBreakpoints.Visible);
		Assert.True(interop.SuzyRegisterWrite.Visible);
		Assert.False(interop.ShowPreviousFrameEvents);
	}

	[Fact]
	public void ToInterop_PreservesColors() {
		var config = new LynxEventViewerConfig();
		var interop = config.ToInterop();

		Assert.Equal(config.Irq.Color, interop.Irq.Color);
		Assert.Equal(config.PaletteWrite.Color, interop.PaletteWrite.Color);
		Assert.Equal(config.TimerRead.Color, interop.TimerRead.Color);
	}

	#endregion

	#region Enable/Disable All (Reflection Pattern)

	[Fact]
	public void EnableDisableAll_ViaReflection_WorksForAllCategories() {
		var config = new LynxEventViewerConfig();

		// Disable all
		foreach (var prop in config.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)) {
			if (prop.PropertyType == typeof(EventViewerCategoryCfg)) {
				((EventViewerCategoryCfg)prop.GetValue(config)!).Visible = false;
			}
		}

		Assert.False(config.Irq.Visible);
		Assert.False(config.PaletteWrite.Visible);
		Assert.False(config.TimerRead.Visible);

		// Enable all
		foreach (var prop in config.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)) {
			if (prop.PropertyType == typeof(EventViewerCategoryCfg)) {
				((EventViewerCategoryCfg)prop.GetValue(config)!).Visible = true;
			}
		}

		Assert.True(config.Irq.Visible);
		Assert.True(config.PaletteWrite.Visible);
		Assert.True(config.TimerRead.Visible);
	}

	#endregion

	#region EventViewerConfig Integration

	[Fact]
	public void EventViewerConfig_HasLynxConfigProperty() {
		var config = new EventViewerConfig();
		Assert.NotNull(config.LynxConfig);
		Assert.IsType<LynxEventViewerConfig>(config.LynxConfig);
	}

	#endregion
}
