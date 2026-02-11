using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media.Imaging;
using Avalonia.Platform;

namespace Nexen.Utilities;

public static class ImageUtilities {
	public static Image FromAsset(string source) {
		return new Image() { Source = new Bitmap(AssetLoader.Open(new Uri("avares://Nexen/" + source))) };
	}

	public static Bitmap BitmapFromAsset(string source) {
		return new Bitmap(AssetLoader.Open(new Uri("avares://Nexen/" + source)));
	}
}
