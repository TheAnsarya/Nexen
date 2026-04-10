using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Media.TextFormatting;
using Avalonia.Platform;
using Avalonia.Rendering.SceneGraph;
using Avalonia.Skia;
using Nexen.Utilities;
using SkiaSharp;

namespace Nexen.Controls;
public sealed class SimpleImageViewer : Control {
	public static readonly StyledProperty<DynamicBitmap> SourceProperty = AvaloniaProperty.Register<SimpleImageViewer, DynamicBitmap>(nameof(Source));
	public static readonly StyledProperty<bool> UseBilinearInterpolationProperty = AvaloniaProperty.Register<SimpleImageViewer, bool>(nameof(UseBilinearInterpolation));

	public DynamicBitmap Source {
		get { return GetValue(SourceProperty); }
		set { SetValue(SourceProperty, value); }
	}

	public bool UseBilinearInterpolation {
		get { return GetValue(UseBilinearInterpolationProperty); }
		set { SetValue(UseBilinearInterpolationProperty, value); }
	}

	static SimpleImageViewer() {
		AffectsRender<SimpleImageViewer>(SourceProperty, UseBilinearInterpolationProperty);
	}

	public SimpleImageViewer() {
	}

	public override void Render(DrawingContext context) {
		if (Source is null) {
			return;
		}

		context.Custom(new DrawOperation(this));
	}

	class DrawOperation : ICustomDrawOperation {
		public Rect Bounds { get; private set; }

		private DynamicBitmap _source;
		private BitmapInterpolationMode _interpolationMode;

		public DrawOperation(SimpleImageViewer viewer) {
			Bounds = viewer.Bounds;
			_interpolationMode = viewer.UseBilinearInterpolation ? BitmapInterpolationMode.HighQuality : BitmapInterpolationMode.None;
			_source = (DynamicBitmap)viewer.Source;
		}

		public void Dispose() {
		}

		public bool Equals(ICustomDrawOperation? other) => false;
		public bool HitTest(Point p) => true;

		public void Render(ImmediateDrawingContext context) {
			var leaseFeature = context.PlatformImpl.GetFeature<ISkiaSharpApiLeaseFeature>();
			if (leaseFeature is not null) {
				using var lease = leaseFeature.Lease();
				var canvas = lease.SkCanvas;

				// Single lock covers both InstallPixels and DrawBitmap — eliminates
				// the double-lock pattern (constructor + Render) that could cause
				// contention with UpdateSurface on the emulation thread
				using var bitmapLock = _source.Lock(true);
				var fb = bitmapLock.FrameBuffer;
				var info = new SKImageInfo(
					fb.Size.Width,
					fb.Size.Height,
					fb.Format.ToSkColorType(),
					SKAlphaType.Premul
				);

				using var bitmap = new SKBitmap();
				bitmap.InstallPixels(info, fb.Address);

				using SKPaint paint = new();
				paint.Color = new SKColor(255, 255, 255, 255);
#pragma warning disable CS0618 // SKPaint.FilterQuality is obsolete
				paint.FilterQuality = _interpolationMode switch {
					BitmapInterpolationMode.None => SKFilterQuality.None,
					BitmapInterpolationMode.LowQuality => SKFilterQuality.Low,
					BitmapInterpolationMode.MediumQuality => SKFilterQuality.Medium,
					BitmapInterpolationMode.HighQuality => SKFilterQuality.High,
					_ => SKFilterQuality.Low,
				};
#pragma warning restore CS0618

				canvas.DrawBitmap(bitmap,
					new SKRect(0, 0, info.Width, info.Height),
					new SKRect(0, 0, (float)Bounds.Width, (float)Bounds.Height),
					paint
				);
			}
		}
	}
}
