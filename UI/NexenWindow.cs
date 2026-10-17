using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Media;
using Avalonia.VisualTree;
using Nexen.Config;
using Nexen.Utilities;

namespace Nexen;

public class NexenWindow : Window {
	static NexenWindow() {
		PopupRoot.ClientSizeProperty.Changed.AddClassHandler<PopupRoot>((s, e) => {
			foreach (var v in s.GetVisualChildren()) {
				SetTextRenderingMode(v);
			}
		});
	}

	protected override void OnInitialized() {
		base.OnInitialized();
		Focusable = true;
		SetTextRenderingMode(this);
	}

	private static void SetTextRenderingMode(Visual v) {
		// During the setup wizard, Config may not be available yet — the user hasn't
		// chosen a home folder, so accessing ConfigManager.Config would prematurely
		// create ~/.config/Nexen/ (Linux) or trigger a TypeInitializationException
		// if the file system isn't ready. Default to SubPixelAntialias in that case.
		FontAntialiasing antialiasing;
		try {
			if (App.ShowConfigWindow) {
				antialiasing = FontAntialiasing.SubPixelAntialias;
			} else {
				antialiasing = ConfigManager.Config.Preferences.FontAntialiasing;
			}
		} catch (Exception ex) {
			Log.Error(ex, "[NexenWindow] Failed to read FontAntialiasing preference, using default");
			antialiasing = FontAntialiasing.SubPixelAntialias;
		}

		switch (antialiasing) {
			case FontAntialiasing.Disabled:
				TextOptions.SetTextRenderingMode(v, TextRenderingMode.Alias);
				break;

			case FontAntialiasing.Antialias:
				TextOptions.SetTextRenderingMode(v, TextRenderingMode.Antialias);
				break;

			default:
			case FontAntialiasing.SubPixelAntialias:
				TextOptions.SetTextRenderingMode(v, TextRenderingMode.SubpixelAntialias);
				break;
		}
	}

	protected override void OnClosed(EventArgs e) {
		base.OnClosed(e);

		if (DataContext is IDisposable disposable) {
			disposable.Dispose();
		}

		//This fixes (or just dramatically reduces?) a memory leak
		//Most windows don't get GCed properly if this isn't done, leading to large memory leaks
		DataContext = null;
	}
}
