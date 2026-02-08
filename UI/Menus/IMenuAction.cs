using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Input;
using Avalonia.Controls;

namespace Nexen.Menus;

/// <summary>
/// Interface for all menu actions in the Nexen menu system.
/// Implementations provide enable/disable logic, visibility, icons, and commands.
/// </summary>
/// <remarks>
/// Property names are chosen to match the existing XAML bindings in NexenStyles.xaml.
/// </remarks>
public interface IMenuAction : INotifyPropertyChanged {
	/// <summary>Gets the display name of the menu item.</summary>
	string ActionName { get; }

	/// <summary>Gets the keyboard shortcut text to display.</summary>
	string ShortcutText { get; }

	/// <summary>Gets the tooltip text (usually ActionName + ShortcutText).</summary>
	string TooltipText { get; }

	/// <summary>Gets the icon to display, or null for no icon.</summary>
	Image? ActionIcon { get; }

	/// <summary>Gets whether the menu item is enabled.</summary>
	bool Enabled { get; }

	/// <summary>Gets whether the menu item is visible.</summary>
	bool Visible { get; }

	/// <summary>Gets the command to execute when the menu item is clicked.</summary>
	ICommand? ClickCommand { get; }

	/// <summary>Gets the submenu items, or null if no submenu.</summary>
	IReadOnlyList<object>? SubActions { get; }

	/// <summary>
	/// Refreshes the menu item state (enabled, visible, name, icon).
	/// Called when the submenu opens.
	/// </summary>
	void Update();
}
