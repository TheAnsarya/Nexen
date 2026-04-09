using System;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class NetplayConfig : BaseConfig<NetplayConfig> {
	[Reactive] public partial string Host { get; set; } = "localhost";
	[Reactive] public partial UInt16 Port { get; set; } = 8888;
	[Reactive] public partial string Password { get; set; } = "";

	[Reactive] public partial UInt16 ServerPort { get; set; } = 8888;
	[Reactive] public partial string ServerPassword { get; set; } = "";
}
