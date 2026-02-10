using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia;
using Nexen.Config;
using Nexen.Controls;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels; 
/// <summary>
/// Primary ViewModel for the main application window.
/// Manages the menu, ROM state, rendering, and recent games display.
/// </summary>
public sealed class MainWindowViewModel : ViewModelBase {
	/// <summary>Gets the singleton instance of the main window ViewModel.</summary>
	public static MainWindowViewModel Instance { get; private set; } = null!;

	/// <summary>Gets or sets the main menu ViewModel.</summary>
	[Reactive] public MainMenuViewModel MainMenu { get; set; }

	/// <summary>Gets or sets the currently loaded ROM information.</summary>
	[Reactive] public RomInfo RomInfo { get; set; }

	/// <summary>Gets or sets the audio player ViewModel for music formats (NSF, SPC, etc.).</summary>
	[Reactive] public AudioPlayerViewModel? AudioPlayer { get; private set; }

	/// <summary>Gets or sets the recent games selection screen ViewModel.</summary>
	[Reactive] public RecentGamesViewModel RecentGames { get; private set; }

	/// <summary>Gets or sets the window title bar text.</summary>
	[Reactive] public string WindowTitle { get; private set; } = "Nexen";

	/// <summary>Gets or sets the current render surface size.</summary>
	[Reactive] public Size RendererSize { get; set; }

	/// <summary>Gets or sets whether the main menu is visible.</summary>
	[Reactive] public bool IsMenuVisible { get; set; }

	/// <summary>Gets or sets whether the native (GPU) renderer is visible.</summary>
	[Reactive] public bool IsNativeRendererVisible { get; set; }

	/// <summary>Gets or sets whether the software renderer is visible.</summary>
	[Reactive] public bool IsSoftwareRendererVisible { get; set; }

	/// <summary>Gets the software renderer ViewModel.</summary>
	public SoftwareRendererViewModel SoftwareRenderer { get; } = new();

	/// <summary>Gets the application configuration.</summary>
	public Configuration Config { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="MainWindowViewModel"/> class.
	/// </summary>
	public MainWindowViewModel() {
		Instance = this;

		Config = ConfigManager.Config;
		MainMenu = new MainMenuViewModel(this);
		RomInfo = new RomInfo();
		RecentGames = new RecentGamesViewModel();

		IsMenuVisible = !Config.Preferences.AutoHideMenu;
	}

	/// <summary>
	/// Initializes the ViewModel with the main window reference.
	/// Sets up reactive subscriptions for UI updates.
	/// </summary>
	/// <param name="wnd">The main window instance.</param>
	public void Init(MainWindow wnd) {
		MainMenu.Initialize(wnd);
		RecentGames.Init(GameScreenMode.RecentGames);

		this.WhenAnyValue(x => x.RecentGames.Visible, x => x.SoftwareRenderer.FrameSurface).Subscribe(x => {
			IsNativeRendererVisible = !RecentGames.Visible && SoftwareRenderer.FrameSurface is null;
			IsSoftwareRendererVisible = !RecentGames.Visible && SoftwareRenderer.FrameSurface is not null;
		});

		this.WhenAnyValue(x => x.RomInfo).Subscribe(x => {
			bool showAudioPlayer = x.Format is RomFormat.Nsf or RomFormat.Spc or RomFormat.Gbs or RomFormat.PceHes;
			if (AudioPlayer is null && showAudioPlayer) {
				AudioPlayer = new AudioPlayerViewModel();
			} else if (!showAudioPlayer) {
				AudioPlayer = null;
			}
		});

		this.WhenAnyValue(
			x => x.RomInfo,
			x => x.RendererSize,
			x => x.Config.Preferences.ShowTitleBarInfo,
			x => x.Config.Video.AspectRatio,
			x => x.Config.Video.VideoFilter
		).Subscribe(x => UpdateWindowTitle());

		UpdateWindowTitle();
	}

	/// <summary>
	/// Updates the window title with ROM name and optional size/filter info.
	/// </summary>
	private void UpdateWindowTitle() {
		string title = "Nexen";
		string romName = RomInfo.GetRomName();
		if (!string.IsNullOrWhiteSpace(romName)) {
			title += " - " + romName;
			if (ConfigManager.Config.Preferences.ShowTitleBarInfo) {
				FrameInfo baseSize = EmuApi.GetBaseScreenSize();
				double scale = (double)RendererSize.Height / baseSize.Height;
				title += $" - {Math.Round(RendererSize.Width)}x{Math.Round(RendererSize.Height)} ({scale:0.###}x, {ResourceHelper.GetEnumText(ConfigManager.Config.Video.VideoFilter)})";
			}
		}

		WindowTitle = title;
	}
}
