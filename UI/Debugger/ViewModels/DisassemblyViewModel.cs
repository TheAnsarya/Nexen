using System;
using System.Linq;
using System.Text;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Controls;
using Nexen.Debugger.Disassembly;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Views;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the disassembly view in the debugger.
/// </summary>
/// <remarks>
/// <para>
/// Provides the data and logic for displaying disassembled code, including:
/// <list type="bullet">
/// <item>Scrollable view of disassembled instructions</item>
/// <item>Selection handling (single row and range)</item>
/// <item>Navigation history (back/forward)</item>
/// <item>Quick search functionality</item>
/// <item>Active address tracking for program counter</item>
/// <item>Copy to clipboard support</item>
/// </list>
/// </para>
/// <para>
/// Implements <see cref="ISelectableModel"/> for row selection support.
/// </para>
/// </remarks>
public class DisassemblyViewModel : DisposableViewModel, ISelectableModel {
	/// <summary>Gets the code data provider for fetching disassembly lines.</summary>
	public ICodeDataProvider DataProvider { get; }

	/// <summary>Gets the CPU type being disassembled.</summary>
	public CpuType CpuType { get; }

	/// <summary>Gets the parent debugger window ViewModel.</summary>
	public DebuggerWindowViewModel Debugger { get; }

	/// <summary>Gets the style provider for syntax highlighting.</summary>
	public DisassemblyViewStyleProvider StyleProvider { get; }

	/// <summary>Gets or sets the current scroll position (0 to MaxScrollPosition).</summary>
	[Reactive] public int ScrollPosition { get; set; } = 0;

	/// <summary>Gets or sets the maximum scroll position.</summary>
	[Reactive] public int MaxScrollPosition { get; private set; } = 1000000000;

	/// <summary>Gets or sets the address at the top of the visible area.</summary>
	[Reactive] public int TopAddress { get; private set; } = 0;

	/// <summary>Gets or sets the currently visible disassembly lines.</summary>
	[Reactive] public CodeLineData[] Lines { get; private set; } = [];

	/// <summary>Gets or sets the active address (program counter), or null if not paused.</summary>
	[Reactive] public int? ActiveAddress { get; set; }

	/// <summary>Gets or sets the address of the currently selected row.</summary>
	[Reactive] public int SelectedRowAddress { get; set; }

	/// <summary>Gets or sets the anchor point for range selection.</summary>
	[Reactive] public int SelectionAnchor { get; set; }

	/// <summary>Gets or sets the start address of the current selection range.</summary>
	[Reactive] public int SelectionStart { get; set; }

	/// <summary>Gets or sets the end address of the current selection range.</summary>
	[Reactive] public int SelectionEnd { get; set; }

	/// <summary>Gets the quick search ViewModel for find functionality.</summary>
	public QuickSearchViewModel QuickSearch { get; } = new QuickSearchViewModel();

	/// <summary>Gets the navigation history for back/forward support.</summary>
	public NavigationHistory<int> History { get; } = new();

	/// <summary>Gets whether scroll bar markers should be shown (disabled for NEC DSP).</summary>
	public bool ShowScrollBarMarkers => CpuType != CpuType.NecDsp;

	/// <summary>Gets the debug configuration.</summary>
	public DebugConfig Config { get; private set; }

	/// <summary>Gets or sets the number of visible rows in the view.</summary>
	public int VisibleRowCount { get; set; } = 100;

	/// <summary>The disassembly viewer control reference.</summary>
	private DisassemblyViewer? _viewer = null;

	/// <summary>Counter to prevent recursive scroll updates.</summary>
	private int _ignoreScrollUpdates = 0;

	/// <summary>Callback to refresh the scrollbar after data changes.</summary>
	private Action? _refreshScrollbar = null;

