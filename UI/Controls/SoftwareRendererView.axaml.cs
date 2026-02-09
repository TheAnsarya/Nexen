using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Controls;
public class SoftwareRendererView : UserControl {
	private SimpleImageViewer _frame;
	private SimpleImageViewer _emuHud;
	private SimpleImageViewer _scriptHud;
	private SoftwareRendererViewModel _model = new();

	public SoftwareRendererView() {
		InitializeComponent();

		_frame = this.GetControl<SimpleImageViewer>("Frame");
		_emuHud = this.GetControl<SimpleImageViewer>("EmuHud");
		_scriptHud = this.GetControl<SimpleImageViewer>("ScriptHud");
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnDataContextChanged(EventArgs e) {
		base.OnDataContextChanged(e);
		if (DataContext is SoftwareRendererViewModel model) {
			_model = model;
		}
	}

	private unsafe void UpdateSurface(SoftwareRendererSurface frame, DynamicBitmap? surface, Action<DynamicBitmap> updateSurfaceRef) {
		PixelSize frameSize = new PixelSize((int)frame.Width, (int)frame.Height);
		if (surface?.PixelSize != frameSize) {
			surface?.Dispose();
			surface = new DynamicBitmap(frameSize, new Vector(96, 96), PixelFormat.Bgra8888, AlphaFormat.Premul);
			updateSurfaceRef(surface);
		}

		int size = (int)frame.Width * (int)frame.Height * sizeof(UInt32);
		using (var bitmapLock = surface.Lock()) {
			var srcSpan = new Span<byte>((byte*)frame.FrameBuffer, size);
			var dstSpan = new Span<byte>((byte*)bitmapLock.FrameBuffer.Address, size);
			srcSpan.CopyTo(dstSpan);
		}
	}

	public unsafe void UpdateSoftwareRenderer(SoftwareRendererFrame frameInfo) {
		UpdateSurface(frameInfo.Frame, _model.FrameSurface, s => _model.FrameSurface = s);

		bool emuHudDirty = frameInfo.EmuHud.IsDirty;
		bool scriptHudDirty = frameInfo.ScriptHud.IsDirty;

		if (emuHudDirty) {
			UpdateSurface(frameInfo.EmuHud, _model.EmuHudSurface, s => _model.EmuHudSurface = s);
		}

		if (scriptHudDirty) {
			UpdateSurface(frameInfo.ScriptHud, _model.ScriptHudSurface, s => _model.ScriptHudSurface = s);
		}

		Dispatcher.UIThread.Post(() => {
			_frame.UseBilinearInterpolation = ConfigManager.Config.Video.UseBilinearInterpolation;
			_frame.InvalidateVisual();

			if (emuHudDirty) {
				_emuHud.InvalidateVisual();
			}

			if (scriptHudDirty) {
				_scriptHud.InvalidateVisual();
			}
		}, DispatcherPriority.MaxValue);
	}
}

public class SoftwareRendererViewModel : ViewModelBase {
	[Reactive] public DynamicBitmap? FrameSurface { get; set; }
	[Reactive] public DynamicBitmap? EmuHudSurface { get; set; }
	[Reactive] public DynamicBitmap? ScriptHudSurface { get; set; }
	[Reactive] public double Width { get; set; }
	[Reactive] public double Height { get; set; }
}
