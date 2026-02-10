using System;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Services;

namespace Nexen.Menus;

/// <summary>
/// Menu action for debug menu items with debugger shortcuts (DbgShortKeys).
/// </summary>
/// <remarks>
/// Unlike <see cref="ShortcutMenuAction"/> which uses <see cref="EmulatorShortcut"/>,
/// this class uses <see cref="DebuggerShortcut"/> and <see cref="DbgShortKeys"/>
/// for debugger-specific keyboard shortcuts.
/// </remarks>
public class DebugMenuAction : MenuActionBase {
	/// <summary>
	/// The debugger shortcut for this action.
	/// Used to look up the configured shortcut keys.
	/// </summary>
	public DebuggerShortcut? DebugShortcut { get; init; }

	/// <summary>
	/// Function to get the shortcut keys dynamically.
	/// Takes precedence over <see cref="DebugShortcut"/> if set.
	/// </summary>
	public Func<DbgShortKeys>? Shortcut { get; init; }

	/// <summary>
	/// Creates a new debug menu action with no shortcut.
	/// </summary>
	public DebugMenuAction() {
	}

	/// <summary>
	/// Creates a new debug menu action with a debugger shortcut.
	/// </summary>
	/// <param name="shortcut">The debugger shortcut to display and use for keyboard handling.</param>
	public DebugMenuAction(DebuggerShortcut shortcut) {
		DebugShortcut = shortcut;
		// Note: The click handler and enable check are set separately via init properties.
		// DebugShortcutManager handles the keyboard shortcut execution.
	}

	/// <summary>
	/// Gets the shortcut keys for this action.
	/// </summary>
	/// <returns>The configured DbgShortKeys, or default if not configured.</returns>
	public DbgShortKeys GetShortcutKeys() {
		if (Shortcut is not null) {
			return Shortcut();
		}

		if (DebugShortcut.HasValue) {
			return ConfigManager.Config.Debug.Shortcuts.Get(DebugShortcut.Value);
		}

		return new DbgShortKeys();
	}

	/// <inheritdoc/>
	protected override string ComputeShortcutText() {
		return GetShortcutKeys().ToString();
	}
}

/// <summary>
/// Menu separator for debug menus.
/// Works with DebugShortcutManager like DebugMenuAction.
/// </summary>
public sealed class DebugMenuSeparator : DebugMenuAction {
	/// <summary>
	/// Creates a new debug menu separator.
	/// </summary>
	public DebugMenuSeparator() {
		IsEnabled = () => false;
	}

	/// <inheritdoc/>
	protected override string ComputeName() => "-";

	/// <inheritdoc/>
	protected override string ComputeShortcutText() => "";
}