	/// <summary>
	/// Initializes a new instance for the designer.
	/// </summary>
	[Obsolete("For designer only")]
	public DisassemblyViewModel() : this(new DebuggerWindowViewModel(), new DebugConfig(), CpuType.Snes) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="DisassemblyViewModel"/> class.
	/// </summary>
	/// <param name="debugger">The parent debugger window ViewModel.</param>
	/// <param name="config">The debug configuration.</param>
	/// <param name="cpuType">The CPU type to disassemble.</param>
	public DisassemblyViewModel(DebuggerWindowViewModel debugger, DebugConfig config, CpuType cpuType) {
		Config = config;
		CpuType = cpuType;
		Debugger = debugger;
		StyleProvider = new DisassemblyViewStyleProvider(cpuType, this);
		DataProvider = new CodeDataProvider(cpuType);

		if (Design.IsDesignMode) {
			return;
		}

		QuickSearch.OnFind += QuickSearch_OnFind;

		AddDisposable(this.WhenAnyValue(x => x.TopAddress).Subscribe(x => Refresh()));

		AddDisposable(this.WhenAnyValue(x => x.QuickSearch.IsSearchBoxVisible).Subscribe(x => {
			if (!QuickSearch.IsSearchBoxVisible) {
				_viewer?.Focus();
			}
		}));

		int lastValue = ScrollPosition;
		AddDisposable(this.WhenAnyValue(x => x.ScrollPosition).Subscribe(scrollPos => {
			if (_viewer == null) {
				ScrollPosition = lastValue;
				return;
			}

			int gap = scrollPos - lastValue;
			lastValue = scrollPos;
			if (_ignoreScrollUpdates > 0) {
				return;
			}

			if (gap != 0) {
				if (Math.Abs(gap) <= 10) {
					Scroll(gap);
				} else {
					int lineCount = DataProvider.GetLineCount();
					TopAddress = Math.Max(0, Math.Min(lineCount - 1, (int)((double)lineCount / MaxScrollPosition * ScrollPosition)));
				}
			}
		}));
	}

	/// <summary>
	/// Handles quick search find events.
	/// </summary>
	/// <param name="e">The find event arguments.</param>
	private void QuickSearch_OnFind(OnFindEventArgs e) {
		DisassemblySearchOptions options = new() { SearchBackwards = e.Direction == SearchDirection.Backward, SkipFirstLine = e.SkipCurrent };
		int findAddress = DebugApi.SearchDisassembly(CpuType, e.SearchString.ToLowerInvariant(), SelectedRowAddress, options);
		if (findAddress >= 0) {
			Dispatcher.UIThread.Post(() => SetSelectedRow(findAddress, true, true));
			e.Success = true;
		} else {
			e.Success = false;
		}
	}

	/// <summary>
	/// Sets the viewer control reference for focus management.
	/// </summary>
	/// <param name="viewer">The disassembly viewer control.</param>
	public void SetViewer(DisassemblyViewer? viewer) {
		_viewer = viewer;
	}

	/// <summary>
	/// Scrolls the view by the specified number of lines.
	/// </summary>
	/// <param name="lineNumberOffset">Number of lines to scroll (positive = down, negative = up).</param>
	public void Scroll(int lineNumberOffset) {
		if (lineNumberOffset == 0) {
			return;
		}

		SetTopAddress(DataProvider.GetRowAddress(TopAddress, lineNumberOffset));
	}

	/// <summary>
	/// Scrolls to the top of the disassembly.
	/// </summary>
	/// <param name="extendSelection">Whether to extend the current selection.</param>
	public void ScrollToTop(bool extendSelection) {
		if (extendSelection) {
			ResizeSelectionTo(0);
		} else {
			SetSelectedRow(0, false, true);
			ScrollToAddress(0, ScrollDisplayPosition.Top);
		}
	}

	/// <summary>
	/// Scrolls to the bottom of the disassembly.
	/// </summary>
	/// <param name="extendSelection">Whether to extend the current selection.</param>
	public void ScrollToBottom(bool extendSelection) {
		if (extendSelection) {
			ResizeSelectionTo(DataProvider.GetLineCount() - 1);
		} else {
			int address = DataProvider.GetLineCount() - 1;
			SetSelectedRow(address, false, true);
			ScrollToAddress((uint)address, ScrollDisplayPosition.Bottom);
		}
	}

