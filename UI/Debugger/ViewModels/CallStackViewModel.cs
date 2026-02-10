using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Avalonia;
using Avalonia.Collections;
using Avalonia.Controls;
using Avalonia.Controls.Selection;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for displaying and managing the call stack in the debugger.
/// Shows the chain of function calls leading to the current execution point.
/// </summary>
/// <remarks>
/// The call stack displays:
/// - Entry point labels or addresses for each stack frame
/// - Current PC address (relative to CPU memory map)
/// - Absolute address with memory type
/// - Visual indication (italic/gray) for unmapped addresses
///
/// Supports navigation to any stack frame location and label editing.
/// </remarks>
public sealed class CallStackViewModel : DisposableViewModel {
	/// <summary>
	/// Gets the CPU type this call stack is associated with.
	/// </summary>
	public CpuType CpuType { get; }

	/// <summary>
	/// Gets the parent debugger window view model.
	/// </summary>
	public DebuggerWindowViewModel Debugger { get; }

	/// <summary>
	/// Gets or sets the observable collection of call stack entries.
	/// </summary>
	[Reactive] public NexenList<StackInfo> CallStackContent { get; private set; } = new();

	/// <summary>
	/// Gets or sets the selection model for the call stack list.
	/// </summary>
	[Reactive] public SelectionModel<StackInfo?> Selection { get; set; } = new();

	/// <summary>
	/// Gets the column widths from user configuration.
	/// </summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.Debugger.CallStackColumnWidths;

	/// <summary>
	/// Cached stack frame information from the emulator.
	/// </summary>
	private StackFrameInfo[] _stackFrames = [];

