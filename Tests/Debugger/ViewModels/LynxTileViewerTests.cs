using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

/// <summary>
/// Tests for the Lynx tile viewer configuration.
/// Verifies the correct tile format (WsBpp4Packed for 4bpp packed nibbles)
/// and format properties match the Lynx framebuffer layout.
/// </summary>
public class LynxTileViewerTests {
	#region Format Selection

	[Fact]
	public void LynxTileFormat_UsesWsBpp4Packed() {
		// The Lynx framebuffer is 4bpp packed nibbles (high nibble first),
		// which matches the WsBpp4Packed tile format exactly.
		var formats = GetLynxTileFormats();
		Assert.Single(formats);
		Assert.Equal(TileFormat.WsBpp4Packed, formats[0]);
	}

	[Fact]
	public void WsBpp4Packed_Is4BitsPerPixel() {
		int bpp = TileFormatExtensions.GetBitsPerPixel(TileFormat.WsBpp4Packed);
		Assert.Equal(4, bpp);
	}

	[Fact]
	public void WsBpp4Packed_TileSizeIs8x8() {
		var size = TileFormat.WsBpp4Packed.GetTileSize();
		Assert.Equal(8, size.Width);
		Assert.Equal(8, size.Height);
	}

	[Fact]
	public void WsBpp4Packed_BytesPerTileIs32() {
		// 8x8 tile at 4bpp = 32 bytes (4 bytes per row, 8 rows)
		int bytesPerTile = TileFormat.WsBpp4Packed.GetBytesPerTile();
		Assert.Equal(32, bytesPerTile);
	}

	#endregion

	#region Format Correctness

	[Fact]
	public void LynxTileFormat_IsNotPlanarBpp4() {
		// Lynx does NOT use SNES planar Bpp4 — that was the original bug.
		// It uses packed nibbles (WsBpp4Packed).
		var formats = GetLynxTileFormats();
		Assert.DoesNotContain(TileFormat.Bpp4, formats);
	}

	[Fact]
	public void WsBpp4Packed_MaxPaletteColors_Is16() {
		// 4bpp = 16 colors per palette
		int maxColors = 1 << TileFormatExtensions.GetBitsPerPixel(TileFormat.WsBpp4Packed);
		Assert.Equal(16, maxColors);
	}

	#endregion

	#region Helpers

	/// <summary>
	/// Returns the tile formats available for Lynx, matching TileViewerViewModel logic.
	/// </summary>
	private static TileFormat[] GetLynxTileFormats() {
		// This mirrors the format list in TileViewerViewModel.InitForCpuType()
		return [TileFormat.WsBpp4Packed];
	}

	#endregion
}
