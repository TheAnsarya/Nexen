using System;
using System.Threading.Tasks;
using Nexen.Config.Shortcuts;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Services;
using Nexen.ViewModels;

namespace Nexen.Menus;

/// <summary>
/// Categories for menu item enable logic.
/// </summary>
public enum EnableCategory {
	/// <summary>Always enabled regardless of emulator state.</summary>
	AlwaysEnabled,

	/// <summary>Requires a ROM to be loaded.</summary>
	RequiresRom,

	/// <summary>Uses custom IsEnabled function.</summary>
	Custom
}

/// <summary>
/// Menu action linked to an EmulatorShortcut.
/// Handles shortcut key display and execution.
/// </summary>
public class ShortcutMenuAction : MenuActionBase {
	/// <summary>The emulator shortcut this action is linked to.</summary>
	public EmulatorShortcut? Shortcut { get; init; }

	/// <summary>Parameter to pass with the shortcut.</summary>
	public uint ShortcutParam { get; init; }

	/// <summary>Custom shortcut text function.</summary>
	public Func<string>? CustomShortcutText { get; init; }

	/// <summary>The enable category for this action.</summary>
	public EnableCategory Category { get; init; } = EnableCategory.Custom;

	/// <summary>
	/// Creates a new shortcut menu action.
	/// </summary>
	public ShortcutMenuAction() {
	}

	/// <summary>
	/// Creates a new shortcut menu action linked to the specified shortcut.
	/// </summary>
	/// <param name="shortcut">The emulator shortcut.</param>
	/// <param name="category">The enable category. Defaults to Custom which uses IsShortcutAllowed.</param>
	public ShortcutMenuAction(EmulatorShortcut shortcut, EnableCategory category = EnableCategory.Custom) {
		Shortcut = shortcut;
		Category = category;

		// Set up IsEnabled based on category
		IsEnabled = category switch {
			EnableCategory.AlwaysEnabled => null, // null means always enabled
			EnableCategory.RequiresRom => () => EmulatorState.Instance.IsRomLoaded,
			EnableCategory.Custom => () => EmuApi.IsShortcutAllowed(shortcut, ShortcutParam),
			_ => null
		};

		// Set up the click handler to execute the shortcut
		OnClick = () => Task.Run(() => EmuApi.ExecuteShortcut(new ExecuteShortcutParams {
			Shortcut = shortcut,
			Param = ShortcutParam
		}));
	}

	/// <inheritdoc/>
	protected override string ComputeShortcutText() {
		if (CustomShortcutText != null) {
			return CustomShortcutText();
		}

		return Shortcut.HasValue
			? Shortcut.Value.GetShortcutKeys()?.ToString() ?? ""
			: "";
	}
}

/// <summary>
/// Simple menu action that is always enabled.
/// Use for items like Open, Exit, Preferences, Help.
/// </summary>
public class SimpleMenuAction : MenuActionBase {
	/// <summary>Custom shortcut text function.</summary>
	public Func<string>? CustomShortcutText { get; init; }

	/// <summary>
	/// Creates a new simple menu action.
	/// </summary>
	public SimpleMenuAction() {
		// IsEnabled defaults to null, meaning always enabled
	}

	/// <summary>
	/// Creates a new simple menu action with the specified action type.
	/// </summary>
	/// <param name="actionType">The action type for localization and icons.</param>
	public SimpleMenuAction(ActionType actionType) {
		ActionType = actionType;
	}

	/// <inheritdoc/>
	protected override string ComputeShortcutText() {
		return CustomShortcutText?.Invoke() ?? "";
	}
}

/// <summary>
/// Menu separator (horizontal line).
/// </summary>
public class MenuSeparator : MenuActionBase {
	/// <summary>
	/// Creates a new menu separator.
	/// </summary>
	public MenuSeparator() {
		IsEnabled = () => false;
	}

	/// <inheritdoc/>
	protected override string ComputeName() => "-";

	/// <inheritdoc/>
	protected override string ComputeShortcutText() => "";
}
