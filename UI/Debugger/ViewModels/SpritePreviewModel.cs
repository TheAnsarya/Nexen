using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Platform;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// Model for displaying sprite preview information in the sprite viewer.
/// </summary>
/// <remarks>
/// <para>
/// Contains all properties needed to display a single sprite's information:
/// <list type="bullet">
///   <item><description>Position (X, Y) and size (Width, Height)</description></item>
///   <item><description>Tile and palette information</description></item>
///   <item><description>Transform flags (mirror, mosaic, rotation)</description></item>
///   <item><description>Bitmap preview of the sprite</description></item>
/// </list>
/// </para>
/// <para>
/// Supports sprite wrapping for consoles that wrap sprites around screen edges (SNES, GBA).
/// </para>
/// </remarks>
public sealed class SpritePreviewModel : ViewModelBase {
	/// <summary>Gets or sets the sprite's index in the OAM table.</summary>
	[Reactive] public int SpriteIndex { get; set; }

	/// <summary>Gets or sets the sprite's X position on screen.</summary>
	[Reactive] public int X { get; set; }

	/// <summary>Gets or sets the sprite's Y position on screen.</summary>
	[Reactive] public int Y { get; set; }

	/// <summary>Gets or sets the raw X value from OAM (before interpretation).</summary>
	[Reactive] public int RawX { get; set; }

	/// <summary>Gets or sets the raw Y value from OAM (before interpretation).</summary>
	[Reactive] public int RawY { get; set; }

	/// <summary>Gets or sets the X position for preview rendering (with coordinate offset).</summary>
	[Reactive] public int PreviewX { get; set; }

	/// <summary>Gets or sets the Y position for preview rendering (with coordinate offset).</summary>
	[Reactive] public int PreviewY { get; set; }

	/// <summary>Gets or sets the sprite width in pixels.</summary>
	[Reactive] public int Width { get; set; }

	/// <summary>Gets or sets the sprite height in pixels.</summary>
	[Reactive] public int Height { get; set; }

	/// <summary>Gets or sets the base tile index in VRAM.</summary>
	[Reactive] public int TileIndex { get; set; }

	/// <summary>Gets or sets the tile data address in VRAM.</summary>
	[Reactive] public int TileAddress { get; set; }

	/// <summary>Gets or sets the sprite's rendering priority.</summary>
	[Reactive] public DebugSpritePriority Priority { get; set; }

	/// <summary>Gets or sets the sprite rendering mode (normal, blending, window).</summary>
	[Reactive] public DebugSpriteMode Mode { get; set; }

	/// <summary>Gets or sets the bits per pixel for this sprite.</summary>
	[Reactive] public int Bpp { get; set; }

	/// <summary>Gets or sets the tile format.</summary>
	[Reactive] public TileFormat Format { get; set; }

	/// <summary>Gets or sets the palette index.</summary>
	[Reactive] public int Palette { get; set; }

	/// <summary>Gets or sets the palette data address.</summary>
	[Reactive] public int PaletteAddress { get; set; }

	/// <summary>Gets or sets the sprite's visibility state.</summary>
	[Reactive] public SpriteVisibility Visibility { get; set; }

	/// <summary>Gets or sets the flag summary string (H=mirror, V=flip, etc.).</summary>
	[Reactive] public string? Flags { get; set; }

	/// <summary>Gets or sets whether horizontal mirroring is enabled.</summary>
	[Reactive] public NullableBoolean HorizontalMirror { get; set; }

	/// <summary>Gets or sets whether vertical mirroring is enabled.</summary>
	[Reactive] public NullableBoolean VerticalMirror { get; set; }

	/// <summary>Gets or sets whether mosaic effect is enabled.</summary>
	[Reactive] public NullableBoolean MosaicEnabled { get; set; }

	/// <summary>Gets or sets whether affine transformation is enabled (GBA).</summary>
	[Reactive] public NullableBoolean TransformEnabled { get; set; }

	/// <summary>Gets or sets whether double-size mode is enabled for transforms (GBA).</summary>
	[Reactive] public NullableBoolean DoubleSize { get; set; }

	/// <summary>Gets or sets the affine transform parameter group index.</summary>
	[Reactive] public sbyte TransformParamIndex { get; set; }

	/// <summary>Gets or sets whether the sprite uses extended VRAM (GBA).</summary>
	[Reactive] public bool UseExtendedVram { get; set; }

	/// <summary>Gets or sets whether the sprite uses the second name table.</summary>
	[Reactive] public NullableBoolean UseSecondTable { get; set; }

	/// <summary>Gets the number of tiles that make up this sprite.</summary>
	public UInt32 TileCount { get; set; }

