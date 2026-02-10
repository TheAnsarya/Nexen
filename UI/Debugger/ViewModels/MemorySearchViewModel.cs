using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reactive.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Controls.Selection;
using Avalonia.Threading;
using Nexen.Controls.DataGridExtensions;
using DynamicData;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;

/// <summary>
/// ViewModel for the memory search window (cheat search functionality).
/// </summary>
/// <remarks>
/// <para>
/// Provides iterative memory searching similar to cheat finders, allowing users to:
/// <list type="bullet">
/// <item>Search for values that changed, stayed the same, increased, decreased, etc.</item>
/// <item>Compare against previous search results or specific values</item>
/// <item>Filter results progressively to narrow down addresses</item>
/// <item>View memory access statistics (read/write/execute counts)</item>
/// <item>Support multiple value sizes (byte, word, dword) and formats (hex, signed, unsigned)</item>
/// </list>
/// </para>
/// <para>
/// Maintains search history for undo functionality and supports sorting by various columns.
/// </para>
/// </remarks>
public sealed class MemorySearchViewModel : DisposableViewModel {
	/// <summary>Gets the memory search configuration.</summary>
	public MemorySearchConfig Config { get; }

	/// <summary>Gets or sets the available memory types for the current console.</summary>
	[Reactive] public Enum[] AvailableMemoryTypes { get; set; } = [];

	/// <summary>Gets or sets the currently selected memory type to search.</summary>
	[Reactive] public MemoryType MemoryType { get; set; } = MemoryType.SnesMemory;

	/// <summary>Gets or sets the display format for values.</summary>
	[Reactive] public MemorySearchFormat Format { get; set; } = MemorySearchFormat.Hex;

	/// <summary>Gets or sets the size of values to search for.</summary>
	[Reactive] public MemorySearchValueSize ValueSize { get; set; } = MemorySearchValueSize.Byte;

	/// <summary>Gets or sets what to compare values against.</summary>
	[Reactive] public MemorySearchCompareTo CompareTo { get; set; } = MemorySearchCompareTo.PreviousRefreshValue;

	/// <summary>Gets or sets the comparison operator.</summary>
	[Reactive] public MemorySearchOperator Operator { get; set; } = MemorySearchOperator.Equal;

	/// <summary>Gets or sets the list of address ViewModels for display.</summary>
	[Reactive] public NexenList<MemoryAddressViewModel> ListData { get; private set; } = new();

	/// <summary>Gets or sets the selection model for the list.</summary>
	[Reactive] public SelectionModel<MemoryAddressViewModel> Selection { get; set; } = new();

	/// <summary>Gets or sets the current sort state.</summary>
	[Reactive] public SortState SortState { get; set; } = new();

	/// <summary>Gets the column widths from configuration.</summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.MemorySearch.ColumnWidths;

	/// <summary>Gets or sets the specific address to compare against.</summary>
	[Reactive] public int SpecificAddress { get; set; } = 0;

	/// <summary>Gets or sets the specific value to compare against.</summary>
	[Reactive] public int SpecificValue { get; set; } = 0;

	/// <summary>Gets or sets whether values are displayed in hex format.</summary>
	[Reactive] public bool IsValueHex { get; set; }

	/// <summary>Gets or sets the minimum valid value for the current format/size.</summary>
	[Reactive] public string MinValue { get; set; } = "";

	/// <summary>Gets or sets the maximum valid value for the current format/size.</summary>
	[Reactive] public string MaxValue { get; set; } = "";

	/// <summary>Gets or sets the maximum address in the current memory region.</summary>
	[Reactive] public int MaxAddress { get; set; } = 0;

	/// <summary>Gets or sets whether undo is available.</summary>
	[Reactive] public bool IsUndoEnabled { get; set; } = false;

	/// <summary>Gets or sets whether the specific value field is enabled.</summary>
	[Reactive] public bool IsSpecificValueEnabled { get; set; } = false;

	/// <summary>Gets or sets whether the specific address field is enabled.</summary>
	[Reactive] public bool IsSpecificAddressEnabled { get; set; } = false;

	/// <summary>Gets the lookup table mapping visible row indices to actual addresses.</summary>
	public int[] AddressLookup { get; private set; } = [];

