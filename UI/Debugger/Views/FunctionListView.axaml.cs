using System;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using DataBoxControl;
using Nexen.Debugger.Labels;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using static Nexen.Debugger.ViewModels.FunctionListViewModel;

namespace Nexen.Debugger.Views {
	public class FunctionListView : UserControl {
		public FunctionListView() {
			InitializeComponent();
		}

		private void InitializeComponent() {
			AvaloniaXamlLoader.Load(this);
		}

		protected override void OnDataContextChanged(EventArgs e) {
			if (DataContext is FunctionListViewModel model) {
				model.InitContextMenu(this);
			}

			base.OnDataContextChanged(e);
		}

		private void OnCellDoubleClick(DataBoxCell cell) {
			if (DataContext is FunctionListViewModel listModel && cell.DataContext is FunctionViewModel vm && vm.RelAddress >= 0) {
				listModel.Debugger.ScrollToAddress(vm.RelAddress);
			}
		}
	}
}
