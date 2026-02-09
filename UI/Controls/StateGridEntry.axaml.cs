using System;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Controls;
public class StateGridEntry : UserControl {
	private static readonly WriteableBitmap EmptyImage = new WriteableBitmap(new PixelSize(256, 240), new Vector(96, 96), Avalonia.Platform.PixelFormat.Rgba8888, Avalonia.Platform.AlphaFormat.Opaque);

	public static readonly StyledProperty<RecentGameInfo> EntryProperty = AvaloniaProperty.Register<StateGridEntry, RecentGameInfo>(nameof(Entry));
	public static readonly StyledProperty<Bitmap?> ImageProperty = AvaloniaProperty.Register<StateGridEntry, Bitmap?>(nameof(Image));
	public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<StateGridEntry, string>(nameof(Title));
	public static readonly StyledProperty<string> SubTitleProperty = AvaloniaProperty.Register<StateGridEntry, string>(nameof(SubTitle));
	public static readonly StyledProperty<bool> EnabledProperty = AvaloniaProperty.Register<StateGridEntry, bool>(nameof(Enabled));
	public static readonly StyledProperty<bool> IsActiveEntryProperty = AvaloniaProperty.Register<StateGridEntry, bool>(nameof(IsActiveEntry));
	public static readonly StyledProperty<double> AspectRatioProperty = AvaloniaProperty.Register<StateGridEntry, double>(nameof(AspectRatio));
	public static readonly StyledProperty<string> BadgeTextProperty = AvaloniaProperty.Register<StateGridEntry, string>(nameof(BadgeText), "");
	public static readonly StyledProperty<IBrush> BadgeBackgroundProperty = AvaloniaProperty.Register<StateGridEntry, IBrush>(nameof(BadgeBackground), Brushes.Transparent);
	public static readonly StyledProperty<bool> ShowBadgeProperty = AvaloniaProperty.Register<StateGridEntry, bool>(nameof(ShowBadge), false);

	public RecentGameInfo Entry {
		get { return GetValue(EntryProperty); }
		set { SetValue(EntryProperty, value); }
	}

	public Bitmap? Image {
		get { return GetValue(ImageProperty); }
		set { SetValue(ImageProperty, value); }
	}

	public double AspectRatio {
		get { return GetValue(AspectRatioProperty); }
		set { SetValue(AspectRatioProperty, value); }
	}

	public bool Enabled {
		get { return GetValue(EnabledProperty); }
		set { SetValue(EnabledProperty, value); }
	}

	public bool IsActiveEntry {
		get { return GetValue(IsActiveEntryProperty); }
		set { SetValue(IsActiveEntryProperty, value); }
	}

	public string Title {
		get { return GetValue(TitleProperty); }
		set { SetValue(TitleProperty, value); }
	}

	public string SubTitle {
		get { return GetValue(SubTitleProperty); }
		set { SetValue(SubTitleProperty, value); }
	}

	public string BadgeText {
		get { return GetValue(BadgeTextProperty); }
		set { SetValue(BadgeTextProperty, value); }
	}

	public IBrush BadgeBackground {
		get { return GetValue(BadgeBackgroundProperty); }
		set { SetValue(BadgeBackgroundProperty, value); }
	}

	public bool ShowBadge {
		get { return GetValue(ShowBadgeProperty); }
		set { SetValue(ShowBadgeProperty, value); }
	}

	// Badge colors for different save state origins (Bootstrap-style)
	private static readonly IBrush BadgeBlue = new SolidColorBrush(Color.Parse("#0d6efd"));    // Auto save
	private static readonly IBrush BadgeGreen = new SolidColorBrush(Color.Parse("#198754"));   // User save
	private static readonly IBrush BadgeRed = new SolidColorBrush(Color.Parse("#dc3545"));     // Recent play
	private static readonly IBrush BadgeYellow = new SolidColorBrush(Color.Parse("#ffc107"));  // Lua save