	/// <summary>Gets the current memory state snapshot.</summary>
	public byte[] MemoryState { get; private set; } = [];

	/// <summary>Gets the previous memory state snapshot for comparison.</summary>
	public byte[] PrevMemoryState { get; private set; } = [];

	/// <summary>Internal list of all address ViewModels.</summary>
	private List<MemoryAddressViewModel> _innerData = new();

	/// <summary>The memory snapshot from the last search operation.</summary>
	private byte[] _lastSearchSnapshot = [];

	/// <summary>Set of addresses that have been filtered out.</summary>
	private HashSet<int> _hiddenAddresses = new();

	/// <summary>History stack for undo functionality.</summary>
	private List<SearchHistory> _undoHistory = new();

	/// <summary>Flag to prevent recursive refreshes.</summary>
	private bool _isRefreshing;

	/// <summary>
	/// Initializes a new instance of the <see cref="MemorySearchViewModel"/> class.
	/// </summary>
	public MemorySearchViewModel() {
		Config = ConfigManager.Config.Debug.MemorySearch;

		if (Design.IsDesignMode) {
			return;
		}

		OnGameLoaded();
		SortState.SetColumnSort("Address", ListSortDirection.Ascending, false);

		AddDisposable(this.WhenAnyValue(x => x.MemoryType).Subscribe(x => ResetSearch()));

		AddDisposable(this.WhenAnyValue(x => x.Operator, x => x.CompareTo, x => x.ValueSize, x => x.Format).Subscribe(x => {
			IsSpecificValueEnabled = CompareTo == MemorySearchCompareTo.SpecificValue;
			IsSpecificAddressEnabled = CompareTo == MemorySearchCompareTo.SpecificAddress;

			IsValueHex = Format == MemorySearchFormat.Hex;
			bool isSigned = Format == MemorySearchFormat.Signed;
			switch (ValueSize) {
				case MemorySearchValueSize.Byte:
					MinValue = isSigned ? sbyte.MinValue.ToString() : byte.MinValue.ToString();
					MaxValue = isSigned ? sbyte.MaxValue.ToString() : byte.MaxValue.ToString();
					break;

				case MemorySearchValueSize.Word:
					MinValue = isSigned ? Int16.MinValue.ToString() : UInt16.MinValue.ToString();
					MaxValue = isSigned ? Int16.MaxValue.ToString() : UInt16.MaxValue.ToString();
					break;

				case MemorySearchValueSize.Dword:
					MinValue = isSigned ? Int32.MinValue.ToString() : UInt32.MinValue.ToString();
					MaxValue = isSigned ? Int32.MaxValue.ToString() : UInt32.MaxValue.ToString();
					break;
			}

			RefreshList(true);
		}));

		AddDisposable(this.WhenAnyValue(x => x.SpecificValue, x => x.SpecificAddress).Subscribe(x => RefreshList(false)));
	}

	/// <summary>
	/// Handles the sort command by refreshing the list with the current sort order.
	/// </summary>
	public void SortCommand() {
		RefreshList(true);
	}

	/// <summary>
	/// Updates available memory types when a game is loaded.
	/// </summary>
	public void OnGameLoaded() {
		AvailableMemoryTypes = Enum.GetValues<MemoryType>().Where(t => t.SupportsMemoryViewer() && !t.IsRelativeMemory() && DebugApi.GetMemorySize(t) > 0).Cast<Enum>().ToArray();
		MemoryType = EmuApi.GetRomInfo().ConsoleType.GetMainCpuType().GetSystemRamType();
	}

	/// <summary>
	/// Refreshes the memory state data from the emulator.
	/// </summary>
	/// <param name="forceSort">Whether to force a re-sort of the data.</param>
	/// <remarks>
	/// Saves the previous memory state before fetching the current state.
	/// This enables comparison between current and previous values.
	/// </remarks>
	public void RefreshData(bool forceSort) {
		PrevMemoryState = MemoryState;
		MemoryState = DebugApi.GetMemoryState(MemoryType);

		Dispatcher.UIThread.Post(() => RefreshList(forceSort));
	}

