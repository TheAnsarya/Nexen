using System;
using System.Collections.Generic;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using AvaloniaEdit.Editing;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using static Nexen.Debugger.ViewModels.LabelListViewModel;

namespace Nexen.Debugger.Views; 
public class RegisterTabView : NexenUserControl {
	public RegisterViewerTab Model => (RegisterViewerTab)DataContext!;

	public RegisterTabView() {
		InitializeComponent();

		DataGrid dataGrid = this.GetControl<DataGrid>("lstRegisterTab");

		// Apply row background styling for header rows
		dataGrid.LoadingRow += (_, e) => {
			if (e.Row.DataContext is RegEntry entry) {
				e.Row.Background = entry.Background;
			}
		};

		AddDisposables(DebugShortcutManager.CreateContextMenu(dataGrid, new List<ContextMenuAction>() {
			new ContextMenuAction() {
				ActionType = ActionType.EditBreakpoint,
				IsEnabled = () => Model.CpuType.HasValue && Model.MemoryType.HasValue && Model.SelectedItem is RegEntry entry && entry.StartAddress >= 0,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.RegisterViewer_EditBreakpoint),
				HintText = () => {
					if(Model.CpuType.HasValue && Model.MemoryType.HasValue && Model.SelectedItem is RegEntry entry && entry.StartAddress >= 0) {
						string hint = $"${entry.StartAddress:X4}";
						if(entry.EndAddress > entry.StartAddress) {
							hint += $" - ${entry.EndAddress:X4}";
						}

						return hint;
					}

					return "";
				},
				OnClick = async () => {
					if(Model.CpuType.HasValue && Model.MemoryType.HasValue && Model.SelectedItem is RegEntry entry) {
						uint startAddress = (uint)entry.StartAddress;
						uint endAddress = (uint)entry.EndAddress;

						Breakpoint? bp = BreakpointManager.GetMatchingBreakpoint(startAddress, endAddress, Model.MemoryType.Value);
						bp ??= new Breakpoint() {
								BreakOnRead = true,
								BreakOnWrite = true,
								CpuType = Model.CpuType.Value,
								StartAddress = (uint)entry.StartAddress,
								EndAddress = (uint)entry.EndAddress,
								MemoryType = Model.MemoryType.Value
							};

						bool result = await BreakpointEditWindow.EditBreakpointAsync(bp, this);
						if(result && DebugWindowManager.GetDebugWindow<DebuggerWindow>(x => x.CpuType == Model.CpuType) == null) {
							DebuggerWindow.GetOrOpenWindow(Model.CpuType.Value);
						}
					}
				}
			}
		}));
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
