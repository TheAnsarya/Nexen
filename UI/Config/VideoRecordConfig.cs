using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class VideoRecordConfig : BaseConfig<VideoRecordConfig> {
	[Reactive] public partial VideoCodec Codec { get; set; } = VideoCodec.CSCD;
	[Reactive] public partial UInt32 CompressionLevel { get; set; } = 6;
	[Reactive] public partial bool RecordSystemHud { get; set; } = false;
	[Reactive] public partial bool RecordInputHud { get; set; } = false;
}

public enum VideoCodec {
	None = 0,
	ZMBV = 1,
	CSCD = 2,
	GIF = 3
}
