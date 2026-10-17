using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Nexen.TAS;
using Nexen.Utilities;

namespace Nexen.Windows;

/// <summary>
/// Result of branch management dialog operations.
/// </summary>
public sealed partial class BranchManagementResult {
	public BranchData? LoadedBranch { get; set; }
	public bool Changed { get; set; }
}

/// <summary>
/// Dialog for managing TAS branches — load, rename, and delete.
/// </summary>
public partial class BranchManagementDialog : NexenWindow {

	private ObservableCollection<BranchData> _branches = null!;
	private BranchData? _loadedBranch;
	private bool _changed;

	public BranchManagementDialog() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void OnCloseClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		Close();
	}

	private void OnLoadClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (lstBranches.SelectedItem is BranchData branch) {
			_loadedBranch = branch;
			Close();
		}
	}

	private async void OnRenameClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (lstBranches.SelectedItem is not BranchData branch) {
			return;
		}

		string? newName = await TasInputDialog.ShowTextAsync(
			this,
			"Rename Branch",
			"Enter new name:",
			branch.Name);

		if (newName is not null && newName != branch.Name) {
			branch.Name = newName;
			_changed = true;
			// Refresh the list display
			int idx = lstBranches.SelectedIndex;
			lstBranches.ItemsSource = null;
			lstBranches.ItemsSource = _branches;
			lstBranches.SelectedIndex = idx;
		}
	}

	private async void OnDeleteClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (lstBranches.SelectedItem is not BranchData branch) {
			return;
		}

		var result = await MessageBox.Show(
			this,
			$"Delete branch \"{branch.Name}\"?\nThis cannot be undone.",
			"Delete Branch",
			MessageBoxButtons.YesNo,
			MessageBoxIcon.Warning);

		if (result == DialogResult.Yes) {
			_branches.Remove(branch);
			_changed = true;
		}
	}

	/// <summary>
	/// Shows the branch management dialog.
	/// </summary>
	/// <param name="parent">Parent window.</param>
	/// <param name="branches">The branches collection (modified in-place for delete/rename).</param>
	/// <returns>Management result with loaded branch and change flag.</returns>
	public static Task<BranchManagementResult> ShowAsync(Window? parent, ObservableCollection<BranchData> branches) {
		var dialog = new BranchManagementDialog();
		dialog._branches = branches;
		dialog.lstBranches.ItemsSource = branches;

		var tcs = new TaskCompletionSource<BranchManagementResult>();
		dialog.Closed += (_, _) => {
			tcs.TrySetResult(new BranchManagementResult {
				LoadedBranch = dialog._loadedBranch,
				Changed = dialog._changed
			});
		};

		parent ??= ApplicationHelper.GetActiveOrMainWindow();
		if (parent != null) {
			if (!OperatingSystem.IsWindows()) {
				dialog.Opened += (_, _) => WindowExtensions.CenterWindow(dialog, parent);
			}
			dialog.ShowDialog(parent);
		}

		return tcs.Task;
	}
}
