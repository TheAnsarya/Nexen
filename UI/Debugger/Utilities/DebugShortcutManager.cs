using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Rendering;
using Nexen.Config;
using Nexen.Menus;

namespace Nexen.Debugger.Utilities;
internal static class DebugShortcutManager {
	public static IEnumerable CreateContextMenu(Control ctrl, Control focusCtrl, IEnumerable actions) {
		if (ctrl is not IInputElement) {
			throw new Exception("Invalid control");
		}

		ctrl.ContextMenu = new ContextMenu();
		ctrl.ContextMenu.Name = "ActionMenu";
		ctrl.ContextMenu.ItemsSource = actions;
		RegisterActions(focusCtrl, actions);

		return actions;
	}

	public static IEnumerable CreateContextMenu(Control ctrl, IEnumerable actions) {
		return CreateContextMenu(ctrl, ctrl, actions);
	}

	public static void RegisterActions(IInputElement focusParent, IEnumerable actions) {
		foreach (object obj in actions) {
			if (obj is ContextMenuAction action) {
				RegisterAction(focusParent, action);
			} else if (obj is DebugMenuAction debugAction) {
				RegisterAction(focusParent, debugAction);
			}
		}
	}

	public static void RegisterActions(IInputElement focusParent, IEnumerable<ContextMenuAction> actions) {
		foreach (ContextMenuAction action in actions) {
			RegisterAction(focusParent, action);
		}
	}

	/// <summary>
	/// Registers keyboard shortcuts for DebugMenuAction items.
	/// </summary>
	public static void RegisterAction(IInputElement focusParent, DebugMenuAction action) {
		WeakReference<IInputElement> weakFocusParent = new(focusParent);
		WeakReference<DebugMenuAction> weakAction = new(action);

		if (action.SubActions is not null) {
			RegisterActions(focusParent, action.SubActions);
		}

		// Only register if this action has a shortcut
		if (action.Shortcut is null && !action.DebugShortcut.HasValue) {
			return;
		}

		EventHandler<KeyEventArgs>? handler = null;
		handler = (s, e) => {
			if (weakFocusParent.TryGetTarget(out IInputElement? elem)) {
				if (weakAction.TryGetTarget(out DebugMenuAction? act)) {
					DbgShortKeys keys = act.GetShortcutKeys();
					if (e.Key != Key.None && e.Key == keys.ShortcutKey && e.KeyModifiers == keys.Modifiers) {
						if (action.RoutingStrategy == RoutingStrategies.Bubble && e.Source is Control ctrl && ctrl.Classes.Contains("PreventShortcuts")) {
							return;
						}

						if (act.Enabled) {
							act.OnClick();
							e.Handled = true;
						}
					}
				} else {
					elem.RemoveHandler(InputElement.KeyDownEvent, handler!);
				}
			}
		};

		focusParent.AddHandler(InputElement.KeyDownEvent, handler, action.RoutingStrategy, handledEventsToo: true);
	}

	public static void RegisterAction(IInputElement focusParent, ContextMenuAction action) {
		WeakReference<IInputElement> weakFocusParent = new WeakReference<IInputElement>(focusParent);
		WeakReference<ContextMenuAction> weakAction = new WeakReference<ContextMenuAction>(action);

		if (action.SubActions is not null) {
			RegisterActions(focusParent, action.SubActions);
		}

		EventHandler<KeyEventArgs>? handler = null;
		handler = (s, e) => {
			if (weakFocusParent.TryGetTarget(out IInputElement? elem)) {
				if (weakAction.TryGetTarget(out ContextMenuAction? act)) {
					if (act.Shortcut is not null) {
						DbgShortKeys keys = act.Shortcut();
						if (e.Key != Key.None && e.Key == keys.ShortcutKey && e.KeyModifiers == keys.Modifiers) {
							if (action.RoutingStrategy == RoutingStrategies.Bubble && e.Source is Control ctrl && ctrl.Classes.Contains("PreventShortcuts")) {
								return;
							}

							if (act.IsEnabled is null || act.IsEnabled()) {
								act.OnClick();
								e.Handled = true;
							}
						}
					}
				} else {
					elem.RemoveHandler(InputElement.KeyDownEvent, handler!);
				}
			}
		};

		focusParent.AddHandler(InputElement.KeyDownEvent, handler, action.RoutingStrategy, handledEventsToo: true);
	}
}
