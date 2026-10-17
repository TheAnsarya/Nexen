using SkiaSharp;
using Svg.Skia;

if (args.Length < 2) {
	Console.Error.WriteLine("Usage: IconRasterizer <sourceSvgDir> <targetPngDir> [icon1 icon2 ...]");
	return 1;
}

var sourceSvgDir = Path.GetFullPath(args[0]);
var targetPngDir = Path.GetFullPath(args[1]);
var iconNames = args.Skip(2).ToArray();

if (!Directory.Exists(sourceSvgDir)) {
	Console.Error.WriteLine($"Source directory not found: {sourceSvgDir}");
	return 2;
}

if (!Directory.Exists(targetPngDir)) {
	Console.Error.WriteLine($"Target directory not found: {targetPngDir}");
	return 3;
}

if (iconNames.Length == 0) {
	iconNames = Directory.GetFiles(sourceSvgDir, "*.svg")
		.Select(Path.GetFileNameWithoutExtension)
		.Where(x => !string.IsNullOrWhiteSpace(x))
		.ToArray()!;
}

var failures = new List<string>();

foreach (var icon in iconNames) {
	var sourceSvgPath = Path.Combine(sourceSvgDir, icon + ".svg");
	var targetPngPath = Path.Combine(targetPngDir, icon + ".png");

	if (!File.Exists(sourceSvgPath)) {
		failures.Add($"Missing source SVG: {sourceSvgPath}");
		continue;
	}

	if (!File.Exists(targetPngPath)) {
		failures.Add($"Missing target PNG: {targetPngPath}");
		continue;
	}

	try {
		using var targetBitmap = SKBitmap.Decode(targetPngPath);
		if (targetBitmap is null || targetBitmap.Width <= 0 || targetBitmap.Height <= 0) {
			failures.Add($"Invalid target PNG dimensions: {targetPngPath}");
			continue;
		}

		using var svg = new SKSvg();
		using var sourceStream = File.OpenRead(sourceSvgPath);
		var picture = svg.Load(sourceStream);
		if (picture is null) {
			failures.Add($"Could not load SVG: {sourceSvgPath}");
			continue;
		}

		var info = new SKImageInfo(targetBitmap.Width, targetBitmap.Height, SKColorType.Rgba8888, SKAlphaType.Premul);
		using var surface = SKSurface.Create(info);
		var canvas = surface.Canvas;
		canvas.Clear(SKColors.Transparent);

		var bounds = picture.CullRect;
		var scaleX = info.Width / bounds.Width;
		var scaleY = info.Height / bounds.Height;
		var scale = Math.Min(scaleX, scaleY);

		var tx = (info.Width - bounds.Width * scale) / 2f - bounds.Left * scale;
		var ty = (info.Height - bounds.Height * scale) / 2f - bounds.Top * scale;

		canvas.Translate(tx, ty);
		canvas.Scale(scale, scale);
		canvas.DrawPicture(picture);
		canvas.Flush();

		using var image = surface.Snapshot();
		using var data = image.Encode(SKEncodedImageFormat.Png, 100);
		using var outStream = File.Open(targetPngPath, FileMode.Create, FileAccess.Write, FileShare.None);
		data.SaveTo(outStream);

		Console.WriteLine($"Updated {icon}.png ({info.Width}x{info.Height})");
	} catch (Exception ex) {
		failures.Add($"{icon}: {ex.Message}");
	}
}

if (failures.Count > 0) {
	Console.Error.WriteLine("Icon rasterization finished with errors:");
	foreach (var failure in failures) {
		Console.Error.WriteLine(" - " + failure);
	}
	return 4;
}

Console.WriteLine($"Icon rasterization complete. Updated {iconNames.Length} icon(s).");
return 0;
