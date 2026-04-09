using System;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Controls.DataGridExtensions;
using Nexen.Debugger.Labels;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using static Nexen.Debugger.ViewModels.FunctionListViewModel;

namespace Nexen.Debugger.Views;
public partial class FunctionListView : UserControl {
	public FunctionListView() {
		InitializeComponent();

		// Subscribe to DataGrid CellDoubleClick routed event
		this.AddHandler(DataGridCellClickBehavior.CellDoubleClickEvent, OnCellDoubleClick);
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

	private void OnCellDoubleClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (DataContext is FunctionListViewModel listModel && e.RowItem is FunctionViewModel vm && vm.RelAddress >= 0) {
			listModel.Debugger.ScrollToAddress(vm.RelAddress);
		}
	}
}