	/// <summary>
	/// Forces the disassembly viewer to refresh by creating a new array reference.
	/// </summary>
	public void InvalidateVisual() {
		//Force DisassemblyViewer to refresh
		Lines = Lines.ToArray();
	}

	/// <summary>
	/// Refreshes the visible disassembly lines from the data provider.
	/// </summary>
	public void Refresh() {
		CodeLineData[] lines = DataProvider.GetCodeLines(TopAddress, VisibleRowCount);
		Lines = lines;

		if (lines.Length > 0 && lines[0].Address >= 0) {
			SetTopAddress(lines[0].Address);
		}

		_refreshScrollbar?.Invoke();
	}

	public void SetRefreshScrollBar(Action? refreshScrollbar) {
		_refreshScrollbar = refreshScrollbar;
	}

	/// <summary>
	/// Sets the top address while updating scroll position proportionally.
	/// </summary>
	/// <param name="address">The address to set as the top visible row.</param>
	private void SetTopAddress(int address) {
		int lineCount = DataProvider.GetLineCount();
		address = Math.Max(0, Math.Min(lineCount - 1, address));

		_ignoreScrollUpdates++;
		TopAddress = address;
		ScrollPosition = (int)(TopAddress / (double)lineCount * MaxScrollPosition);
		_ignoreScrollUpdates--;
	}

	/// <summary>
	/// Sets the active address (program counter) and scrolls to it.
	/// </summary>
	/// <param name="pc">The program counter address, or null to clear.</param>
	/// <remarks>
	/// Adds the navigation to history for back/forward support.
	/// </remarks>
	public void SetActiveAddress(int? pc) {
		ActiveAddress = pc;
		if (pc != null) {
			int currentAddress = SelectedRowAddress;
			SetSelectedRow((int)pc);
			bool scrolled = ScrollToAddress((uint)pc, ScrollDisplayPosition.Center, Config.Debugger.KeepActiveStatementInCenter);
			if (scrolled) {
				if (currentAddress != 0 || History.CanGoBack()) {
					History.AddHistory(currentAddress);
				}

				History.AddHistory((int)pc);
			}
		}
	}

	/// <summary>
	/// Checks if an address is within the current selection range.
	/// </summary>
	/// <param name="address">The address to check.</param>
	/// <returns>True if the address is selected.</returns>
	public bool IsSelected(int address) {
		return address >= SelectionStart && address <= SelectionEnd;
	}

	/// <summary>
	/// Gets the currently selected row address as an AddressInfo.
	/// </summary>
	/// <returns>The address info for the selected row.</returns>
	public AddressInfo? GetSelectedRowAddress() {
		return new AddressInfo() {
			Address = SelectedRowAddress,
			Type = CpuType.ToMemoryType()
		};
	}

	/// <summary>
	/// Navigates back in the history.
	/// </summary>
	public void GoBack() {
		SetSelectedRow(History.GoBack(), true, false);
	}

	/// <summary>
	/// Navigates forward in the history.
	/// </summary>
	public void GoForward() {
		SetSelectedRow(History.GoForward(), true, false);
	}

	/// <summary>
	/// Sets the selected row to the specified address.
	/// </summary>
	/// <param name="address">The address to select.</param>
	public void SetSelectedRow(int address) {
		SetSelectedRow(address, false);
	}

	/// <summary>
	/// Sets the selected row with optional scrolling and history tracking.
	/// </summary>
	/// <param name="address">The address to select.</param>
	/// <param name="scrollToRow">Whether to scroll the view to show the selected row.</param>
	/// <param name="addToHistory">Whether to add this navigation to history.</param>
	public void SetSelectedRow(int address, bool scrollToRow = false, bool addToHistory = false) {
		int currentAddress = SelectedRowAddress;
		SelectionStart = address;
		SelectionEnd = address;
		SelectedRowAddress = address;
		SelectionAnchor = address;

		if (addToHistory) {
			if (currentAddress != 0 || History.CanGoBack()) {
				History.AddHistory(currentAddress);
			}

			History.AddHistory(address);
		}

		InvalidateVisual();

		if (scrollToRow) {
			ScrollToAddress((uint)address);
		}
	}

