using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class AudioPlayerConfig : BaseConfig<AudioPlayerConfig> {
	[Reactive] public partial UInt32 Volume { get; set; } = 100;
	[Reactive] public partial bool Repeat { get; set; } = false;
	[Reactive] public partial bool Shuffle { get; set; } = false;

	public void ApplyConfig() {
		ConfigApi.SetAudioPlayerConfig(new InteropAudioPlayerConfig() {
			Volume = Volume,
			Repeat = Repeat,
			Shuffle = Shuffle,
		});
	}
}

[StructLayout(LayoutKind.Sequential)]
public struct InteropAudioPlayerConfig {
	public UInt32 Volume;
	[MarshalAs(UnmanagedType.I1)] public bool Repeat;
	[MarshalAs(UnmanagedType.I1)] public bool Shuffle;
}