	/// <summary>Gets the screen width for horizontal wrapping (0 if no wrap).</summary>
	public UInt32 WrapWidth { get; set; }

	/// <summary>Gets the screen height for vertical wrapping (0 if no wrap).</summary>
	public UInt32 WrapHeight { get; set; }

	/// <summary>Gets the addresses of all tiles in this sprite.</summary>
	public UInt32[] TileAddresses { get; set; } = [];

	private UInt32[] _rawPreview = new UInt32[128 * 128];

	/// <summary>Gets or sets the sprite preview bitmap.</summary>
	[Reactive] public DynamicBitmap? SpritePreview { get; set; }

	/// <summary>Gets or sets the zoom factor for preview display.</summary>
	[Reactive] public double SpritePreviewZoom { get; set; }

	/// <summary>Gets or sets whether to fade the preview (sprite not visible).</summary>
	[Reactive] public bool FadePreview { get; set; }

	/// <summary>Gets the actual width accounting for double-size mode.</summary>
	public int RealWidth => Width / (DoubleSize == NullableBoolean.True ? 2 : 1);

	/// <summary>Gets the actual height accounting for double-size mode.</summary>
	public int RealHeight => Height / (DoubleSize == NullableBoolean.True ? 2 : 1);

	/// <summary>
	/// Initializes the model from debug sprite information.
	/// </summary>
	/// <param name="sprite">The sprite info from the debug API.</param>
	/// <param name="spritePreviews">Array of sprite preview pixel data.</param>
	/// <param name="previewInfo">Preview rendering configuration.</param>
	public unsafe void Init(ref DebugSpriteInfo sprite, UInt32[] spritePreviews, DebugSpritePreviewInfo previewInfo) {
		SpriteIndex = sprite.SpriteIndex;

		X = sprite.X;
		Y = sprite.Y;
		RawX = sprite.RawX;
		RawY = sprite.RawY;
		PreviewX = sprite.X + previewInfo.CoordOffsetX;
		PreviewY = sprite.Y + previewInfo.CoordOffsetY;

		Width = sprite.Width;
		Height = sprite.Height;
		TileIndex = sprite.TileIndex;
		Priority = sprite.Priority;
		Mode = sprite.Mode;
		Bpp = sprite.Bpp;
		Format = sprite.Format;
		Palette = sprite.Palette;
		TileAddress = sprite.TileAddress;
		PaletteAddress = sprite.PaletteAddress;
		UseSecondTable = sprite.UseSecondTable;
		UseExtendedVram = sprite.UseExtendedVram;

		WrapWidth = previewInfo.WrapRightToLeft ? previewInfo.Width : 0;
		WrapHeight = previewInfo.WrapBottomToTop ? previewInfo.Height : 0;

		TileCount = sprite.TileCount;
		fixed (UInt32* p = sprite.TileAddresses) {
			if (TileAddresses is null || TileAddresses.Length != TileCount) {
				TileAddresses = new UInt32[TileCount];
			}

			for (int i = 0; i < sprite.TileCount; i++) {
				TileAddresses[i] = p[i];
			}
		}

		fixed (UInt32* p = spritePreviews) {
			bool needUpdate = false;

			UInt32* spritePreview = p + (sprite.SpriteIndex * 128 * 128);

			if (SpritePreview is null || SpritePreview.PixelSize.Width != sprite.Width || SpritePreview.PixelSize.Height != sprite.Height) {
				SpritePreview?.Dispose();
				SpritePreview = new DynamicBitmap(new PixelSize(Width, Height), new Vector(96, 96), PixelFormat.Bgra8888, AlphaFormat.Premul);
				needUpdate = true;
			}

			int len = Width * Height;
			for (int i = 0; i < len; i++) {
				if (_rawPreview[i] != spritePreview[i]) {
					needUpdate = true;
				}

				_rawPreview[i] = spritePreview[i];
			}

			if (needUpdate) {
				int spriteSize = len * sizeof(UInt32);
				using (var bitmapLock = SpritePreview.Lock()) {
					Buffer.MemoryCopy(spritePreview, (void*)bitmapLock.FrameBuffer.Address, spriteSize, spriteSize);
				}
			}
		}

		if (Width == 0 && Height == 0) {
			//Can happen when reloading rom while the UI is updating the sprite list (sprite list will be empty data - all 0s)
			SpritePreviewZoom = 1.0;
		} else {
			SpritePreviewZoom = 32.0 / Math.Max(Width, Height);
		}

		Visibility = sprite.Visibility;
		FadePreview = sprite.Visibility != SpriteVisibility.Visible;

		HorizontalMirror = sprite.HorizontalMirror;
		VerticalMirror = sprite.VerticalMirror;
		MosaicEnabled = sprite.MosaicEnabled;
		TransformEnabled = sprite.TransformEnabled;
		DoubleSize = sprite.DoubleSize;
		TransformParamIndex = sprite.TransformParamIndex;

		string flags = sprite.HorizontalMirror == NullableBoolean.True ? "H" : "";
		flags += sprite.VerticalMirror == NullableBoolean.True ? "V" : "";
		flags += sprite.Visibility == SpriteVisibility.Disabled ? "D" : "";
		flags += sprite.TransformEnabled == NullableBoolean.True ? "T" : "";
		flags += sprite.Mode == DebugSpriteMode.Blending ? "B" : "";
		flags += sprite.Mode == DebugSpriteMode.Window ? "W" : "";
		flags += sprite.MosaicEnabled == NullableBoolean.True ? "M" : "";
		flags += sprite.UseSecondTable == NullableBoolean.True ? "N" : "";
		Flags = flags;
	}

