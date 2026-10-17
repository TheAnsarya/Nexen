using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.GUI.Utilities;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Windows;
public partial class SelectRomWindow : NexenWindow {
	private ListBox _listBox;
	private TextBox _searchBox;

	public SelectRomWindow() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif

		_searchBox = this.GetControl<TextBox>("Search");
		_listBox = this.GetControl<ListBox>("ListBox");
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);

		//Post this to allow focus to work properly when drag and dropping file
		Dispatcher.UIThread.Post(() => {
			Activate();
			_searchBox.Focus();
		});
	}

	protected override void OnKeyDown(KeyEventArgs e) {
		if (DataContext is SelectRomViewModel model) {
			if (e.Key is Key.Down or Key.Up) {
				if (_searchBox.IsKeyboardFocusWithin) {
					var firstTwo = model.FilteredEntries.Take(2).ToList();
					if (firstTwo.Count > 1) {
						model.SelectedEntry = firstTwo[1];
						_listBox.ContainerFromIndex(1)?.Focus();
					} else if (firstTwo.Count > 0) {
						model.SelectedEntry = firstTwo[0];
						_listBox.ContainerFromIndex(1)?.Focus();
					}
				}
			} else if (e.Key == Key.Enter && model.SelectedEntry != null) {
				model.Cancelled = false;
				Close();
			} else if (e.Key == Key.Escape) {
				Close();
			}
		}

		base.OnKeyDown(e);
	}

	public static async Task<ResourcePath?> Show(string file) {
		List<ArchiveRomEntry> entries = ArchiveHelper.GetArchiveRomList(file);
		if (entries.Count == 0) {
			return file;
		} else if (entries.Count == 1) {
			return new ResourcePath() { Path = file, InnerFile = entries[0].ArchiveKey, InnerFileIndex = 0 };
		}

		SelectRomViewModel model = new(entries) { SelectedEntry = entries[0] };
		SelectRomWindow wnd = new SelectRomWindow() { DataContext = model };

		wnd.WindowStartupLocation = WindowStartupLocation.CenterOwner;
		Window? parent = ApplicationHelper.GetMainWindow();
		if (parent == null) {
			return null;
		}

		await wnd.ShowDialog(parent);

		if (model.Cancelled || model.SelectedEntry == null) {
			return null;
		}

		return new ResourcePath() { Path = file, InnerFile = model.SelectedEntry.ArchiveKey, InnerFileIndex = 0 };
	}

	private void OnOkClick(object sender, RoutedEventArgs e) {
		if (DataContext is SelectRomViewModel model && model.SelectedEntry != null) {
			model.Cancelled = false;
			Close();
		}
	}

	private void OnCancelClick(object sender, RoutedEventArgs e) {
		Close();
	}

	bool _isDoubleTap = false;
	private void OnPointerReleased(object? sender, PointerReleasedEventArgs e) {
		if (_isDoubleTap) {
			if (DataContext is SelectRomViewModel model && model.SelectedEntry != null) {
				model.Cancelled = false;
				Close();
			}

			_isDoubleTap = false;
		}
	}

	private void OnDoubleTapped(object sender, TappedEventArgs e) {
		_isDoubleTap = true;
	}

	protected override void OnClosed(EventArgs e) {
		((SelectRomViewModel?)DataContext)?.Dispose();
		base.OnClosed(e);
	}
}

public partial class SelectRomViewModel : DisposableViewModel {
	private readonly List<ArchiveRomEntry> _entries;
	[Reactive] public partial IEnumerable<ArchiveRomEntry> FilteredEntries { get; set; }
	[Reactive] public partial string SearchString { get; set; } = "";
	[Reactive] public partial ArchiveRomEntry? SelectedEntry { get; set; }
	[Reactive] public partial bool Cancelled { get; set; } = true;

	public SelectRomViewModel(List<ArchiveRomEntry> entries) {
		_entries = entries;
		FilteredEntries = entries;
		AddDisposable(this.WhenAnyValue(x => x.SearchString).Subscribe(x => {
			FilteredEntries = string.IsNullOrWhiteSpace(x) ? _entries : _entries.Where(e => e.Filename.Contains(x, StringComparison.OrdinalIgnoreCase)).ToList();

			SelectedEntry = FilteredEntries.FirstOrDefault();
		}));
	}
}