	/// <summary>
	/// Refreshes the UI list with current memory data.
	/// </summary>
	/// <param name="forceSort">Whether to force a re-sort of the data.</param>
	/// <remarks>
	/// Runs sorting and filtering on a background thread to keep the UI responsive.
	/// Uses a flag to prevent concurrent refresh operations.
	/// </remarks>
	private void RefreshList(bool forceSort) {
		byte[] memoryState = MemoryState;
		byte[] prevMemoryState = PrevMemoryState;

		if (memoryState.Length != prevMemoryState.Length) {
			//Mismatching arrays - happens when changing game or mem type
			return;
		}

		if (_isRefreshing) {
			return;
		}

		_isRefreshing = true;
		List<Tuple<string, ListSortDirection>> sortOrder = new(SortState.SortOrder);

		Task.Run(() => {
			bool isDefaultSort = sortOrder.Count == 1 && sortOrder[0].Item2 == ListSortDirection.Ascending && sortOrder[0].Item1 == "Address";
			if (!isDefaultSort || forceSort) {
				Sort(sortOrder);
			}

			Dispatcher.UIThread.Post(() => RefreshUiList(memoryState));
		});
	}

	/// <summary>
	/// Updates the UI list with current memory values.
	/// </summary>
	/// <param name="memoryState">The memory state to display.</param>
	/// <remarks>
	/// Dynamically adjusts the list size to match memory size.
	/// Updates all visible address view models with current values.
	/// </remarks>
	private void RefreshUiList(byte[] memoryState) {
		if (Disposed) {
			return;
		}

		MemoryType memType = MemoryType;

		if (_innerData.Count < memoryState.Length) {
			_innerData.AddRange(Enumerable.Range(_innerData.Count, memoryState.Length - _innerData.Count).Select(i => new MemoryAddressViewModel(i, this)));
		} else if (_innerData.Count > memoryState.Length) {
			_innerData.RemoveRange(memoryState.Length, _innerData.Count - memoryState.Length);
		}

		int visibleCount = memoryState.Length - _hiddenAddresses.Count;
		if (ListData.Count != visibleCount) {
			ListData.Replace(_innerData.GetRange(0, visibleCount));
		}

		List<MemoryAddressViewModel> list = ListData.GetInnerList();
		for (int i = 0, len = list.Count; i < len; i++) {
			list[i].Update();
		}

		_isRefreshing = false;
	}

	/// <summary>
	/// Sorts the address lookup array based on the specified sort order.
	/// </summary>
	/// <param name="sortOrder">List of column names and sort directions.</param>
	/// <remarks>
	/// Supports sorting by address, value, previous value, and memory access counters.
	/// Only fetches access counters from the debug API when sorting by counter columns.
	/// </remarks>
	public void Sort(List<Tuple<string, ListSortDirection>> sortOrder) {
		byte[] mem = MemoryState;
		byte[] prevMem = PrevMemoryState;

		AddressCounters[] counters = [];
		foreach ((string column, ListSortDirection order) in SortState.SortOrder) {
			if (column.Contains("Read") || column.Contains("Write") || column.Contains("Exec")) {
				//Only get counters if user sorted on a counter column
				counters = DebugApi.GetMemoryAccessCounts(MemoryType);
				break;
			}
		}

		Dictionary<string, Func<int, int, int>> comparers = new() {
			{ "Address", (a, b) => a.CompareTo(b) },
			{ "Value", (a, b) => GetValue(a, mem).CompareTo(GetValue(b, mem)) },
			{ "PrevValue", (a, b) => GetValue(a, prevMem).CompareTo(GetValue(b, prevMem)) },
			{ "ReadCount", (a, b) => counters[a].ReadCounter.CompareTo(counters[b].ReadCounter) },
			{ "LastRead", (a, b) => -counters[a].ReadStamp.CompareTo(counters[b].ReadStamp) },
			{ "WriteCount", (a, b) => counters[a].WriteCounter.CompareTo(counters[b].WriteCounter) },
			{ "LastWrite", (a, b) => -counters[a].WriteStamp.CompareTo(counters[b].WriteStamp) },
			{ "ExecCount", (a, b) => counters[a].ExecCounter.CompareTo(counters[b].ExecCounter) },
			{ "LastExec", (a, b) => -counters[a].ExecStamp.CompareTo(counters[b].ExecStamp) },
		};

		SortHelper.SortArray(AddressLookup, sortOrder, comparers, "Address");
	}