	/// <summary>
	/// Moves the cursor by the specified row offset.
	/// </summary>
	/// <param name="rowOffset">Number of rows to move (positive = down, negative = up).</param>
	/// <param name="extendSelection">Whether to extend the selection rather than move it.</param>
	public void MoveCursor(int rowOffset, bool extendSelection) {
		int address = DataProvider.GetRowAddress(SelectedRowAddress, rowOffset);
		if (extendSelection) {
			ResizeSelectionTo(address);
		} else {
			SetSelectedRow(address);
			ScrollToAddress((uint)address, rowOffset < 0 ? ScrollDisplayPosition.Top : ScrollDisplayPosition.Bottom);
		}
	}

	/// <summary>
	/// Extends the selection to include the specified address.
	/// </summary>
	/// <param name="address">The address to extend selection to.</param>
	public void ResizeSelectionTo(int address) {
		if (SelectedRowAddress == address) {
			return;
		}

		bool anchorTop = SelectionAnchor == SelectionStart;
		if (anchorTop) {
			if (address < SelectionStart) {
				SelectionEnd = SelectionStart;
				SelectionStart = address;
			} else {
				SelectionEnd = address;
			}
		} else {
			if (address < SelectionEnd) {
				SelectionStart = address;
			} else {
				SelectionStart = SelectionEnd;
				SelectionEnd = address;
			}
		}

		ScrollDisplayPosition displayPos = SelectedRowAddress < address ? ScrollDisplayPosition.Bottom : ScrollDisplayPosition.Top;
		SelectedRowAddress = address;
		ScrollToAddress((uint)address, displayPos);

		InvalidateVisual();
	}

	/// <summary>
	/// Checks if an address is currently visible in the view.
	/// </summary>
	/// <param name="address">The address to check.</param>
	/// <returns>True if the address is visible (not in the first or last row).</returns>
	private bool IsAddressVisible(int address) {
		for (int i = 1; i < VisibleRowCount - 2 && i < Lines.Length; i++) {
			if (Lines[i].Address == address) {
				return true;
			}
		}

		return false;
	}

	/// <summary>
	/// Scrolls the view to show the specified address.
	/// </summary>
	/// <param name="pc">The address to scroll to.</param>
	/// <param name="position">Where to position the address in the view.</param>
	/// <param name="forceScroll">Whether to scroll even if the address is already visible.</param>
	/// <returns>True if scrolling occurred.</returns>
	private bool ScrollToAddress(uint pc, ScrollDisplayPosition position = ScrollDisplayPosition.Center, bool forceScroll = false) {
		if (!forceScroll && IsAddressVisible((int)pc)) {
			//Row is already visible, don't scroll
			return false;
		}

		ICodeDataProvider dp = DataProvider;

		switch (position) {
			case ScrollDisplayPosition.Top: TopAddress = dp.GetRowAddress((int)pc, -1); break;
			case ScrollDisplayPosition.Center: TopAddress = dp.GetRowAddress((int)pc, (-VisibleRowCount / 2) + 1); break;
			case ScrollDisplayPosition.Bottom: TopAddress = dp.GetRowAddress((int)pc, -VisibleRowCount + 2); break;
		}

		if (!IsAddressVisible((int)pc)) {
			TopAddress = dp.GetRowAddress(TopAddress, TopAddress < pc ? 1 : -1);
		}

		return true;
	}

	/// <summary>
	/// Copies the current selection to the clipboard.
	/// </summary>
	public void CopySelection() {
		DebuggerConfig cfg = Config.Debugger;
		string code = GetSelection(cfg.CopyAddresses, cfg.CopyByteCode, cfg.CopyComments, cfg.CopyBlockHeaders, out _, false);
		ApplicationHelper.GetMainWindow()?.Clipboard?.SetTextAsync(code);
	}

