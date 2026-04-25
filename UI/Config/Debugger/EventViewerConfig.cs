using System.Collections.Generic;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class EventViewerConfig : BaseWindowConfig<EventViewerConfig> {
	[Reactive] public partial bool ShowSettingsPanel { get; set; } = true;

	[Reactive] public partial double ImageScale { get; set; } = 1;

	[Reactive] public partial bool RefreshOnBreakPause { get; set; } = true;
	[Reactive] public partial bool AutoRefresh { get; set; } = true;
	[Reactive] public partial RefreshTimingConfig RefreshTiming { get; set; } = new();

	[Reactive] public partial List<int> ColumnWidths { get; set; } = new();

	[Reactive] public partial bool ShowToolbar { get; set; } = true;

	[Reactive] public partial bool ShowListView { get; set; } = true;
	[Reactive] public partial double ListViewHeight { get; set; } = 200;

	[Reactive] public partial SnesEventViewerConfig SnesConfig { get; set; } = new SnesEventViewerConfig();
	[Reactive] public partial NesEventViewerConfig NesConfig { get; set; } = new NesEventViewerConfig();
	[Reactive] public partial GbEventViewerConfig GbConfig { get; set; } = new GbEventViewerConfig();
	[Reactive] public partial GbaEventViewerConfig GbaConfig { get; set; } = new GbaEventViewerConfig();
	[Reactive] public partial PceEventViewerConfig PceConfig { get; set; } = new PceEventViewerConfig();
	[Reactive] public partial SmsEventViewerConfig SmsConfig { get; set; } = new SmsEventViewerConfig();
	[Reactive] public partial GenesisEventViewerConfig GenesisConfig { get; set; } = new GenesisEventViewerConfig();
	[Reactive] public partial WsEventViewerConfig WsConfig { get; set; } = new WsEventViewerConfig();
	[Reactive] public partial LynxEventViewerConfig LynxConfig { get; set; } = new LynxEventViewerConfig();
	[Reactive] public partial Atari2600EventViewerConfig Atari2600Config { get; set; } = new Atari2600EventViewerConfig();
	[Reactive] public partial ChannelFEventViewerConfig ChannelFConfig { get; set; } = new ChannelFEventViewerConfig();
}
