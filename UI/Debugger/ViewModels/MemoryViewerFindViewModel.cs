using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using Avalonia.Controls;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;

/// <summary>
/// ViewModel for the memory viewer find/search dialog.
/// Provides search functionality within memory viewer with support for hex, string, and integer searches,
/// along with various filtering options based on memory access patterns and data types.
/// </summary>
public sealed class MemoryViewerFindViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the type of data being searched for (hex bytes, string, or integer).
	/// </summary>
	[Reactive] public SearchDataType DataType { get; set; }

	/// <summary>
	/// Gets or sets the integer size type when searching for integers (auto-detect, 8-bit, 16-bit, or 32-bit).
	/// </summary>
	[Reactive] public SearchIntType IntType { get; set; }

	/// <summary>
	/// Gets or sets whether string searches should be case-sensitive.
	/// </summary>
	[Reactive] public bool CaseSensitive { get; set; }

	/// <summary>
	/// Gets or sets whether to use TBL (table) mappings for string encoding conversion.
	/// When enabled, string searches use the loaded TBL file for character mapping.
	/// </summary>
	[Reactive] public bool UseTblMappings { get; set; }

	/// <summary>
	/// Gets or sets the filter for memory locations that have not been accessed.
	/// </summary>
	[Reactive] public bool FilterNotAccessed { get; set; }

	/// <summary>
	/// Gets or sets the filter for memory locations that have been read.
	/// </summary>
	[Reactive] public bool FilterRead { get; set; }

	/// <summary>
	/// Gets or sets the filter for memory locations that have been written.
	/// </summary>
	[Reactive] public bool FilterWrite { get; set; }

	/// <summary>
	/// Gets or sets the filter for memory locations that have been executed.
	/// </summary>
	[Reactive] public bool FilterExec { get; set; }

	/// <summary>
	/// Gets or sets whether the time span filter is enabled for access filtering.
	/// </summary>
	[Reactive] public bool FilterTimeSpanEnabled { get; set; }

	/// <summary>
	/// Gets or sets the time span in frames for access filtering.
	/// </summary>
	[Reactive] public int FilterTimeSpan { get; set; }

	/// <summary>
	/// Gets or sets the filter for memory identified as code.
	/// </summary>
	[Reactive] public bool FilterCode { get; set; }

	/// <summary>
	/// Gets or sets the filter for memory identified as data.
	/// </summary>
	[Reactive] public bool FilterData { get; set; }

	/// <summary>
	/// Gets or sets the filter for unidentified memory regions.
	/// </summary>
	[Reactive] public bool FilterUnidentified { get; set; }

	/// <summary>
	/// Gets whether the current data type is integer (for UI binding).
	/// </summary>
	[Reactive] public bool IsInteger { get; private set; }

	/// <summary>
	/// Gets whether the current data type is string (for UI binding).
	/// </summary>
	[Reactive] public bool IsString { get; private set; }

	/// <summary>
	/// Gets whether the current search input is valid and can be executed.
	/// </summary>
	[Reactive] public bool IsValid { get; private set; } = false;

	/// <summary>
	/// Gets or sets whether to show the "not found" error message.
	/// </summary>
	[Reactive] public bool ShowNotFoundError { get; set; }

	/// <summary>
	/// Gets or sets the search string entered by the user.
	/// </summary>
	[Reactive] public string SearchString { get; set; } = "";

	/// <summary>
	/// Reference to the parent memory tools view model for TBL converter access.
	/// </summary>
	private MemoryToolsViewModel _memToolsModel;

	/// <summary>
	/// Designer-only constructor. Do not use in production code.
	/// </summary>
	[Obsolete("For designer only")]
	public MemoryViewerFindViewModel() : this(new(new())) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="MemoryViewerFindViewModel"/> class.
	/// </summary>
	/// <param name="memToolsModel">The parent memory tools view model providing TBL converter access.</param>
	public MemoryViewerFindViewModel(MemoryToolsViewModel memToolsModel) {
		_memToolsModel = memToolsModel;

		AddDisposable(this.WhenAnyValue(x => x.DataType).Subscribe(x => {
			IsInteger = DataType == SearchDataType.Integer;
			IsString = DataType == SearchDataType.String;
		}));

		AddDisposable(this.WhenAnyValue(x => x.SearchString).Subscribe(x => {
			if (SearchString.Contains(Environment.NewLine)) {
				//Run asynchronously to allow the textbox to update its content correctly
				Dispatcher.UIThread.Post(() => SearchString = SearchString.Replace(Environment.NewLine, " "));
			}
		}));

		AddDisposable(this.WhenAnyValue(x => x.DataType, x => x.IntType, x => x.SearchString).Subscribe(x => {
			SearchData? searchData = GetSearchData();
			IsValid = searchData is not null && searchData.Data.Length > 0;
		}));
	}

	/// <summary>
	/// Parses the search string and converts it to search data based on the current data type.
	/// </summary>
	/// <returns>
	/// A <see cref="SearchData"/> object containing the byte pattern to search for,
	/// or <c>null</c> if the input is invalid.
	/// </returns>
	/// <remarks>
	/// For hex searches, accepts hex digits and '?' wildcards.
	/// For string searches, optionally uses TBL mappings and supports case-insensitive matching.
	/// For integer searches, converts to little-endian byte representation based on IntType.
	/// </remarks>
	public SearchData? GetSearchData() {
		switch (DataType) {
			case SearchDataType.Hex:
				if (Regex.IsMatch(SearchString, "^[ a-f0-9?]+$", RegexOptions.IgnoreCase)) {
					return new SearchData(HexUtilities.HexToArrayWithWildcards(SearchString));
				}

				break;

			case SearchDataType.String:
				if (UseTblMappings && _memToolsModel.TblConverter is not null) {
					if (CaseSensitive) {
						return new SearchData(_memToolsModel.TblConverter.GetBytes(SearchString));
					} else {
						byte[] lcData = _memToolsModel.TblConverter.GetBytes(SearchString.ToLower());
						byte[] ucData = _memToolsModel.TblConverter.GetBytes(SearchString.ToUpper());
						if (lcData.Length != ucData.Length) {
							return null;
						}

						return new SearchData(lcData, ucData);
					}
				} else {
					if (CaseSensitive) {
						return new SearchData(Encoding.UTF8.GetBytes(SearchString));
					} else {
						byte[] lcData = Encoding.UTF8.GetBytes(SearchString.ToLower());
						byte[] ucData = Encoding.UTF8.GetBytes(SearchString.ToUpper());
						if (lcData.Length != ucData.Length) {
							return null;
						}

						return new SearchData(lcData, ucData);
					}
				}

			case SearchDataType.Integer:
				if (long.TryParse(SearchString, out long value)) {
					switch (IntType) {
						case SearchIntType.IntAuto:
							if (value is >= int.MinValue and <= uint.MaxValue) {
								return new SearchData(new byte[] { (byte)(value & 0xFF), (byte)((value >> 8) & 0xFF), (byte)((value >> 16) & 0xFF), (byte)((value >> 24) & 0xFF) });
							} else if (value is >= short.MinValue and <= ushort.MaxValue) {
								return new SearchData(new byte[] { (byte)(value & 0xFF), (byte)((value >> 8) & 0xFF) });
							} else if (value is >= sbyte.MinValue and <= byte.MaxValue) {
								return new SearchData(new byte[] { (byte)(value & 0xFF) });
							}

							break;

						case SearchIntType.Int32:
							if (value is >= int.MinValue and <= uint.MaxValue) {
								return new SearchData(new byte[] { (byte)(value & 0xFF), (byte)((value >> 8) & 0xFF), (byte)((value >> 16) & 0xFF), (byte)((value >> 24) & 0xFF) });
							}

							break;

						case SearchIntType.Int16:
							if (value is >= short.MinValue and <= ushort.MaxValue) {
								return new SearchData(new byte[] { (byte)(value & 0xFF), (byte)((value >> 8) & 0xFF) });
							}

							break;

						case SearchIntType.Int8:
							if (value is >= sbyte.MinValue and <= byte.MaxValue) {
								return new SearchData(new byte[] { (byte)(value & 0xFF) });
							}

							break;
					}
				}

				break;
		}

		return null;
	}

	/// <summary>
	/// Gets whether data type filtering is active (at least one but not all data types selected).
	/// </summary>
	/// <remarks>
	/// Returns <c>true</c> when 1 or 2 of the 3 data type filters (Code, Data, Unidentified) are selected,
	/// indicating that some filtering is in effect but not all types are included.
	/// </remarks>
	public bool IsDataTypeFiltered {
		get {
			int count = (FilterCode ? 1 : 0) + (FilterData ? 1 : 0) + (FilterUnidentified ? 1 : 0);
			return count is > 0 and < 3;
		}
	}

	/// <summary>
	/// Gets whether access type filtering is active (at least one but not all access types selected).
	/// </summary>
	/// <remarks>
	/// Returns <c>true</c> when 1-3 of the 4 access filters (NotAccessed, Read, Write, Exec) are selected,
	/// indicating that some filtering is in effect but not all access types are included.
	/// </remarks>
	public bool IsAccessFiltered {
		get {
			int count = (FilterNotAccessed ? 1 : 0) + (FilterRead ? 1 : 0) + (FilterWrite ? 1 : 0) + (FilterExec ? 1 : 0);
			return count is > 0 and < 4;
		}
	}
}

