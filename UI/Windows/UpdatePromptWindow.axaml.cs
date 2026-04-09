using System;
using System.ComponentModel;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Windows; 
public partial class UpdatePromptWindow : NexenWindow {
	private UpdatePromptViewModel _model;

	[Obsolete("For designer only")]
	public UpdatePromptWindow() : this(new(new(), new())) { }

	public UpdatePromptWindow(UpdatePromptViewModel model) {
		_model = model;
		DataContext = model;

		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		base.OnClosing(e);
		if (_model.IsUpdating) {
			e.Cancel = true;
		}
	}

	private void OnUpdateClick(object sender, RoutedEventArgs e) {
		if (_model.FileInfo == null || UpdateHelper.GetCommitHash() == null) {
			NexenMsgBox.Show(null, "AutoUpdateNotSupported", MessageBoxButtons.OK, MessageBoxIcon.Info);
			return;
		}

		_model.Progress = 0;
		_model.IsUpdating = true;

		Task.Run(async () => {
			bool result;
			try {
				result = await _model.UpdateNexen(this);
			} catch (Exception ex) {
				result = false;
				Dispatcher.UIThread.Post(() => {
					_model.IsUpdating = false;
					NexenMsgBox.ShowException(ex);
				});
				return;
			}

			Dispatcher.UIThread.Post(() => {
				_model.IsUpdating = false;
				if (result) {
					Close(true);
				}
			});
		});
	}

	private void OnCancelClick(object sender, RoutedEventArgs e) {
		Close(false);
	}
}