	/// <summary>
	/// Gets the value at the specified address from a memory state array.
	/// </summary>
	/// <param name="address">The memory address to read from.</param>
	/// <param name="memoryState">The memory state array to read from.</param>
	/// <returns>The value at the address, formatted according to current ValueSize and Format settings.</returns>
	/// <remarks>
	/// Reads 1, 2, or 4 bytes based on ValueSize.
	/// Returns signed or unsigned values based on Format.
	/// </remarks>
	public long GetValue(int address, byte[] memoryState) {
		uint value = 0;
		for (int i = 0; i < (int)ValueSize && address + i < memoryState.Length; i++) {
			value |= (uint)memoryState[address + i] << (i * 8);
		}

		switch (Format) {
			default:
			case MemorySearchFormat.Hex: {
					return value;
				}

			case MemorySearchFormat.Signed:
				switch (ValueSize) {
					default:
					case MemorySearchValueSize.Byte: return (sbyte)value;
					case MemorySearchValueSize.Word: return (short)value;
					case MemorySearchValueSize.Dword: return (int)value;
				}

			case MemorySearchFormat.Unsigned:
				switch (ValueSize) {
					default:
					case MemorySearchValueSize.Byte: return (byte)value;
					case MemorySearchValueSize.Word: return (ushort)value;
					case MemorySearchValueSize.Dword: return (uint)value;
				}
		}
	}

	/// <summary>
	/// Determines if the value at the specified address matches the current search criteria.
	/// </summary>
	/// <param name="address">The memory address to check.</param>
	/// <returns>True if the address value matches the search criteria; otherwise false.</returns>
	/// <remarks>
	/// Compares the current value against the comparison target using the selected operator.
	/// Comparison targets can be previous search value, previous refresh value, specific address, or specific value.
	/// </remarks>
	public bool IsMatch(int address) {
		long value = GetValue(address, MemoryState);

		long compareValue = CompareTo switch {
			MemorySearchCompareTo.PreviousSearchValue => GetValue(address, _lastSearchSnapshot),
			MemorySearchCompareTo.PreviousRefreshValue => GetValue(address, PrevMemoryState),
			MemorySearchCompareTo.SpecificAddress => GetValue(SpecificAddress, MemoryState),
			MemorySearchCompareTo.SpecificValue => SpecificValue,
			_ => throw new Exception("Unsupported compare type")
		};

		return Operator switch {
			MemorySearchOperator.Equal => value == compareValue,
			MemorySearchOperator.NotEqual => value != compareValue,
			MemorySearchOperator.LessThan => value < compareValue,
			MemorySearchOperator.LessThanOrEqual => value <= compareValue,
			MemorySearchOperator.GreaterThan => value > compareValue,
			MemorySearchOperator.GreaterThanOrEqual => value >= compareValue,
			_ => throw new Exception("Unsupported operator")
		};
	}

	/// <summary>
	/// Adds a filter based on the current search criteria, hiding non-matching addresses.
	/// </summary>
	/// <remarks>
	/// Saves current state to undo history before filtering.
	/// Addresses that don't match the criteria are added to the hidden set.
	/// Takes a new snapshot after filtering for subsequent comparisons.
	/// </remarks>
	public void AddFilter() {
		_undoHistory.Add(new SearchHistory(_hiddenAddresses, _lastSearchSnapshot));
		IsUndoEnabled = true;

		int count = _lastSearchSnapshot.Length;
		for (int i = 0; i < count; i++) {
			if (_hiddenAddresses.Contains(i)) {
				continue;
			} else if (!IsMatch(i)) {
				_hiddenAddresses.Add(i);
			}
		}

		UpdateAddressLookup();

		_lastSearchSnapshot = DebugApi.GetMemoryState(MemoryType);
		RefreshList(true);
	}