	static StateGridEntry() {
		//Make empty image black
		using var imgLock = EmptyImage.Lock();
		unsafe {
			UInt32* buffer = (UInt32*)imgLock.Address;
			for (int i = 0; i < 256 * 240; i++) {
				buffer[i] = 0xFF000000;
			}
		}
	}

	public StateGridEntry() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void OnImageClick(object sender, RoutedEventArgs e) {
		if (!IsEffectivelyVisible) {
			return;
		}

		Entry.Load();
	}

	public void Init() {
		RecentGameInfo game = Entry;
		if (game == null) {
			return;
		}

		Title = game.Name;

		bool fileExists = File.Exists(game.FileName);
		if (fileExists) {
			if (game.IsTimestampedSave && !string.IsNullOrEmpty(game.FriendlyTimestamp)) {
				// Use the friendly timestamp for timestamped saves (e.g., "Today 2:30 PM")
				SubTitle = game.FriendlyTimestamp;
			} else {
				string ext = Path.GetExtension(game.FileName);
				if (ext is ("." + FileDialogHelper.NexenSaveStateExt) or ("." + FileDialogHelper.MesenSaveStateExt)) {
					SubTitle = new FileInfo(game.FileName).LastWriteTime.ToString();
				} else {
					DateTime writeTime = new FileInfo(game.FileName).LastWriteTime;
					SubTitle = writeTime.ToShortDateString() + " " + writeTime.ToShortTimeString();
				}
			}
		} else {
			SubTitle = ResourceHelper.GetMessage("EmptyState");
		}

		// Set origin badge for timestamped saves
		if (game.IsTimestampedSave) {
			ShowBadge = true;
			(BadgeText, BadgeBackground) = game.Origin switch {
				SaveStateOrigin.Auto => ("Auto", BadgeBlue),
				SaveStateOrigin.Recent => ("Recent", BadgeRed),
				SaveStateOrigin.Lua => ("Lua", BadgeYellow),
				_ => ("Save", BadgeGreen) // Default to Save
			};
		} else {
			ShowBadge = false;
		}

		Enabled = fileExists || game.SaveMode;
		Image = StateGridEntry.EmptyImage;

		if (fileExists) {
			Task.Run(() => {
				Bitmap? img = null;
				double aspectRatio = 0;
				try {
					string ext = Path.GetExtension(game.FileName);
					if (ext is ("." + FileDialogHelper.NexenSaveStateExt) or ("." + FileDialogHelper.MesenSaveStateExt)) {
						img = EmuApi.GetSaveStatePreview(game.FileName);
					} else {
						using FileStream? fs = FileHelper.OpenRead(game.FileName);
						if (fs != null) {
							ZipArchive zip = new ZipArchive(fs);
							ZipArchiveEntry? entry = zip.GetEntry("Screenshot.png");
							if (entry != null) {
								using Stream stream = entry.Open();

								//Copy to a memory stream (to avoid what looks like a Skia or Avalonia issue?)
								using MemoryStream ms = new MemoryStream();
								stream.CopyTo(ms);
								ms.Seek(0, SeekOrigin.Begin);

								img = new Bitmap(ms);
							}

							entry = zip.GetEntry("RomInfo.txt");
							if (entry != null) {
								using StreamReader reader = new StreamReader(entry.Open());
								string? line = null;
								while ((line = reader.ReadLine()) != null) {
									if (line.StartsWith("aspectratio=")) {
										double.TryParse(line.Split('=')[1], NumberStyles.Float | NumberStyles.AllowThousands, CultureInfo.InvariantCulture, out aspectRatio);
										break;
									}
								}
							}
						}
					}
				} catch { }

				Dispatcher.UIThread.Post(() => {
					Image = img ?? StateGridEntry.EmptyImage;
					AspectRatio = aspectRatio;
				});
			});
		}
	}
}
