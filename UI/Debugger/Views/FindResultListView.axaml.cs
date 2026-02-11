using System;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Controls.DataGridExtensions;
using Nexen.Debugger;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Debugger.Views;
public class FindResultListView : UserControl {
	public FindResultListView() {
		InitializeComponent();

		// Subscribe to DataGrid CellDoubleClick routed event
		this.AddHandler(DataGridCellClickBehavior.CellDoubleClickEvent, OnCellDoubleClick);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnDataContextChanged(EventArgs e) {
		if (DataContext is FindResultListViewModel vm) {
			vm.InitContextMenu(this);
		}

		base.OnDataContextChanged(e);
	}

	private void OnCellDoubleClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (DataContext is FindResultListViewModel listModel && e.RowItem is FindResultViewModel result) {
			listModel.GoToResult(result);
		}
	}
}