	/// <summary>
	/// Rebuilds the address lookup array excluding hidden addresses.
	/// </summary>
	/// <remarks>
	/// Creates a contiguous array of visible addresses for display.
	/// Called after filtering operations to update the visible address list.
	/// </remarks>
	private void UpdateAddressLookup() {
		int count = _lastSearchSnapshot.Length;
		int[] addressLookup = new int[count];
		int visibleRowCounter = 0;

		for (int i = 0; i < count; i++) {
			if (!_hiddenAddresses.Contains(i)) {
				addressLookup[visibleRowCounter] = i;
				visibleRowCounter++;
			}
		}

		Array.Resize(ref addressLookup, visibleRowCounter);
		AddressLookup = addressLookup;
	}

	/// <summary>
	/// Resets the search to show all addresses with fresh memory snapshots.
	/// </summary>
	/// <remarks>
	/// Clears all hidden addresses and undo history.
	/// Takes a new memory snapshot as the baseline for comparisons.
	/// </remarks>
	public void ResetSearch() {
		_lastSearchSnapshot = DebugApi.GetMemoryState(MemoryType);
		AddressLookup = Enumerable.Range(0, _lastSearchSnapshot.Length).ToArray();
		MaxAddress = Math.Max(0, _lastSearchSnapshot.Length - 1);
		_hiddenAddresses.Clear();
		_undoHistory.Clear();
		IsUndoEnabled = false;
		MemoryState = DebugApi.GetMemoryState(MemoryType);
		RefreshData(true);
	}

	/// <summary>
	/// Undoes the last filter operation, restoring the previous search state.
	/// </summary>
	/// <remarks>
	/// Restores hidden addresses and search snapshot from the undo history.
	/// Disables undo when the history is empty.
	/// </remarks>
	public void Undo() {
		if (_undoHistory.Count > 0) {
			_hiddenAddresses = _undoHistory[^1].HiddenAddresses;
			_lastSearchSnapshot = _undoHistory[^1].SearchSnapshot;
			_undoHistory.RemoveAt(_undoHistory.Count - 1);
			IsUndoEnabled = _undoHistory.Count > 0;
			UpdateAddressLookup();
			RefreshList(true);
		}
	}

	/// <summary>
	/// Resets all memory access counters and refreshes the display.
	/// </summary>
	public void ResetCounters() {
		DebugApi.ResetMemoryAccessCounts();
		RefreshList(true);
	}

	/// <summary>
	/// Stores the state of a search for undo functionality.
	/// </summary>
	private class SearchHistory {
		/// <summary>
		/// Set of addresses hidden by the filter.
		/// </summary>
		public HashSet<int> HiddenAddresses;

		/// <summary>
		/// Memory snapshot at the time of the filter.
		/// </summary>
		public byte[] SearchSnapshot;

		/// <summary>
		/// Creates a new search history entry.
		/// </summary>
		/// <param name="hiddenAddresses">The set of hidden addresses to copy.</param>
		/// <param name="searchSnapshot">The memory snapshot to store.</param>
		public SearchHistory(HashSet<int> hiddenAddresses, byte[] searchSnapshot) {
			HiddenAddresses = new(hiddenAddresses);
			SearchSnapshot = searchSnapshot;
		}
	}
}

/// <summary>
/// View model representing a single memory address in the search results list.
/// </summary>
/// <remarks>
/// Displays current value, previous value, and memory access statistics for a single address.
/// Uses property change notification to efficiently update only changed fields.
/// </remarks>
public sealed class MemoryAddressViewModel : INotifyPropertyChanged {
	/// <summary>
	/// Event raised when a property value changes.
	/// </summary>
	public event PropertyChangedEventHandler? PropertyChanged;

	/// <summary>
	/// Gets the hexadecimal string representation of the memory address.
	/// </summary>
	/// <remarks>
	/// Lazily updates fields when accessed to ensure current data.
	/// </remarks>
	public string AddressString {
		get {
			UpdateFields();
			return field;
		}

		private set;
	} = "";

	/// <summary>
	/// Gets or sets the current value at this address, formatted according to search settings.
	/// </summary>
	public string Value { get; set; } = "";

	/// <summary>
	/// Gets or sets the previous value at this address, formatted according to search settings.
	/// </summary>
	public string PrevValue { get; set; } = "";

