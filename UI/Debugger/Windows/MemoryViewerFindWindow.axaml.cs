using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.Windows; 
public partial class MemoryViewerFindWindow : NexenWindow {
	private MemoryViewerFindViewModel _model;
	private MemoryToolsViewModel _viewerModel;

	[Obsolete("For designer only")]
	public MemoryViewerFindWindow() : this(new(), null!) { }

	public MemoryViewerFindWindow(MemoryViewerFindViewModel model, MemoryToolsViewModel viewerModel) {
		DataContext = model;
		_model = model;
		_viewerModel = viewerModel;

		Activated += MemorySearchWindow_Activated;

		AddHandler(InputElement.KeyDownEvent, OnPreviewKeyDown, RoutingStrategies.Tunnel, true);

		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnClosed(EventArgs e) {
		//Prevent NexenWindow logic from disposing the model
		DataContext = null;

		base.OnClosed(e);
	}

	private void OnPreviewKeyDown(object? sender, KeyEventArgs e) {
		if (e.Key == Key.Enter) {
			_viewerModel.Find(SearchDirection.Forward);
			e.Handled = true;
		}
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		this.GetControl<TextBox>("txtValue").FocusAndSelectAll();

		DebugShortcutManager.RegisterActions(this, new List<ContextMenuAction>() {
			new ContextMenuAction() {
				ActionType = ActionType.FindPrev,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FindPrev),
				OnClick = () => _viewerModel.Find(SearchDirection.Backward)
			},
			new ContextMenuAction() {
				ActionType = ActionType.FindNext,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FindNext),
				OnClick = () => _viewerModel.Find(SearchDirection.Forward)
			},
		});
	}

	private void MemorySearchWindow_Activated(object? sender, EventArgs e) {
		this.GetControl<TextBox>("txtValue").Focus();
		this.GetControl<TextBox>("txtValue").SelectAll();
	}

	private void FindPrev_OnClick(object sender, RoutedEventArgs e) {
		_viewerModel.Find(SearchDirection.Backward);
	}

	private void FindNext_OnClick(object sender, RoutedEventArgs e) {
		_viewerModel.Find(SearchDirection.Forward);
	}

	private void Close_OnClick(object sender, RoutedEventArgs e) {
		Close();
	}
}
