using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Svg.Skia;

namespace Nexen.Utilities;

public static class ImageUtilities {
	private static readonly ConcurrentDictionary<string, IImage> _imageCache = new(StringComparer.OrdinalIgnoreCase);
	private static readonly ConcurrentDictionary<string, string> _preferredPathCache = new(StringComparer.OrdinalIgnoreCase);

	public static Image FromAsset(string source) {
		return new Image() { Source = ImageFromAsset(source) };
	}

	public static IImage? ImageFromAsset(string source) {
		string preferredPath = ResolvePreferredAssetPath(source);
		try {
			return _imageCache.GetOrAdd(preferredPath, LoadImageCore);
		} catch (Exception ex) {
			Log.Warn($"[ImageUtilities] Failed to load image asset '{source}': {ex.Message}");
			return null;
		}
	}

	public static Bitmap BitmapFromAsset(string source) {
		return new Bitmap(AssetLoader.Open(ToAssetUri(source)));
	}

	public static string ResolvePreferredAssetPath(string source) {
		return _preferredPathCache.GetOrAdd(source, ResolvePreferredAssetPathCore);
	}

	private static string ResolvePreferredAssetPathCore(string source) {
		if (string.IsNullOrWhiteSpace(source)) {
			return string.Empty;
		}

		string normalizedSource = NormalizeAssetPath(source);
		if (normalizedSource.EndsWith(".png", StringComparison.OrdinalIgnoreCase)) {
			string svgCandidate = normalizedSource[..^4] + ".svg";
			if (AssetExists(svgCandidate)) {
				return svgCandidate;
			}
		}

		return normalizedSource;
	}

	private static IImage LoadImageCore(string source) {
		Uri uri = ToAssetUri(source);
		if (source.EndsWith(".svg", StringComparison.OrdinalIgnoreCase)) {
			using Stream stream = AssetLoader.Open(uri);
			SvgSource svgSource = SvgSource.LoadFromStream(stream);
			return new SvgImage { Source = svgSource };
		}

		return new Bitmap(AssetLoader.Open(uri));
	}

	private static bool AssetExists(string source) {
		try {
			using Stream stream = AssetLoader.Open(ToAssetUri(source));
			return stream is not null;
		} catch (Exception ex) {
			Debug.WriteLine($"ImageUtilities.AssetExists: Asset '{source}' not found: {ex.Message}");
			return false;
		}
	}

	private static string NormalizeAssetPath(string source) {
		return source.Trim().TrimStart('/').Replace('\\', '/');
	}

	private static Uri ToAssetUri(string source) {
		return new Uri("avares://Nexen/" + NormalizeAssetPath(source));
	}
}
