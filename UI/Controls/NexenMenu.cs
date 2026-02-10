using System;
using System.Collections;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Nexen.Debugger.Utilities;
using Nexen.Menus;

namespace Nexen.Controls;

/// <summary>
/// Custom menu control that properly updates menu item enabled/visible states.
/// Menu items with IsEnabled set will have their Enabled property updated when the submenu opens.
/// Items without IsEnabled set default to enabled (Enabled = true).
/// </summary>
public sealed class NexenMenu : Menu {
	protected override Type StyleKeyOverride => typeof(Menu);

	private void SubmenuOpened(object? sender, RoutedEventArgs e) {
		MenuItem menuItem = (MenuItem)sender!;
		IEnumerable? source = menuItem.ItemsSource ?? menuItem.Items;
		if (source is not null) {
			foreach (object subItemAction in source) {
				// Subscribe to nested submenu events
				if (menuItem.ContainerFromItem(subItemAction) is MenuItem subMenuItem) {
					subMenuItem.SubmenuOpened -= SubmenuOpened;
					subMenuItem.SubmenuOpened += SubmenuOpened;
				}

				// Update the menu item state (enabled/visible/text)
				// Support both old BaseMenuAction and new IMenuAction
				if (subItemAction is IMenuAction newAction) {
					newAction.Update();
				} else if (subItemAction is BaseMenuAction action) {
					action.Update();
				}
			}
		}
	}

	protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e) {
		base.OnAttachedToVisualTree(e);
		var items = ItemsSource ?? Items;
		if (items is not null) {
			foreach (object item in items) {
				if (item is MenuItem menuItem) {
					menuItem.SubmenuOpened += SubmenuOpened;
				}
			}
		}
	}

	protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e) {
		base.OnDetachedFromVisualTree(e);
		var items = ItemsSource ?? Items;
		if (items is not null) {
			foreach (object item in items) {
				if (item is MenuItem menuItem) {
					menuItem.SubmenuOpened -= SubmenuOpened;
				}
			}
		}
	}
}