	/// <summary>
	/// Gets or sets the number of times this address has been read.
	/// </summary>
	public string ReadCount { get; set; } = "";

	/// <summary>
	/// Gets or sets the number of times this address has been written.
	/// </summary>
	public string WriteCount { get; set; } = "";

	/// <summary>
	/// Gets or sets the number of times this address has been executed.
	/// </summary>
	public string ExecCount { get; set; } = "";

	/// <summary>
	/// Gets or sets the time since the last read operation at this address.
	/// </summary>
	public string LastRead { get; set; } = "";

	/// <summary>
	/// Gets or sets the time since the last write operation at this address.
	/// </summary>
	public string LastWrite { get; set; } = "";

	/// <summary>
	/// Gets or sets the time since the last execution at this address.
	/// </summary>
	public string LastExec { get; set; } = "";

	/// <summary>
	/// Gets or sets whether this address matches the current search criteria.
	/// </summary>
	public bool IsMatch { get; set; } = true;

	/// <summary>
	/// The index of this view model in the list.
	/// </summary>
	private int _index;

	/// <summary>
	/// Reference to the parent search view model for accessing memory state and settings.
	/// </summary>
	private MemorySearchViewModel _search;

	/// <summary>
	/// Creates a new memory address view model.
	/// </summary>
	/// <param name="index">The index in the address lookup array.</param>
	/// <param name="memorySearch">The parent search view model.</param>
	public MemoryAddressViewModel(int index, MemorySearchViewModel memorySearch) {
		_index = index;
		_search = memorySearch;
	}

	/// <summary>
	/// Pre-created property changed event args for efficient notification.
	/// </summary>
	static private PropertyChangedEventArgs[] _args = new[] {
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.AddressString)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.Value)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.PrevValue)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.ReadCount)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.WriteCount)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.ExecCount)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.LastRead)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.LastWrite)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.LastExec)),
			new PropertyChangedEventArgs(nameof(MemoryAddressViewModel.IsMatch))
		};

	/// <summary>
	/// Notifies the UI that all properties have changed and need to be refreshed.
	/// </summary>
	/// <remarks>
	/// Uses pre-created event args for efficiency as this is called frequently during list updates.
	/// Marked with AggressiveInlining for performance in hot paths.
	/// </remarks>
	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public void Update() {
		if (PropertyChanged is not null) {
			PropertyChanged(this, _args[0]);
			PropertyChanged(this, _args[1]);
			PropertyChanged(this, _args[2]);
			PropertyChanged(this, _args[3]);
			PropertyChanged(this, _args[4]);
			PropertyChanged(this, _args[5]);
			PropertyChanged(this, _args[6]);
			PropertyChanged(this, _args[7]);
			PropertyChanged(this, _args[8]);
			PropertyChanged(this, _args[9]);
		}
	}

	/// <summary>
	/// Updates all display fields with current values from memory state.
	/// </summary>
	/// <remarks>
	/// Reads the actual memory address from the address lookup using this view model's index.
	/// Formats values according to the search format (hex, signed, or unsigned).
	/// Fetches memory access counters from the debug API.
	/// </remarks>
	private void UpdateFields() {
		if (_index >= _search.AddressLookup.Length) {
			return;
		}

		int address = _search.AddressLookup[_index];
		AddressString = address.ToString("X4");

		uint value = 0;
		uint prevValue = 0;
		for (int i = 0; i < (int)_search.ValueSize && address + i < _search.MemoryState.Length; i++) {
			value |= (uint)_search.MemoryState[address + i] << (i * 8);
			prevValue |= (uint)_search.PrevMemoryState[address + i] << (i * 8);
		}

		switch (_search.Format) {
			case MemorySearchFormat.Hex: {
					string format = "X" + (((int)_search.ValueSize) * 2);
					Value = value.ToString(format);
					PrevValue = prevValue.ToString(format);
					break;
				}

			case MemorySearchFormat.Signed: {
					switch (_search.ValueSize) {
						case MemorySearchValueSize.Byte:
							Value = ((sbyte)value).ToString();
							PrevValue = ((sbyte)prevValue).ToString();
							break;
						case MemorySearchValueSize.Word:
							Value = ((short)value).ToString();
							PrevValue = ((short)prevValue).ToString();
							break;
						case MemorySearchValueSize.Dword:
							Value = ((int)value).ToString();
							PrevValue = ((int)prevValue).ToString();
							break;
					}

					break;
				}

			case MemorySearchFormat.Unsigned: {
					switch (_search.ValueSize) {
						case MemorySearchValueSize.Byte:
							Value = ((byte)value).ToString();
							PrevValue = ((byte)prevValue).ToString();
							break;
						case MemorySearchValueSize.Word:
							Value = ((ushort)value).ToString();
							PrevValue = ((ushort)prevValue).ToString();
							break;
						case MemorySearchValueSize.Dword:
							Value = ((uint)value).ToString();
							PrevValue = ((uint)prevValue).ToString();
							break;
					}

					break;
				}
		}

		AddressCounters counters = DebugApi.GetMemoryAccessCounts((uint)address, 1, _search.MemoryType)[0];
		UInt64 masterClock = EmuApi.GetTimingInfo(_search.MemoryType.ToCpuType()).MasterClock;

		ReadCount = CodeTooltipHelper.FormatCount(counters.ReadCounter);
		WriteCount = CodeTooltipHelper.FormatCount(counters.WriteCounter);
		ExecCount = CodeTooltipHelper.FormatCount(counters.ExecCounter);

		LastRead = CodeTooltipHelper.FormatCount(masterClock - counters.ReadStamp, counters.ReadStamp);
		LastWrite = CodeTooltipHelper.FormatCount(masterClock - counters.WriteStamp, counters.WriteStamp);
		LastExec = CodeTooltipHelper.FormatCount(masterClock - counters.ExecStamp, counters.ExecStamp);

		IsMatch = _search.IsMatch(address);
	}
}

