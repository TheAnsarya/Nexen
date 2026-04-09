using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Utilities;

namespace Nexen.Debugger.Windows; 
public partial class BreakpointEditWindow : NexenWindow {
	[Obsolete("For designer only")]
	public BreakpointEditWindow() : this(new BreakpointEditViewModel()) { }

	public BreakpointEditWindow(BreakpointEditViewModel model) {
		DataContext = model;
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		this.GetControl<NexenNumericTextBox>("startAddress").FocusAndSelectAll();
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		Close(true);
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close(false);
	}

	public static async void EditBreakpoint(Breakpoint bp, Control parent) {
		await EditBreakpointAsync(bp, parent);
	}

	public static async Task<bool> EditBreakpointAsync(Breakpoint bp, Control parent) {
		Breakpoint copy = bp.Clone();
		BreakpointEditViewModel model = new BreakpointEditViewModel(copy);
		BreakpointEditWindow wnd = new BreakpointEditWindow(model);

		bool result = await wnd.ShowCenteredDialog<bool>(parent);
		if (result) {
			bp.CopyFrom(copy);
			BreakpointManager.AddBreakpoint(bp);
		}

		model.Dispose();

		return result;
	}
}
