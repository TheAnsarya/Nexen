using System;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels {
	/// <summary>
	/// ViewModel for the video configuration tab.
	/// </summary>
	public class VideoConfigViewModel : DisposableViewModel {
		/// <summary>Gets whether to show custom aspect ratio options.</summary>
		[ObservableAsProperty] public bool ShowCustomRatio { get; }

		/// <summary>Gets whether to show NTSC Blargg filter settings.</summary>
		[ObservableAsProperty] public bool ShowNtscBlarggSettings { get; }

		/// <summary>Gets whether to show NTSC Bisqwit filter settings.</summary>
		[ObservableAsProperty] public bool ShowNtscBisqwitSettings { get; }

		/// <summary>Gets whether to show LCD grid filter settings.</summary>
		[ObservableAsProperty] public bool ShowLcdGridSettings { get; }

		/// <summary>Gets whether the current platform is Windows.</summary>
		public bool IsWindows { get; }

		/// <summary>Gets whether the current platform is macOS.</summary>
		public bool IsMacOs { get; }

		/// <summary>Gets the command to apply composite video preset.</summary>
		public ReactiveCommand<Unit, Unit> PresetCompositeCommand { get; }

		/// <summary>Gets the command to apply S-Video preset.</summary>
		public ReactiveCommand<Unit, Unit> PresetSVideoCommand { get; }

		/// <summary>Gets the command to apply RGB preset.</summary>
		public ReactiveCommand<Unit, Unit> PresetRgbCommand { get; }

		/// <summary>Gets the command to apply monochrome preset.</summary>
		public ReactiveCommand<Unit, Unit> PresetMonochromeCommand { get; }

		/// <summary>Gets the command to reset picture settings.</summary>
		public ReactiveCommand<Unit, Unit> ResetPictureSettingsCommand { get; }

		/// <summary>Gets or sets the current video configuration.</summary>
		[Reactive] public VideoConfig Config { get; set; }

		/// <summary>Gets or sets the original video configuration for revert.</summary>
		[Reactive] public VideoConfig OriginalConfig { get; set; }

		/// <summary>Gets the available refresh rates for selection.</summary>
		public UInt32[] AvailableRefreshRates { get; } = new UInt32[] { 50, 60, 75, 100, 120, 144, 200, 240, 360 };

		/// <summary>
		/// Initializes a new instance of the <see cref="VideoConfigViewModel"/> class.
		/// </summary>
		public VideoConfigViewModel() {
			Config = ConfigManager.Config.Video;
			OriginalConfig = Config.Clone();

			PresetCompositeCommand = ReactiveCommand.Create(() => SetNtscPreset(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, false));
			PresetSVideoCommand = ReactiveCommand.Create(() => SetNtscPreset(0, 0, 0, 0, 20, 0, 20, -100, -100, 0, 15, false));
			PresetRgbCommand = ReactiveCommand.Create(() => SetNtscPreset(0, 0, 0, 0, 20, 0, 70, -100, -100, -100, 15, false));
			PresetMonochromeCommand = ReactiveCommand.Create(() => SetNtscPreset(0, -100, 0, 0, 20, 0, 70, -20, -20, -10, 15, false));
			ResetPictureSettingsCommand = ReactiveCommand.Create(() => ResetPictureSettings());

			AddDisposable(this.WhenAnyValue(_ => _.Config.AspectRatio).Select(_ => _ == VideoAspectRatio.Custom).ToPropertyEx(this, _ => _.ShowCustomRatio));
			AddDisposable(this.WhenAnyValue(_ => _.Config.VideoFilter).Select(_ => _ == VideoFilterType.NtscBlargg).ToPropertyEx(this, _ => _.ShowNtscBlarggSettings));
			AddDisposable(this.WhenAnyValue(_ => _.Config.VideoFilter).Select(_ => _ == VideoFilterType.NtscBisqwit).ToPropertyEx(this, _ => _.ShowNtscBisqwitSettings));
			AddDisposable(this.WhenAnyValue(_ => _.Config.VideoFilter).Select(_ => _ == VideoFilterType.LcdGrid).ToPropertyEx(this, _ => _.ShowLcdGridSettings));
			AddDisposable(this.WhenAnyValue(_ => _.Config.UseSoftwareRenderer).Subscribe(softwareRenderer => {
				if (softwareRenderer) {
					//Not supported
					Config.UseExclusiveFullscreen = false;
					Config.VerticalSync = false;
				}
			}));

			//Exclusive fullscreen is only supported on Windows currently
			IsWindows = OperatingSystem.IsWindows();

			//MacOS only supports the software renderer
			IsMacOs = OperatingSystem.IsMacOS();

			if (Design.IsDesignMode) {
				return;
			}

			AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
		}

		private void SetNtscPreset(int hue, int saturation, int contrast, int brightness, int sharpness, int gamma, int resolution, int artifacts, int fringing, int bleed, int scanlines, bool mergeFields) {
			Config.VideoFilter = VideoFilterType.NtscBlargg;
			Config.Hue = hue;
			Config.Saturation = saturation;
			Config.Contrast = contrast;
			Config.Brightness = brightness;
			Config.NtscSharpness = sharpness;
			Config.NtscGamma = gamma;
			Config.NtscResolution = resolution;
			Config.NtscArtifacts = artifacts;
			Config.NtscFringing = fringing;
			Config.NtscBleed = bleed;
			Config.NtscMergeFields = mergeFields;

			Config.ScanlineIntensity = scanlines;
		}

		private void ResetPictureSettings() {
			SetNtscPreset(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
			Config.NtscScale = NtscBisqwitFilterScale._2x;
			Config.NtscYFilterLength = 0;
			Config.NtscIFilterLength = 50;
			Config.NtscQFilterLength = 50;
			Config.VideoFilter = VideoFilterType.None;

			Config.LcdGridTopLeftBrightness = 100;
			Config.LcdGridTopRightBrightness = 85;
			Config.LcdGridBottomLeftBrightness = 85;
			Config.LcdGridBottomRightBrightness = 85;
		}
	}
}