/// <summary>
/// Specifies how values should be displayed in the memory search.
/// </summary>
public enum MemorySearchFormat {
	/// <summary>
	/// Display values in hexadecimal format.
	/// </summary>
	Hex,

	/// <summary>
	/// Display values as signed decimal integers.
	/// </summary>
	Signed,

	/// <summary>
	/// Display values as unsigned decimal integers.
	/// </summary>
	Unsigned,
}

/// <summary>
/// Specifies the size of values to search for.
/// </summary>
public enum MemorySearchValueSize {
	/// <summary>
	/// 8-bit (1 byte) values.
	/// </summary>
	Byte = 1,

	/// <summary>
	/// 16-bit (2 byte) values.
	/// </summary>
	Word = 2,

	/// <summary>
	/// 32-bit (4 byte) values.
	/// </summary>
	Dword = 4,
}

/// <summary>
/// Specifies what value to compare against during memory searches.
/// </summary>
public enum MemorySearchCompareTo {
	/// <summary>
	/// Compare against the value at the time of the last search filter.
	/// </summary>
	PreviousSearchValue,

	/// <summary>
	/// Compare against the value from the previous data refresh.
	/// </summary>
	PreviousRefreshValue,

	/// <summary>
	/// Compare against a user-specified constant value.
	/// </summary>
	SpecificValue,

	/// <summary>
	/// Compare against the value at a user-specified address.
	/// </summary>
	SpecificAddress
}

/// <summary>
/// Specifies the comparison operator for memory searches.
/// </summary>
public enum MemorySearchOperator {
	/// <summary>
	/// Match addresses where the value equals the comparison value.
	/// </summary>
	Equal,

	/// <summary>
	/// Match addresses where the value does not equal the comparison value.
	/// </summary>
	NotEqual,

	/// <summary>
	/// Match addresses where the value is less than the comparison value.
	/// </summary>
	LessThan,

	/// <summary>
	/// Match addresses where the value is greater than the comparison value.
	/// </summary>
	GreaterThan,

	/// <summary>
	/// Match addresses where the value is less than or equal to the comparison value.
	/// </summary>
	LessThanOrEqual,

	/// <summary>
	/// Match addresses where the value is greater than or equal to the comparison value.
	/// </summary>
	GreaterThanOrEqual,
}
