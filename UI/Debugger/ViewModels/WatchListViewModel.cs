using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Reactive.Linq;
using System.Text.RegularExpressions;
using Avalonia.Collections;
using Avalonia.Controls;
using Avalonia.Controls.Selection;
using Avalonia.Interactivity;
using Nexen.Config;
using Nexen.Debugger.Disassembly;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for managing and displaying watch expressions in the debugger.
/// Provides evaluation, formatting, and manipulation of watch entries.
/// </summary>
/// <remarks>
/// Watch expressions can be:
/// - Memory addresses: [address] or {address} syntax
/// - Labels: references to named memory locations
/// - Complex expressions: evaluated by the debugger
///
/// Supports multiple display formats (hex, decimal, binary, signed/unsigned)
/// with configurable byte lengths (8, 16, 24, 32 bits).
/// </remarks>
public class WatchListViewModel : DisposableViewModel, IToolHelpTooltip {
	/// <summary>
	/// Regex pattern for matching watch expressions that are addresses or labels.
	/// Matches formats like [$FFFF], {123}, [LabelName], etc.
	/// </summary>
	private static Regex _watchAddressOrLabel = new Regex(@"^(\[|{)(\s*((\$[0-9A-Fa-f]+)|(\d+)|([@_a-zA-Z0-9]+)))\s*[,]{0,1}\d*\s*(\]|})$", RegexOptions.Compiled);

	/// <summary>
	/// Gets or sets the observable collection of watch entries.
	/// </summary>
	[Reactive] public NexenList<WatchValueInfo> WatchEntries { get; private set; } = new();

	/// <summary>
	/// Gets or sets the selection model for multi-select support.
	/// </summary>
	[Reactive] public SelectionModel<WatchValueInfo> Selection { get; set; } = new() { SingleSelect = false };

	/// <summary>
	/// Gets the column widths from user configuration.
	/// </summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.Debugger.WatchListColumnWidths;

	/// <summary>
	/// Gets the watch manager that stores and evaluates watch expressions.
	/// </summary>
	public WatchManager Manager { get; }

	/// <summary>
	/// Gets the CPU type this watch list is associated with.
	/// </summary>
	public CpuType CpuType { get; }

	/// <summary>
	/// Gets the help tooltip content for watch expression syntax.
	/// </summary>
	public object HelpTooltip => ExpressionTooltipHelper.GetHelpTooltip(CpuType, true);

	/// <summary>
	/// Designer-only constructor using SNES as default CPU type.
	/// </summary>
	public WatchListViewModel() : this(CpuType.Snes) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="WatchListViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to use for watch evaluation.</param>
	public WatchListViewModel(CpuType cpuType) {
		CpuType = cpuType;
		Manager = WatchManager.GetWatchManager(cpuType);

		Manager.WatchChanged += Manager_WatchChanged;
	}

	/// <summary>
	/// Disposes the view and unsubscribes from watch change events.
	/// </summary>
	protected override void DisposeView() {
		base.DisposeView();
		Manager.WatchChanged -= Manager_WatchChanged;
	}

	/// <summary>
	/// Handles watch change events from the manager.
	/// </summary>
	/// <param name="resetSelection">True to clear selection, false to preserve it.</param>
	private void Manager_WatchChanged(bool resetSelection) {
		if (resetSelection) {
			Selection.Clear();
		}

		UpdateWatch();
	}

	/// <summary>
	/// Updates all watch entries with current values from memory.
	/// Either replaces the entire list or updates values in place depending on count.
	/// </summary>
	public void UpdateWatch() {
		List<WatchValueInfo> newEntries = Manager.GetWatchContent(WatchEntries);

		if (newEntries.Count != WatchEntries.Count) {
			List<int> selectedIndexes = Selection.SelectedIndexes.ToList();
			WatchEntries.Replace(newEntries);
			Selection.SelectIndexes(selectedIndexes, WatchEntries.Count);
		} else {
			for (int i = 0; i < newEntries.Count; i++) {
				WatchEntries[i].Expression = newEntries[i].Expression;
				WatchEntries[i].Value = newEntries[i].Value;
				WatchEntries[i].NumericValue = newEntries[i].NumericValue;
				WatchEntries[i].IsChanged = newEntries[i].IsChanged;
			}
		}
	}

	/// <summary>
	/// Edits the watch expression at the specified index.
	/// </summary>
	/// <param name="index">The index of the watch entry to edit.</param>
	/// <param name="expression">The new watch expression.</param>
	public void EditWatch(int index, string expression) {
		if (index < 0) {
			return;
		}

		Manager.UpdateWatch(index, expression);
	}