	/// <summary>
	/// Gets the preview rectangles for this sprite, including wrapped portions.
	/// </summary>
	/// <returns>
	/// A tuple of four rectangles:
	/// <list type="bullet">
	///   <item><description>mainRect: The primary sprite rectangle</description></item>
	///   <item><description>wrapTopRect: Portion wrapped to top of screen (vertical wrap)</description></item>
	///   <item><description>wrapLeftRect: Portion wrapped to left of screen (horizontal wrap)</description></item>
	///   <item><description>wrapBothRect: Portion wrapped to top-left corner (both directions)</description></item>
	/// </list>
	/// </returns>
	public ValueTuple<Rect, Rect, Rect, Rect> GetPreviewRect() {
		Rect mainRect = new Rect(PreviewX, PreviewY, Width, Height);
		Rect wrapTopRect = default;
		Rect wrapLeftRect = default;
		Rect wrapBothRect = default;

		bool wrapVertical = WrapHeight > 0 && PreviewY + Height > WrapHeight;
		bool wrapHorizontal = WrapWidth > 0 && PreviewX + Width > WrapWidth;
		if (wrapVertical) {
			//Generate a 2nd rect for the portion of the sprite that wraps
			//around to the top of the screen when it goes past the screen's
			//bottom, when the WrapBottomToTop flag is set (used by SNES/SMS/GBA)
			wrapTopRect = new Rect(PreviewX, PreviewY - WrapHeight, Width, Height);
		}

		if (wrapHorizontal) {
			//Generate a 2nd rect for the portion of the sprite that wraps
			//around to the left of the screen when it goes past the screen's
			//right side, when the WrapRightToLeft flag is set (used by GBA)
			wrapLeftRect = new Rect(PreviewX - WrapWidth, PreviewY, Width, Height);
		}

		if (wrapVertical && wrapHorizontal) {
			wrapBothRect = new Rect(PreviewX - WrapWidth, PreviewY - WrapHeight, Width, Height);
		}

		return (mainRect, wrapTopRect, wrapLeftRect, wrapBothRect);
	}

	/// <summary>
	/// Creates a shallow copy of this sprite preview model.
	/// </summary>
	/// <returns>A new instance with copied property values.</returns>
	public SpritePreviewModel Clone() {
		SpritePreviewModel copy = new SpritePreviewModel();
		CopyTo(copy);
		return copy;
	}

	/// <summary>
	/// Copies all property values to another instance.
	/// </summary>
	/// <param name="dst">The destination instance to copy values to.</param>
	public void CopyTo(SpritePreviewModel dst) {
		dst.SpriteIndex = SpriteIndex;
		dst.X = X;
		dst.Y = Y;
		dst.RawX = RawX;
		dst.RawY = RawY;
		dst.PreviewX = PreviewX;
		dst.PreviewY = PreviewY;
		dst.Width = Width;
		dst.Height = Height;
		dst.TileIndex = TileIndex;
		dst.TileAddress = TileAddress;
		dst.Priority = Priority;
		dst.Mode = Mode;
		dst.Bpp = Bpp;
		dst.Format = Format;
		dst.Palette = Palette;
		dst.PaletteAddress = PaletteAddress;
		dst.Visibility = Visibility;
		dst.Flags = Flags;
		dst.HorizontalMirror = HorizontalMirror;
		dst.VerticalMirror = VerticalMirror;
		dst.FadePreview = FadePreview;

		dst.MosaicEnabled = MosaicEnabled;
		dst.TransformEnabled = TransformEnabled;
		dst.DoubleSize = DoubleSize;
		dst.TransformParamIndex = TransformParamIndex;

		dst.UseExtendedVram = UseExtendedVram;
		dst.UseSecondTable = UseSecondTable;
	}
}
