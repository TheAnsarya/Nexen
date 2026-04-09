using System;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Controls.DataGridExtensions;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;

namespace Nexen.Debugger.Views;
public partial class CallStackView : UserControl {
	public CallStackView() {
		InitializeComponent();

		// Subscribe to DataGrid CellDoubleClick routed event
		this.AddHandler(DataGridCellClickBehavior.CellDoubleClickEvent, OnCellDoubleClick);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnDataContextChanged(EventArgs e) {
		if (DataContext is CallStackViewModel model) {
			model.InitContextMenu(this);
		}

		base.OnDataContextChanged(e);
	}

	private void OnCellDoubleClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (e.RowItem is StackInfo stack && DataContext is CallStackViewModel model) {
			model.GoToLocation(stack);
		}
	}
}
