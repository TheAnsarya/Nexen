using System;
using System.Collections.Generic;
using System.Windows.Input;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Menus;

/// <summary>
/// Base class for all menu actions in the new menu system.
/// Provides common functionality for enable/disable, visibility, icons, and commands.
/// </summary>
/// <remarks>
/// Unlike the old BaseMenuAction, this class:
/// - Defaults Enabled to true (calculated immediately)
/// - Evaluates IsEnabled during construction, not just on Update()
/// - Has clear categories for enable logic (AlwaysEnabled, RequiresRom, Custom)
/// </remarks>
public abstract class MenuActionBase : ViewModelBase, IMenuAction, IDisposable {
	private static readonly Dictionary<ActionType, string?> IconCache = new();

	static MenuActionBase() {
		foreach (ActionType value in Enum.GetValues<ActionType>()) {
			IconCache[value] = value.GetAttribute<IconFileAttribute>()?.Icon;
		}
	}

	/// <summary>The action type for localization and icon lookup.</summary>
	public ActionType ActionType { get; init; }

	/// <summary>Custom text to display instead of localized ActionType text.</summary>
	public string? CustomText { get; init; }

	/// <summary>Function to get dynamic text (evaluated on each Update).</summary>
	public Func<string>? DynamicText { get; init; }

	/// <summary>Function to get hint text appended to name in parentheses.</summary>
	public Func<string>? HintText { get; init; }

	/// <summary>Function to get dynamic icon path (evaluated on each Update).</summary>
	public Func<string>? DynamicIcon { get; init; }

	/// <summary>Function to determine if the item is enabled.</summary>
	public Func<bool>? IsEnabled { get; init; }

	/// <summary>Function to determine if the item is visible.</summary>
	public Func<bool>? IsVisible { get; init; }

	/// <summary>Function to determine if the item is selected (shows checkmark).</summary>
	public Func<bool>? IsSelected { get; init; }

	/// <summary>Allow action to execute even when hidden.</summary>
	public bool AllowedWhenHidden { get; init; }

	/// <summary>Routing strategy for events.</summary>
	public RoutingStrategies RoutingStrategy { get; init; } = RoutingStrategies.Bubble;

	// ========================================
	// Bound Properties
	// ========================================

	/// <summary>The display name shown in the menu.</summary>
	[Reactive] public string ActionName { get; protected set; } = "";

	/// <summary>The shortcut key text shown in the menu.</summary>
	[Reactive] public string ShortcutText { get; protected set; } = "";

	/// <summary>Tooltip text (Name + ShortcutText).</summary>
	[Reactive] public string TooltipText { get; protected set; } = "";

	/// <summary>The icon shown in the menu.</summary>
	[Reactive] public Image? ActionIcon { get; protected set; }

	/// <summary>Whether the menu item is enabled. Defaults to true.</summary>
	[Reactive] public bool Enabled { get; protected set; } = true;

	/// <summary>Whether the menu item is visible. Defaults to true.</summary>
	[Reactive] public bool Visible { get; protected set; } = true;

	/// <summary>The command to execute when clicked.</summary>
	public ICommand? ClickCommand { get; protected set; }

	// ========================================
	// Submenu Support
	// ========================================

	private List<object>? _subActions;

	/// <summary>Gets or sets the submenu items.</summary>
	public IReadOnlyList<object>? SubActions {
		get => _subActions;
		init {
			_subActions = value != null ? new List<object>(value) : null;
			if (_subActions != null) {
				// Submenus are enabled if any child is enabled
				var parentIsEnabled = IsEnabled;
				IsEnabled = () => {
					if (parentIsEnabled != null && !parentIsEnabled()) {
						return false;
					}

					foreach (object subAction in _subActions) {
						if (subAction is IMenuAction action) {
							if (action.Enabled) {
								return true;
							}
						}
					}

					return false;
				};
			}
		}
	}

	// ========================================
	// Click Handler
	// ========================================

	private Action _onClick = () => { };

	/// <summary>Gets or sets the action to execute when clicked.</summary>
	public Action OnClick {
		get => _onClick;
		init {
			_onClick = CreateSafeClickHandler(value);
			ClickCommand = new SimpleCommand(_onClick);
		}
	}

	/// <summary>Wraps the click handler with safety checks.</summary>
	private Action CreateSafeClickHandler(Action handler) {
		return () => {
			if ((IsVisible == null || AllowedWhenHidden || IsVisible()) &&
			    (IsEnabled == null || IsEnabled())) {
				if (ActionType == ActionType.Exit) {
					// Exit needs to run after the command completes to avoid crash
					Dispatcher.UIThread.Post(handler);
				} else {
					try {
						handler();
					} catch (Exception ex) {
						Dispatcher.UIThread.Post(() => NexenMsgBox.ShowException(ex));
					}
				}
			}
		};
	}

	// ========================================
	// Update Logic
	// ========================================

	private string? _currentIconPath;

	/// <summary>Gets the computed display name.</summary>
	protected virtual string ComputeName() {
		string label = DynamicText != null
			? DynamicText()
			: ActionType == ActionType.Custom
				? CustomText ?? ""
				: ResourceHelper.GetEnumText(ActionType);

		if (HintText != null) {
			string hint = HintText();
			if (!string.IsNullOrWhiteSpace(hint)) {
				label += " (" + hint + ")";
			}
		}

		// Escape underscores to prevent alt-key highlighting
		if (!label.StartsWith('_')) {
			label = label.Replace("_", "__");
		}

		return label;
	}

	/// <summary>Gets the shortcut text to display.</summary>
	protected abstract string ComputeShortcutText();

	/// <summary>Gets the icon file path.</summary>
	protected virtual string? ComputeIconPath() {
		string? actionIcon = IconCache.GetValueOrDefault(ActionType);
		if (!string.IsNullOrEmpty(actionIcon)) {
			return actionIcon;
		}

		if (DynamicIcon != null) {
			return "Assets/" + DynamicIcon() + ".png";
		}

		if (IsSelected?.Invoke() == true) {
			return ConfigManager.ActiveTheme == NexenTheme.Light
				? "Assets/MenuItemChecked.png"
				: "Assets/MenuItemCheckedDark.png";
		}

		return null;
	}

	/// <summary>
	/// Refreshes all computed properties.
	/// Called when the submenu opens.
	/// </summary>
	public virtual void Update() {
		ActionName = ComputeName();
		ShortcutText = ComputeShortcutText();
		TooltipText = ShortcutText.Length > 0 ? $"{ActionName} ({ShortcutText})" : ActionName;

		string? iconPath = ComputeIconPath();
		if (_currentIconPath != iconPath) {
			ActionIcon = iconPath != null ? ImageUtilities.FromAsset(iconPath) : null;
			_currentIconPath = iconPath;
		}

		Enabled = IsEnabled?.Invoke() ?? true;
		Visible = IsVisible?.Invoke() ?? true;
	}

	/// <summary>
	/// Initializes the menu item state immediately.
	/// Call this after setting all properties.
	/// </summary>
	public void Initialize() {
		Update();
	}

	// ========================================
	// IDisposable
	// ========================================

	/// <summary>Disposes resources.</summary>
	public virtual void Dispose() {
		_onClick = () => { };
		ClickCommand = null;
		if (_subActions != null) {
			foreach (object subAction in _subActions) {
				if (subAction is IDisposable disposable) {
					disposable.Dispose();
				}
			}
		}

		GC.SuppressFinalize(this);
	}
}