	/// <summary>
	/// Moves the watch entry at the specified index up one position.
	/// </summary>
	/// <param name="index">The index of the watch entry to move up.</param>
	public void MoveUp(int index) {
		List<string> entries = Manager.WatchEntries;
		if (index > 0 && index < entries.Count) {
			string currentEntry = entries[index];
			string entryAbove = entries[index - 1];
			Manager.UpdateWatch(index - 1, currentEntry);
			Manager.UpdateWatch(index, entryAbove);
		}
	}

	/// <summary>
	/// Moves the watch entry at the specified index down one position.
	/// </summary>
	/// <param name="index">The index of the watch entry to move down.</param>
	public void MoveDown(int index) {
		List<string> entries = Manager.WatchEntries;
		if (index < entries.Count - 1) {
			string currentEntry = entries[index];
			string entryBelow = entries[index + 1];
			Manager.UpdateWatch(index + 1, currentEntry);
			Manager.UpdateWatch(index, entryBelow);
		}
	}

	/// <summary>
	/// Converts a collection of watch value items to their indices in the watch list.
	/// </summary>
	/// <param name="items">The items to get indices for.</param>
	/// <returns>Array of indices corresponding to the items.</returns>
	private int[] GetIndexes(IEnumerable<WatchValueInfo?> items) {
		return items.Cast<WatchValueInfo>().Select(x => WatchEntries.IndexOf(x)).ToArray();
	}

	/// <summary>
	/// Deletes the specified watch entries from the list.
	/// </summary>
	/// <param name="items">The watch entries to delete.</param>
	internal void DeleteWatch(List<WatchValueInfo?> items) {
		Manager.RemoveWatch(GetIndexes(items));
	}

	/// <summary>
	/// Sets the display format for the specified watch entries.
	/// </summary>
	/// <param name="format">The format style (Hex, Signed, Unsigned, Binary).</param>
	/// <param name="byteLength">The number of bytes to display (1, 2, 3, or 4).</param>
	/// <param name="items">The watch entries to format.</param>
	internal void SetSelectionFormat(WatchFormatStyle format, int byteLength, IEnumerable<WatchValueInfo?> items) {
		Manager.SetSelectionFormat(format, byteLength, GetIndexes(items));
	}

	/// <summary>
	/// Clears the custom format from the specified watch entries, reverting to default display.
	/// </summary>
	/// <param name="items">The watch entries to clear formatting from.</param>
	internal void ClearSelectionFormat(IEnumerable<WatchValueInfo?> items) {
		Manager.ClearSelectionFormat(GetIndexes(items));
	}

	/// <summary>
	/// Gets the memory location information for the currently selected watch entry.
	/// Parses the expression to extract address or label information.
	/// </summary>
	/// <returns>Location information if valid, null otherwise.</returns>
	private LocationInfo? GetLocation() {
		if (Selection.SelectedIndex < 0 || Selection.SelectedIndex >= WatchEntries.Count) {
			return null;
		}

		WatchValueInfo entry = WatchEntries[Selection.SelectedIndex];

		Match match = _watchAddressOrLabel.Match(entry.Expression);
		if (match.Success) {
			string address = match.Groups[3].Value;

			if (address[0] is (>= '0' and <= '9') or '$') {
				//CPU Address
				bool isHex = address[0] == '$';
				string addrString = isHex ? address.Substring(1) : address;
				if (Int32.TryParse(addrString, isHex ? NumberStyles.AllowHexSpecifier : NumberStyles.None, null, out int parsedAddress)) {
					return new LocationInfo() { RelAddress = new() { Address = parsedAddress, Type = CpuType.ToMemoryType() } };
				}
			} else {
				//Label
				CodeLabel? label = LabelManager.GetLabel(address);
				if (label != null) {
					if (label.Matches(CpuType)) {
						return new LocationInfo() { Label = label, RelAddress = label.GetRelativeAddress(CpuType) };
					}

					return null;
				}
			}
		}

		if (entry.NumericValue is >= 0 and < uint.MaxValue) {
			return new LocationInfo() { RelAddress = new() { Address = (int)entry.NumericValue, Type = CpuType.ToMemoryType() } };
		} else {
			return null;
		}
	}

