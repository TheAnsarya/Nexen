using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Platform;
using Nexen.Config;
using Nexen.Interop;
using Nexen.ViewModels;

namespace Nexen; 
public sealed class NativeRenderer : NativeControlHost {
	public NativeRenderer() {
		Focusable = true;
	}

	public IntPtr Handle { get; private set; }

	protected override IPlatformHandle CreateNativeControlCore(IPlatformHandle parent) {
		var handle = base.CreateNativeControlCore(parent);
		Handle = handle.Handle;
		return handle;
	}
}
