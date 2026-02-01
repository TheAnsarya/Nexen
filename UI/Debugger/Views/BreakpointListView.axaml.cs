using System;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using DataBoxControl;
using Nexen.Config;
using Nexen.Debugger;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using Nexen.Utilities;
using Nexen.ViewModels;
using static Nexen.Debugger.ViewModels.BreakpointListViewModel;

namespace Nexen.Debugger.Views {
	public class BreakpointListView : UserControl {
		public BreakpointListView() {
			InitializeComponent();
		}

		private void InitializeComponent() {
			AvaloniaXamlLoader.Load(this);
		}

		protected override void OnDataContextChanged(EventArgs e) {
			if (DataContext is BreakpointListViewModel vm) {
				vm.InitContextMenu(this);
			}

			base.OnDataContextChanged(e);
		}

		private void OnCellClick(DataBoxCell cell) {
			if (DataContext is BreakpointListViewModel bpList && cell.DataContext is BreakpointViewModel) {
				string? header = cell.Column?.Header?.ToString() ?? "";
				if (header is "E" or "M") {
					bool isEnabledColumn = header == "E";
					bool newValue = !bpList.Selection.SelectedItems.Any(bp => (isEnabledColumn ? bp?.Breakpoint.Enabled : bp?.Breakpoint.MarkEvent) == true);

					foreach (BreakpointViewModel? bp in bpList.Selection.SelectedItems) {
						if (bp != null) {
							if (isEnabledColumn) {
								bp.Breakpoint.Enabled = newValue;
							} else {
								if (!bp.Breakpoint.Forbid) {
									bp.Breakpoint.MarkEvent = newValue;
								}
							}
						}
					}

					DebugWorkspaceManager.AutoSave();
					BreakpointManager.RefreshBreakpoints();
				}
			}
		}

		private void OnCellDoubleClick(DataBoxCell cell) {
			if (cell.DataContext is BreakpointViewModel vm) {
				BreakpointEditWindow.EditBreakpoint(vm.Breakpoint, this);
			}
		}
	}
}
