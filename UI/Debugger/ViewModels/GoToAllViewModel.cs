using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Avalonia.Controls.Selection;
using Nexen.Debugger.Integration;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.ViewModels;

/// <summary>
/// ViewModel for the "Go To All" dialog, providing unified search across labels, symbols, and addresses.
/// </summary>
/// <remarks>
/// <para>
/// This ViewModel provides a unified search interface that can find:
/// <list type="bullet">
///   <item><description>Labels by name</description></item>
///   <item><description>Symbols from external symbol providers</description></item>
///   <item><description>Addresses by hexadecimal value</description></item>
///   <item><description>Source file locations (when source integration is enabled)</description></item>
/// </list>
/// </para>
/// <para>
/// Results are filtered and sorted by relevance as the user types.
/// </para>
/// </remarks>
public sealed partial class GoToAllViewModel : DisposableViewModel {
	/// <summary>Gets or sets the current search string.</summary>
	[Reactive] public partial string SearchString { get; set; } = "";

	/// <summary>Gets or sets the list of search results.</summary>
	[Reactive] public partial List<SearchResultInfo> SearchResults { get; set; } = new();

	/// <summary>Gets or sets the selection model for the results list.</summary>
	[Reactive] public partial SelectionModel<SearchResultInfo?> SelectionModel { get; set; } = new();

	/// <summary>Gets or sets the currently selected search result.</summary>
	[Reactive] public partial SearchResultInfo? SelectedItem { get; set; } = null;

	/// <summary>Gets or sets whether a valid item is selected (enabling the Go button).</summary>
	[Reactive] public partial bool CanSelect { get; set; } = false;

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public GoToAllViewModel() : this(CpuType.Snes, GoToAllOptions.None) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="GoToAllViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type for address resolution.</param>
	/// <param name="options">Search options controlling what types of results to include.</param>
	/// <param name="symbolProvider">Optional external symbol provider for source-level symbols.</param>
	/// <remarks>
	/// Sets up reactive subscriptions to update search results as the user types
	/// and to track whether a valid selection can be made.
	/// </remarks>
	public GoToAllViewModel(CpuType cpuType, GoToAllOptions options, ISymbolProvider? symbolProvider = null) {
		// Update search results whenever the search string changes
		AddDisposable(this.WhenAnyValue(x => x.SearchString).Subscribe(x => {
			SearchResults = SearchHelper.GetGoToAllResults(cpuType, SearchString, options, symbolProvider);
			if (SearchResults.Count > 0) {
				SelectionModel.SelectedIndex = 0;
			}
		}));

		// Track whether a valid (non-disabled) item is selected
		AddDisposable(this.WhenAnyValue(x => x.SelectionModel.SelectedItem).Subscribe(item => CanSelect = item?.Disabled == false));
	}
}
