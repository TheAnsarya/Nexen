using System.Reflection;
using Nexen.Config;
using Xunit;
using System.Linq;

namespace Nexen.Tests.Debugger.ViewModels;

public class GenesisEventViewerConfigTests {
	[Fact]
	public void Config_Has11EventCategories() {
		var config = new GenesisEventViewerConfig();
		var categoryProps = config.GetType()
			.GetProperties(BindingFlags.Public | BindingFlags.Instance)
			.Where(p => p.PropertyType == typeof(EventViewerCategoryCfg))
			.ToList();

		Assert.Equal(11, categoryProps.Count);
	}

	[Fact]
	public void Config_HasShowPreviousFrameEventsProperty() {
		var config = new GenesisEventViewerConfig();
		Assert.True(config.ShowPreviousFrameEvents);
	}

	[Fact]
	public void ToInterop_PreservesAllCategories() {
		var config = new GenesisEventViewerConfig();
		config.Irq.Visible = false;
		config.VdpControlWrite.Visible = false;
		config.ShowPreviousFrameEvents = false;

		var interop = config.ToInterop();

		Assert.False(interop.Irq.Visible);
		Assert.False(interop.VdpControlWrite.Visible);
		Assert.True(interop.MarkedBreakpoints.Visible);
		Assert.True(interop.IoRead.Visible);
		Assert.False(interop.ShowPreviousFrameEvents);
	}

	[Fact]
	public void EventViewerConfig_HasGenesisConfigProperty() {
		var config = new EventViewerConfig();
		Assert.NotNull(config.GenesisConfig);
		Assert.IsType<GenesisEventViewerConfig>(config.GenesisConfig);
	}
}