/// <summary>
/// Represents the data pattern to search for in memory.
/// Supports exact byte matching and case-insensitive string matching via alternate patterns.
/// </summary>
public sealed class SearchData {
	/// <summary>
	/// The primary byte pattern to search for. Values 0-255 are literal bytes, -1 is a wildcard.
	/// </summary>
	public short[] Data;

	/// <summary>
	/// Alternate byte pattern for case-insensitive searches (e.g., uppercase variant).
	/// When set, matching succeeds if either Data or DataAlt matches at each position.
	/// </summary>
	public short[]? DataAlt; //used for case insensitive searches

	/// <summary>
	/// Initializes a new instance of the <see cref="SearchData"/> class from byte arrays.
	/// </summary>
	/// <param name="data">The primary search pattern as bytes.</param>
	/// <param name="dataAlt">Optional alternate pattern for case-insensitive matching.</param>
	public SearchData(byte[] data, byte[]? dataAlt = null) {
		Data = new short[data.Length];
		Array.Copy(data, 0, Data, 0, data.Length);
		if (dataAlt is not null) {
			DataAlt = new short[dataAlt.Length];
			Array.Copy(dataAlt, 0, DataAlt, 0, dataAlt.Length);
		}
	}

	/// <summary>
	/// Initializes a new instance of the <see cref="SearchData"/> class from a short array.
	/// Used for patterns with wildcards (-1 values).
	/// </summary>
	/// <param name="data">The search pattern as shorts, where -1 represents a wildcard.</param>
	public SearchData(short[] data) {
		Data = data;
	}
}

/// <summary>
/// Specifies the type of data format for memory searches.
/// </summary>
public enum SearchDataType {
	/// <summary>Hexadecimal byte pattern search (e.g., "a9 00 8d ?? 20").</summary>
	Hex,
	/// <summary>Text string search with optional TBL mapping and case sensitivity.</summary>
	String,
	/// <summary>Integer value search with configurable byte size.</summary>
	Integer,
}

/// <summary>
/// Specifies the integer size type for integer-based memory searches.
/// </summary>
public enum SearchIntType {
	/// <summary>Auto-detect integer size based on value range.</summary>
	IntAuto,
	/// <summary>8-bit integer (1 byte, signed -128 to 127 or unsigned 0 to 255).</summary>
	Int8,
	/// <summary>16-bit integer (2 bytes, signed -32768 to 32767 or unsigned 0 to 65535).</summary>
	Int16,
	/// <summary>32-bit integer (4 bytes).</summary>
	Int32
}

/// <summary>
/// Specifies the direction of memory search operations.
/// </summary>
public enum SearchDirection {
	/// <summary>Search forward from current position toward higher addresses.</summary>
	Forward,
	/// <summary>Search backward from current position toward lower addresses.</summary>
	Backward
}