	/// <summary>
	/// Designer-only constructor. Do not use in production code.
	/// </summary>
	[Obsolete("For designer only")]
	public CallStackViewModel() : this(CpuType.Snes, new()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="CallStackViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to get call stack for.</param>
	/// <param name="debugger">The parent debugger window view model.</param>
	public CallStackViewModel(CpuType cpuType, DebuggerWindowViewModel debugger) {
		Debugger = debugger;
		CpuType = cpuType;
	}

	/// <summary>
	/// Updates the call stack by fetching current stack frames from the debug API.
	/// </summary>
	public void UpdateCallStack() {
		_stackFrames = DebugApi.GetCallstack(CpuType);
		RefreshCallStack();
	}

	/// <summary>
	/// Refreshes the call stack display using cached stack frame data.
	/// </summary>
	public void RefreshCallStack() {
		CallStackContent.Replace(GetStackInfo());
	}

	/// <summary>
	/// Converts stack frame data into displayable StackInfo objects.
	/// Adds current location at the top and builds the call chain.
	/// </summary>
	/// <returns>List of stack info entries, newest first.</returns>
	private List<StackInfo> GetStackInfo() {
		StackFrameInfo[] stackFrames = _stackFrames;

		List<StackInfo> stack = [];
		for (int i = 0; i < stackFrames.Length; i++) {
			bool isMapped = DebugApi.GetRelativeAddress(stackFrames[i].AbsSource, CpuType).Address >= 0;
			stack.Insert(0, new StackInfo() {
				EntryPoint = GetEntryPoint(i == 0 ? null : stackFrames[i - 1]),
				EntryPointAddr = i == 0 ? null : stackFrames[i - 1].AbsTarget,
				RelAddress = stackFrames[i].Source,
				Address = stackFrames[i].AbsSource,
				RowBrush = isMapped ? AvaloniaProperty.UnsetValue : Brushes.Gray,
				RowStyle = isMapped ? FontStyle.Normal : FontStyle.Italic
			});
		}

		//Add current location
		stack.Insert(0, new StackInfo() {
			EntryPoint = GetEntryPoint(stackFrames.Length > 0 ? stackFrames[^1] : null),
			EntryPointAddr = stackFrames.Length > 0 ? stackFrames[^1].AbsTarget : null,
			RelAddress = DebugApi.GetProgramCounter(CpuType, true),
			Address = DebugApi.GetAbsoluteAddress(new AddressInfo() { Address = (int)DebugApi.GetProgramCounter(CpuType, true), Type = CpuType.ToMemoryType() })
		});

		return stack;
	}

	/// <summary>
	/// Gets a display string for the entry point of a stack frame.
	/// Shows label name if available, or address with NMI/IRQ indication.
	/// </summary>
	/// <param name="stackFrame">The stack frame, or null for bottom of stack.</param>
	/// <returns>Formatted entry point string.</returns>
	private string GetEntryPoint(StackFrameInfo? stackFrame) {
		if (stackFrame is null) {
			return "[bottom of stack]";
		}

		StackFrameInfo entry = stackFrame.Value;

		string format = "X" + CpuType.GetAddressSize();
		CodeLabel? label = entry.AbsTarget.Address >= 0 ? LabelManager.GetLabel(entry.AbsTarget) : null;
		if (label is not null) {
			return label.Label + " ($" + entry.Target.ToString(format) + ")";
		} else if (entry.Flags == StackFrameFlags.Nmi) {
			return "[nmi] $" + entry.Target.ToString(format);
		} else if (entry.Flags == StackFrameFlags.Irq) {
			return "[irq] $" + entry.Target.ToString(format);
		}

		return "$" + entry.Target.ToString(format);
	}

	/// <summary>
	/// Checks if a stack entry's address is mapped to a valid relative address.
	/// </summary>
	/// <param name="entry">The stack entry to check.</param>
	/// <returns>True if the address is mapped, false otherwise.</returns>
	private bool IsMapped(StackInfo entry) {
		return DebugApi.GetRelativeAddress(entry.Address, CpuType).Address >= 0;
	}

	/// <summary>
	/// Navigates the debugger to the location specified by a stack entry.
	/// </summary>
	/// <param name="entry">The stack entry to navigate to.</param>
	public void GoToLocation(StackInfo entry) {
		if (IsMapped(entry)) {
			Debugger.ScrollToAddress((int)entry.RelAddress);
		}
	}

	/// <summary>
	/// Initializes the context menu with call stack actions.
	/// Includes edit label and go to location options.
	/// </summary>
	/// <param name="parent">The parent control for the context menu.</param>
	public void InitContextMenu(Control parent) {
		AddDisposables(DebugShortcutManager.CreateContextMenu(parent, new object[] {
			new ContextMenuAction() {
				ActionType = ActionType.EditLabel,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.CallStack_EditLabel),
				IsEnabled = () => Selection.SelectedItem is StackInfo entry && entry.EntryPointAddr is not null,
				OnClick = () => {
					if(Selection.SelectedItem is StackInfo entry && entry.EntryPointAddr is not null) {
						AddressInfo addr = entry.EntryPointAddr.Value;
						CodeLabel? label = LabelManager.GetLabel((uint)addr.Address, addr.Type);
						label ??= new CodeLabel() {
								Address = (uint)entry.EntryPointAddr.Value.Address,
								MemoryType = entry.EntryPointAddr.Value.Type
							};

						LabelEditWindow.EditLabel(CpuType, parent, label);
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.GoToLocation,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.CallStack_GoToLocation),
				IsEnabled = () => Selection.SelectedItem is StackInfo entry && IsMapped(entry),
				OnClick = () => {
					if(Selection.SelectedItem is StackInfo entry) {
						GoToLocation(entry);
					}
				}
			},
		}));
	}
}

/// <summary>
/// Represents a single entry in the call stack display.
/// Contains both display-formatted strings and raw address data.
/// </summary>
public sealed class StackInfo {
	/// <summary>
	/// Gets or sets the entry point label or address for this stack frame.
	/// </summary>
	public string EntryPoint { get; set; } = "";

	/// <summary>
	/// Gets the PC address formatted as a hex string.
	/// </summary>
	public string PcAddress => $"${RelAddress:X4}";

	/// <summary>
	/// Gets the absolute address with memory type, or empty string if unmapped.
	/// </summary>
	public string AbsAddress {
		get {
			if (Address.Address >= 0) {
				return $"${Address.Address:X4} [{Address.Type.GetShortName()}]";
			} else {
				return "";
			}
		}
	}

	/// <summary>
	/// Gets or sets the absolute address of the entry point, if available.
	/// </summary>
	public AddressInfo? EntryPointAddr { get; set; }

	/// <summary>
	/// Gets or sets the relative (CPU-visible) address.
	/// </summary>
	public UInt32 RelAddress { get; set; }

	/// <summary>
	/// Gets or sets the absolute address with memory type.
	/// </summary>
	public AddressInfo Address { get; set; }

	/// <summary>
	/// Gets or sets the brush for row highlighting (gray for unmapped).
	/// </summary>
	public object RowBrush { get; set; } = AvaloniaProperty.UnsetValue;

	/// <summary>
	/// Gets or sets the font style (italic for unmapped addresses).
	/// </summary>
	public FontStyle RowStyle { get; set; }
}
