using System;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Controls.DataGridExtensions;
using Nexen.Debugger.Labels;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using static Nexen.Debugger.ViewModels.LabelListViewModel;

namespace Nexen.Debugger.Views;
public partial class LabelListView : UserControl {
	public LabelListView() {
		InitializeComponent();

		// Subscribe to DataGrid CellDoubleClick routed event
		this.AddHandler(DataGridCellClickBehavior.CellDoubleClickEvent, OnCellDoubleClick);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnDataContextChanged(EventArgs e) {
		if (DataContext is LabelListViewModel model) {
			model.InitContextMenu(this);
		}

		base.OnDataContextChanged(e);
	}

	private void OnCellDoubleClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (DataContext is LabelListViewModel listModel && e.RowItem is LabelViewModel label) {
			LabelEditWindow.EditLabel(listModel.CpuType, this, label.Label);
		}
	}
}
