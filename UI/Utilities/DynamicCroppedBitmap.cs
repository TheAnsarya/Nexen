using System;
using Avalonia;
using Avalonia.Media;
using Avalonia.Media.Imaging;

namespace Nexen.Utilities; 
public sealed class DynamicCroppedBitmap : CroppedBitmap, IDynamicBitmap {
	public new event EventHandler? Invalidated;

	static DynamicCroppedBitmap() {
		SourceProperty.Changed.AddClassHandler<DynamicCroppedBitmap>((x, e) => {
			if (e.OldValue is IDynamicBitmap oldSource) {
				oldSource.Invalidated -= x.OnSourceInvalidated;
			}

			if (x.Source is IDynamicBitmap newSource) {
				newSource.Invalidated += x.OnSourceInvalidated;
			}

			x.Invalidate();
		});
	}

	public DynamicCroppedBitmap(IImage source, PixelRect sourceRect) : base(source, sourceRect) {
	}

	public void Invalidate() {
		Invalidated?.Invoke(this, EventArgs.Empty);
	}

	private void OnSourceInvalidated(object? sender, EventArgs e) {
		Invalidate();
	}
}
