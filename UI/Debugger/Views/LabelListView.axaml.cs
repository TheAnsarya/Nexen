using System;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using DataBoxControl;
using Nexen.Debugger.Labels;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using static Nexen.Debugger.ViewModels.LabelListViewModel;

namespace Nexen.Debugger.Views {
	public class LabelListView : UserControl {
		public LabelListView() {
			InitializeComponent();
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

		private void OnCellDoubleClick(DataBoxCell cell) {
			if (DataContext is LabelListViewModel listModel && cell.DataContext is LabelViewModel label) {
				LabelEditWindow.EditLabel(listModel.CpuType, this, label.Label);
			}
		}
	}
}