	/// <summary>
	/// Initializes the context menu with watch list actions.
	/// Includes delete, move, format, and navigation options.
	/// </summary>
	/// <param name="ctrl">The control to attach the context menu to.</param>
	public void InitContextMenu(Control ctrl) {
		AddDisposables(DebugShortcutManager.CreateContextMenu(ctrl, new object[] {
			new ContextMenuAction() {
				ActionType = ActionType.Delete,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.WatchList_Delete),
				IsEnabled = () => Selection.SelectedItems.Count > 0,
				OnClick = () => DeleteWatch(Selection.SelectedItems.ToList())              },

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.MoveUp,
				RoutingStrategy = RoutingStrategies.Tunnel,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.WatchList_MoveUp),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedIndex > 0,
				OnClick = () => MoveUp(Selection.SelectedIndex)          },

			new ContextMenuAction() {
				ActionType = ActionType.MoveDown,
				RoutingStrategy = RoutingStrategies.Tunnel,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.WatchList_MoveDown),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedIndex < WatchEntries.Count - 2,
				OnClick = () => MoveDown(Selection.SelectedIndex)              },

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.RowDisplayFormat,
				SubActions = new() {
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatBinary,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Binary, 1, Selection.SelectedItems)
					},
					new ContextMenuSeparator(),
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatHex8Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Hex, 1, Selection.SelectedItems)
					},
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatHex16Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Hex, 2, Selection.SelectedItems)
					},
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatHex24Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Hex, 3, Selection.SelectedItems)
					},
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatHex32Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Hex, 4, Selection.SelectedItems)
					},
					new ContextMenuSeparator(),
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatSigned8Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Signed, 1, Selection.SelectedItems)
					},
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatSigned16Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Signed, 2, Selection.SelectedItems)
					},
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatSigned24Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Signed, 3, Selection.SelectedItems)
					},
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatSigned32Bits,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Signed, 4, Selection.SelectedItems)
					},
					new ContextMenuSeparator(),
					new ContextMenuAction() {
						ActionType = ActionType.RowFormatUnsigned,
						OnClick = () => SetSelectionFormat(WatchFormatStyle.Unsigned, 1, Selection.SelectedItems)
					},
					new ContextMenuSeparator(),
					new ContextMenuAction() {
						ActionType = ActionType.ClearFormat,
						OnClick = () => ClearSelectionFormat(Selection.SelectedItems)
					}
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.WatchDecimalDisplay,
				IsSelected = () => ConfigManager.Config.Debug.Debugger.WatchFormat == WatchFormatStyle.Signed,
				OnClick = () => {
					ConfigManager.Config.Debug.Debugger.WatchFormat = WatchFormatStyle.Signed;
					UpdateWatch();
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.WatchHexDisplay,
				IsSelected = () => ConfigManager.Config.Debug.Debugger.WatchFormat == WatchFormatStyle.Hex,
				OnClick = () => {
					ConfigManager.Config.Debug.Debugger.WatchFormat = WatchFormatStyle.Hex;
					UpdateWatch();
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.WatchBinaryDisplay,
				IsSelected = () => ConfigManager.Config.Debug.Debugger.WatchFormat == WatchFormatStyle.Binary,
				OnClick = () => {
					ConfigManager.Config.Debug.Debugger.WatchFormat = WatchFormatStyle.Binary;
					UpdateWatch();
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.ViewInMemoryViewer,
				IsEnabled = () => GetLocation() != null,
				HintText = GetLocationHint,
				OnClick = () => {
					LocationInfo? location = GetLocation();
					if(location != null && location.RelAddress != null) {
						MemoryToolsWindow.ShowInMemoryTools(CpuType.ToMemoryType(), location.RelAddress.Value.Address);
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.GoToLocation,
				IsEnabled = () => GetLocation() != null,
				HintText = GetLocationHint,
				OnClick = () => {
					LocationInfo? location = GetLocation();
					if(location != null && location.RelAddress != null) {
						DebuggerWindow.OpenWindowAtAddress(CpuType, location.RelAddress.Value.Address);
					}
				}
			}
		}));
	}

	/// <summary>
	/// Gets a display hint string for the current location.
	/// Returns the label name or formatted hex address.
	/// </summary>
	/// <returns>Label name, hex address, or empty string if no valid location.</returns>
	private string GetLocationHint() {
		LocationInfo? location = GetLocation();
		if (location?.Label != null) {
			return location.Label.Label;
		} else if (location?.RelAddress != null) {
			return "$" + location.RelAddress.Value.Address.ToString("X" + CpuType.GetAddressSize());
		}

		return "";
	}
}
