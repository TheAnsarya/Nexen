using System;
using System.Collections.Concurrent;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Reflection;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;

namespace Nexen.Utilities;

public static class ImageUtilities {
	private static readonly ConcurrentDictionary<string, IImage> _imageCache = new(StringComparer.OrdinalIgnoreCase);
	private static readonly ConcurrentDictionary<string, string> _preferredPathCache = new(StringComparer.OrdinalIgnoreCase);

	public static Image FromAsset(string source) {
		return new Image() { Source = ImageFromAsset(source) };
	}

	public static IImage ImageFromAsset(string source) {
		string preferredPath = ResolvePreferredAssetPath(source);
		return _imageCache.GetOrAdd(preferredPath, LoadImageCore);
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
		if (source.EndsWith(".svg", StringComparison.OrdinalIgnoreCase) && TryCreateSvgImage(uri, out IImage? svgImage)) {
			return svgImage!;
		}

		return new Bitmap(AssetLoader.Open(uri));
	}

	private static bool TryCreateSvgImage(Uri uri, out IImage? image) {
		image = null;

		try {
			Type? svgImageType = Type.GetType("Avalonia.Svg.Skia.SvgImage, Avalonia.Svg.Skia", throwOnError: false);
			Type? svgSourceType = Type.GetType("Avalonia.Svg.Skia.SvgSource, Avalonia.Svg.Skia", throwOnError: false);
			if (svgImageType is null || svgSourceType is null) {
				return false;
			}

			object? svgSource = CreateSvgSource(svgSourceType, uri);
			if (svgSource is null) {
				return false;
			}

			object? svgImage = Activator.CreateInstance(svgImageType);
			PropertyInfo? sourceProperty = svgImageType.GetProperty("Source", BindingFlags.Public | BindingFlags.Instance);
			if (svgImage is null || sourceProperty is null) {
				return false;
			}

			sourceProperty.SetValue(svgImage, svgSource);
			image = svgImage as IImage;
			return image is not null;
		} catch {
			image = null;
			return false;
		}
	}

	private static object? CreateSvgSource([DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicMethods)] Type svgSourceType, Uri uri) {
		MethodInfo[] loadMethods = svgSourceType
			.GetMethods(BindingFlags.Public | BindingFlags.Static)
			.Where(x => x.ReturnType == svgSourceType && x.Name.StartsWith("Load", StringComparison.Ordinal))
			.ToArray();

		foreach (MethodInfo method in loadMethods) {
			if (TryInvokeSvgLoadMethod(method, uri, out object? result)) {
				return result;
			}
		}

		return null;
	}

	private static bool TryInvokeSvgLoadMethod(MethodInfo method, Uri uri, out object? result) {
		result = null;
		Stream? stream = null;

		try {
			ParameterInfo[] parameters = method.GetParameters();
			object?[] args = new object?[parameters.Length];
			for (int i = 0; i < parameters.Length; i++) {
				Type parameterType = parameters[i].ParameterType;
				if (typeof(Uri).IsAssignableFrom(parameterType)) {
					args[i] = uri;
				} else if (parameterType == typeof(string)) {
					args[i] = uri.ToString();
				} else if (typeof(Stream).IsAssignableFrom(parameterType)) {
					stream ??= AssetLoader.Open(uri);
					args[i] = stream;
				} else if (parameters[i].HasDefaultValue) {
					args[i] = parameters[i].DefaultValue;
				} else if (!parameterType.IsValueType || Nullable.GetUnderlyingType(parameterType) is not null) {
					args[i] = null;
				} else {
					return false;
				}
			}

			result = method.Invoke(null, args);
			return result is not null;
		} catch {
			result = null;
			return false;
		} finally {
			stream?.Dispose();
		}
	}

	private static bool AssetExists(string source) {
		try {
			using Stream stream = AssetLoader.Open(ToAssetUri(source));
			return stream is not null;
		} catch {
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
