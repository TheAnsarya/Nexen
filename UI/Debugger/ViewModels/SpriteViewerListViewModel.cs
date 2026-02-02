using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Windows.Input;
using Avalonia.Controls.Selection;
using DataBoxControl;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the sprite viewer list view panel.
/// Displays sprites in a sortable data grid with selection synchronization to the main sprite viewer.
/// </summary>
public class SpriteViewerListViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets whether the list view is visible.
	/// </summary>
	[Reactive] public bool ShowListView { get; set; }

	/// <summary>
	/// Gets or sets the minimum height for the list view panel.
	/// </summary>
	[Reactive] public double MinListViewHeight { get; set; }

	/// <summary>
	/// Gets or sets the current height of the list view panel.
	/// </summary>
	[Reactive] public double ListViewHeight { get; set; }

	/// <summary>
	/// Gets or sets the list of sprite preview models for data grid binding.
	/// </summary>
	[Reactive] public List<SpritePreviewModel>? SpritePreviews { get; set; } = null;

	/// <summary>
	/// Gets or sets the selection model for the data grid.
	/// </summary>
	[Reactive] public SelectionModel<SpritePreviewModel?> Selection { get; set; } = new();

	/// <summary>
	/// Gets or sets the current sort state for the data grid columns.
	/// </summary>
	[Reactive] public SortState SortState { get; set; } = new();

	/// <summary>
	/// Gets the list of column widths for grid persistence.
	/// </summary>
	public List<int> ColumnWidths => SpriteViewer.Config.ColumnWidths;

	/// <summary>
	/// Gets the command to execute when a column sort is requested.
	/// </summary>
	public ICommand SortCommand { get; }

	/// <summary>
	/// Gets the parent sprite viewer view model.
	/// </summary>
	public SpriteViewerViewModel SpriteViewer { get; }

	/// <summary>
	/// Gets the sprite viewer configuration.
	/// </summary>
	public SpriteViewerConfig Config { get; }

	/// <summary>Timestamp of last list refresh for throttling.</summary>
	private DateTime _lastRefresh = DateTime.MinValue;

	/// <summary>
	/// Initializes a new instance of the <see cref="SpriteViewerListViewModel"/> class.
	/// </summary>
	/// <param name="viewer">The parent sprite viewer view model.</param>
	public SpriteViewerListViewModel(SpriteViewerViewModel viewer) {
		SpriteViewer = viewer;
		Config = viewer.Config;
		ShowListView = Config.ShowListView;
		ListViewHeight = Config.ShowListView ? Config.ListViewHeight : 0;

		SortState.SetColumnSort("SpriteIndex", ListSortDirection.Ascending, false);

		SortCommand = ReactiveCommand.Create<string?>(sortMemberPath => RefreshList(true));

		AddDisposable(this.WhenAnyValue(x => x.Selection.SelectedItem).Subscribe(x => {
			if (x != null) {
				SpriteViewer.SelectSprite(x.SpriteIndex);
			}
		}));
	}

	/// <summary>
	/// Selects a sprite in the list view by its index.
	/// </summary>
	/// <param name="spriteIndex">The sprite index to select.</param>
	public void SelectSprite(int spriteIndex) {
		Selection.SelectedItem = SpritePreviews?.Find(x => x.SpriteIndex == spriteIndex);
	}

	/// <summary>
	/// Forces an immediate refresh on the next call by resetting the throttle timestamp.
	/// </summary>
	public void ForceRefresh() {
		_lastRefresh = DateTime.MinValue;
	}

	/// <summary>Comparison functions for sorting sprites by different columns.</summary>
	private Dictionary<string, Func<SpritePreviewModel, SpritePreviewModel, int>> _comparers = new() {
		{ "SpriteIndex", (a, b) => a.SpriteIndex.CompareTo(b.SpriteIndex) },
		{ "X", (a, b) => a.X.CompareTo(b.X) },
		{ "Y", (a, b) => a.Y.CompareTo(b.Y) },
		{ "TileIndex", (a, b) => a.TileIndex.CompareTo(b.TileIndex) },
		{ "Size", (a, b) => {
			int result = a.Width.CompareTo(b.Width);
			if(result == 0) {
				return a.Height.CompareTo(b.Height);
			}

			return result;
		} },
		{ "Palette", (a, b) => a.Palette.CompareTo(b.Palette) },
		{ "Priority", (a, b) => a.Priority.CompareTo(b.Priority) },
		{ "Flags", (a, b) => string.Compare(a.Flags, b.Flags, StringComparison.OrdinalIgnoreCase) },
		{ "Visible", (a, b) => a.FadePreview.CompareTo(b.FadePreview) }
	};

	/// <summary>
	/// Refreshes the sprite list from the parent viewer with optional throttling.
	/// </summary>
	/// <param name="force">If true, bypasses throttling and refreshes immediately.</param>
	public void RefreshList(bool force = false) {
		if (!force && (DateTime.Now - _lastRefresh).TotalMilliseconds < 70) {
			return;
		}

		_lastRefresh = DateTime.Now;

		if (!ShowListView) {
			SpritePreviews = null;
			return;
		}

		if (SpritePreviews == null || SpritePreviews.Count != SpriteViewer.SpritePreviews.Count) {
			SpritePreviews = SpriteViewer.SpritePreviews.Select(x => x.Clone()).ToList();
		}

		int? selectedIndex = Selection.SelectedItem?.SpriteIndex;

		List<SpritePreviewModel> newList = new(SpriteViewer.SpritePreviews.Select(x => x.Clone()).ToList());
		SortHelper.SortList(newList, SortState.SortOrder, _comparers, "SpriteIndex");

		for (int i = 0; i < newList.Count; i++) {
			newList[i].CopyTo(SpritePreviews[i]);
		}

		if (selectedIndex != null && Selection.SelectedItem?.SpriteIndex != selectedIndex) {
			Selection.SelectedItem = null;
		}
	}

	/// <summary>
	/// Initializes reactive observers for list view visibility and height changes.
	/// </summary>
	public void InitListViewObservers() {
		//Update list view height based on show list view flag
		AddDisposable(this.WhenAnyValue(x => x.ShowListView).Subscribe(showListView => {
			Config.ShowListView = showListView;
			ListViewHeight = showListView ? Config.ListViewHeight : 0;
			MinListViewHeight = showListView ? 100 : 0;
			RefreshList(true);
		}));

		AddDisposable(this.WhenAnyValue(x => x.SpriteViewer.SpritePreviews).Subscribe(x => RefreshList(true)));

		AddDisposable(this.WhenAnyValue(x => x.ListViewHeight).Subscribe(height => {
			if (ShowListView) {
				Config.ListViewHeight = height;
			} else {
				ListViewHeight = 0;
			}
		}));
	}
}
