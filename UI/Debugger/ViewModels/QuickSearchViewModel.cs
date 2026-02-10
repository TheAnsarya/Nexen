using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Threading;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;

/// <summary>
/// ViewModel for the quick search overlay in debugger windows.
/// </summary>
/// <remarks>
/// <para>
/// Provides incremental search functionality with:
/// <list type="bullet">
///   <item><description>Real-time search as the user types</description></item>
///   <item><description>Find next/previous navigation</description></item>
///   <item><description>Error indication when no match is found</description></item>
///   <item><description>Optimized search skipping when previous search had no results</description></item>
/// </list>
/// </para>
/// <para>
/// Search is performed asynchronously to avoid blocking the UI during large searches.
/// </para>
/// </remarks>
public sealed class QuickSearchViewModel : ViewModelBase {
	/// <summary>Gets or sets whether the search box is visible.</summary>
	[Reactive] public bool IsSearchBoxVisible { get; set; }

	/// <summary>Gets or sets the current search string.</summary>
	[Reactive] public string SearchString { get; set; } = "";

	/// <summary>Gets or sets whether the error indicator is visible (no match found).</summary>
	[Reactive] public bool IsErrorVisible { get; set; } = false;

	/// <summary>
	/// Event handler delegate for find operations.
	/// </summary>
	/// <param name="e">The search event arguments.</param>
	public delegate void OnFindEventHandler(OnFindEventArgs e);

	/// <summary>
	/// Raised when a find operation should be performed by the parent view.
	/// </summary>
	public event OnFindEventHandler? OnFind;

	private string _noMatchSearch = "";
	private bool _searchInProgress = false;
	private OnFindEventArgs? _searchArgs;

	private TextBox? _txtSearch = null;

	/// <summary>
	/// Initializes a new instance of the <see cref="QuickSearchViewModel"/> class.
	/// </summary>
	/// <remarks>
	/// Sets up reactive subscription to trigger search when search string changes.
	/// </remarks>
	public QuickSearchViewModel() {
		this.WhenAnyValue(x => x.SearchString).Subscribe(x => {
			if (!string.IsNullOrWhiteSpace(x)) {
				// Optimization: skip search if current string starts with a previous no-match string
				if (!string.IsNullOrEmpty(_noMatchSearch) && x.StartsWith(_noMatchSearch)) {
					//Previous search gave no result, current search starts with the same string, so can't give results either, don't search
					return;
				}

				Find(SearchDirection.Forward, false);
			}
		});
	}

	/// <summary>
	/// Opens the search box and focuses the search input.
	/// </summary>
	public void Open() {
		IsSearchBoxVisible = true;
		_txtSearch?.FocusAndSelectAll();
	}

	/// <summary>
	/// Closes the search box and clears the search string.
	/// </summary>
	/// <param name="param">Unused parameter (for command binding compatibility).</param>
	public void Close(object? param = null) {
		IsSearchBoxVisible = false;
		SearchString = "";
	}

	/// <summary>
	/// Finds the previous occurrence of the search string.
	/// </summary>
	/// <param name="param">Unused parameter (for command binding compatibility).</param>
	public void FindPrev(object? param = null) {
		Find(SearchDirection.Backward, false);
	}

	/// <summary>
	/// Finds the next occurrence of the search string.
	/// </summary>
	/// <param name="param">Unused parameter (for command binding compatibility).</param>
	public void FindNext(object? param = null) {
		Find(SearchDirection.Forward, true);
	}

	/// <summary>
	/// Performs a find operation in the specified direction.
	/// </summary>
	/// <param name="dir">The search direction (Forward or Backward).</param>
	/// <param name="skipCurrent">Whether to skip the current match and find the next one.</param>
	/// <remarks>
	/// <para>
	/// If the search string is empty, opens the search box instead.
	/// Search is performed asynchronously on a background thread.
	/// </para>
	/// <para>
	/// Uses Interlocked.Exchange to coalesce rapid search requests,
	/// ensuring only the most recent search is processed.
	/// </para>
	/// </remarks>
	public void Find(SearchDirection dir, bool skipCurrent) {
		if (string.IsNullOrWhiteSpace(SearchString)) {
			Open();
			return;
		}

		_searchArgs = new OnFindEventArgs() { SearchString = SearchString, Direction = dir, SkipCurrent = skipCurrent };
		if (_searchInProgress) {
			return;
		}

		_searchInProgress = true;

		Task.Run(() => {
			OnFindEventArgs? args;
			OnFindEventArgs? lastArgs = null;
			// Process search requests until the queue is empty
			while ((args = Interlocked.Exchange(ref _searchArgs, null)) != null) {
				//Keep searching until the most recent search is processed
				OnFind?.Invoke(args);
				lastArgs = args;
			}

			// Update UI on dispatcher thread
			Dispatcher.UIThread.Post(() => {
				if (lastArgs is not null) {
					if (lastArgs.Success) {
						IsErrorVisible = false;
						_noMatchSearch = "";
					} else {
						IsErrorVisible = true;
						_noMatchSearch = SearchString;
					}
				}
			});
			_searchInProgress = false;
		});
	}

	/// <summary>
	/// Sets the search text box reference for focus management.
	/// </summary>
	/// <param name="txtSearch">The search TextBox control.</param>
	public void SetSearchBox(TextBox txtSearch) {
		_txtSearch = txtSearch;
	}
}

/// <summary>
/// Event arguments for find operations in quick search.
/// </summary>
public sealed class OnFindEventArgs : EventArgs {
	/// <summary>Gets the search string to find.</summary>
	public string SearchString { get; init; } = "";

	/// <summary>Gets the search direction.</summary>
	public SearchDirection Direction { get; init; }

	/// <summary>Gets whether to skip the current match.</summary>
	public bool SkipCurrent { get; init; }

	/// <summary>Gets or sets whether the search found a match.</summary>
	public bool Success { get; set; } = true;
}