	/// <summary>
	/// Gets the text content of the current selection.
	/// </summary>
	/// <param name="getAddresses">Whether to include addresses in the output.</param>
	/// <param name="getByteCode">Whether to include byte code in the output.</param>
	/// <param name="getComments">Whether to include comments in the output.</param>
	/// <param name="getHeaders">Whether to include block headers in the output.</param>
	/// <param name="byteCount">Output: the total number of bytes in the selection.</param>
	/// <param name="skipGeneratedJmpSubLabels">Whether to skip auto-generated jump/sub labels.</param>
	/// <returns>The formatted text of the selection.</returns>
	public string GetSelection(bool getAddresses, bool getByteCode, bool getComments, bool getHeaders, out int byteCount, bool skipGeneratedJmpSubLabels) {
		ICodeDataProvider dp = DataProvider;

		const int commentSpacingCharCount = 25;

		int addrSize = dp.CpuType.GetAddressSize();
		string addrFormat = "X" + addrSize;
		StringBuilder sb = new StringBuilder();
		int i = SelectionStart;
		int endAddress = 0;
		CodeLineData? prevLine = null;
		AddressDisplayType addressDisplayType = Config.Debugger.AddressDisplayType;
		do {
			CodeLineData[] data = dp.GetCodeLines(i, 5000);
			for (int j = 0; j < data.Length; j++) {
				CodeLineData lineData = data[j];
				if (prevLine?.Address == lineData.Address && prevLine?.Text == lineData.Text) {
					continue;
				}

				if (lineData.Address > SelectionEnd) {
					i = lineData.Address;
					break;
				}

				string codeString = lineData.Text.Trim();
				if (lineData.Flags.HasFlag(LineFlags.BlockEnd) || lineData.Flags.HasFlag(LineFlags.BlockStart)) {
					if (getHeaders) {
						codeString = "--------" + codeString + "--------";
					} else {
						if (j == data.Length - 1) {
							i = lineData.Address;
						}

						continue;
					}
				}

				prevLine = lineData;

				int padding = Math.Max(commentSpacingCharCount, codeString.Length);
				if (codeString.Length == 0) {
					padding = 0;
				}

				codeString = codeString.PadRight(padding);

				bool indentText = !(lineData.Flags.HasFlag(LineFlags.ShowAsData) || lineData.Flags.HasFlag(LineFlags.BlockStart) || lineData.Flags.HasFlag(LineFlags.BlockEnd) || lineData.Flags.HasFlag(LineFlags.Label) || (lineData.Flags.HasFlag(LineFlags.Comment) && lineData.Text.Length == 0));
				string line = (indentText ? "  " : "") + codeString;
				if (getByteCode) {
					line = lineData.ByteCodeStr.PadRight(13) + line;
				}

				if (getAddresses) {
					string addressText = lineData.GetAddressText(addressDisplayType, addrFormat);
					line = addressText.PadRight(addrSize) + "  " + line;
				}

				if (getComments && !string.IsNullOrWhiteSpace(lineData.Comment)) {
					line += lineData.Comment;
				}

				//Skip lines that contain a jump/sub "label" (these aren't
				bool skipLine = skipGeneratedJmpSubLabels && lineData.Flags.HasFlag(LineFlags.Label) && lineData.Text.StartsWith("$");

				string result = line.TrimEnd();
				if (!skipLine && result.Length > 0) {
					sb.AppendLine(result);
				}

				i = lineData.Address;
				endAddress = lineData.Address + lineData.OpSize - 1;
			}
		} while (i < SelectionEnd);

		byteCount = SelectionStart <= endAddress ? endAddress - SelectionStart + 1 : 0;

		return sb.ToString();
	}
}

/// <summary>
/// Specifies where to position an address when scrolling to it.
/// </summary>
public enum ScrollDisplayPosition {
	/// <summary>Position the address at the top of the visible area.</summary>
	Top,
	/// <summary>Position the address in the center of the visible area.</summary>
	Center,
	/// <summary>Position the address at the bottom of the visible area.</summary>
	Bottom
}
