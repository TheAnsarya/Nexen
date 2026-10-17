using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.VisualTree;
using Nexen.Config;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Views;
public partial class MainMenuView : UserControl {
	public Menu MainMenu { get; }
	private readonly MenuItem _mnuLanguageRoot;
	private readonly Dictionary<UiLanguage, MenuItem> _languageItems = [];

	public MainMenuView() {
		InitializeComponent();
		MainMenu = this.GetControl<Menu>("ActionMenu");
		_mnuLanguageRoot = this.GetControl<MenuItem>("mnuLanguageRoot");

		MainMenu.Closed += (s, e) =>                //When an option is selected in the menu (e.g with enter or mouse click)
													//steal focus away from the menu to ensure pressing e.g left/right goes to the
													//game only and doesn't re-activate the main menu
			ApplicationHelper.GetMainWindow()?.Focus();

		Panel panel = this.GetControl<Panel>("MenuPanel");
		panel.PointerPressed += (s, e) => {
			if (s == panel) {
				//Close the menu when the blank space on the right is clicked
				MainMenu.Close();
				ApplicationHelper.GetMainWindow()?.Focus();
			}
		};

		ResourceHelper.LanguageChanged += ResourceHelper_LanguageChanged;
		RebuildLanguageMenuItems();
		RefreshLanguageCheckStates();
	}

	protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e) {
		ResourceHelper.LanguageChanged -= ResourceHelper_LanguageChanged;
		base.OnDetachedFromVisualTree(e);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void ResourceHelper_LanguageChanged(object? sender, System.EventArgs e) {
		UpdateLanguageRootHeader();
		RebuildLanguageMenuItems();
		RefreshLanguageCheckStates();
	}

	private void UpdateLanguageRootHeader() {
		string languageText = ResourceHelper.GetViewLabel("MainMenuView", "mnuLanguageSelector");
		_mnuLanguageRoot.Header = "🌐 " + languageText;
	}

	private static UiLanguage GetUiLanguageFromCode(string languageCode) {
		return languageCode switch {
			"en" => UiLanguage.English,
			"es" => UiLanguage.Spanish,
			"ja" => UiLanguage.Japanese,
			_ => UiLanguage.English,
		};
	}

	private static string GetLanguageMenuText(string languageCode) {
		try {
			CultureInfo culture = CultureInfo.GetCultureInfo(languageCode);
			return culture.NativeName + " (" + culture.EnglishName + ")";
		} catch (CultureNotFoundException) {
			return languageCode;
		}
	}

	private static string GetLocalizedLanguageTextOrFallback(string languageCode) {
		return languageCode switch {
			"en" => ResourceHelper.GetViewLabel("MainMenuView", "mnuLanguageEnglish"),
			"es" => ResourceHelper.GetViewLabel("MainMenuView", "mnuLanguageSpanish"),
			"ja" => ResourceHelper.GetViewLabel("MainMenuView", "mnuLanguageJapanese"),
			_ => GetLanguageMenuText(languageCode),
		};
	}

	private void RebuildLanguageMenuItems() {
		_languageItems.Clear();
		UpdateLanguageRootHeader();
		IList items = (IList)_mnuLanguageRoot.Items;
		items.Clear();

		string[] languageCodes = ResourceHelper.GetAvailableLanguageCodes();
		foreach (string languageCode in languageCodes.OrderBy(code => code, System.StringComparer.OrdinalIgnoreCase)) {
			UiLanguage uiLanguage = GetUiLanguageFromCode(languageCode);
			if (_languageItems.ContainsKey(uiLanguage)) {
				continue;
			}

			MenuItem item = new MenuItem {
				Header = GetLocalizedLanguageTextOrFallback(languageCode),
				ToggleType = MenuItemToggleType.CheckBox,
			};
			item.Click += (_, _) => {
				ApplyUiLanguage(uiLanguage);
				RefreshLanguageCheckStates();
			};

			_languageItems[uiLanguage] = item;
			items.Add(item);
		}
	}

	private void RefreshLanguageCheckStates() {
		UiLanguage language = ConfigManager.Config.Preferences.UiLanguage;
		foreach ((UiLanguage itemLanguage, MenuItem item) in _languageItems) {
			item.IsChecked = itemLanguage == language;
		}
	}

	private static void ApplyUiLanguage(UiLanguage language) {
		if (ConfigManager.Config.Preferences.UiLanguage == language) {
			return;
		}

		ConfigManager.Config.Preferences.UiLanguage = language;
		ResourceHelper.LoadResources(PreferencesConfig.GetLanguageCode(language));
		ConfigManager.Config.Save();
	}

	private void mnuTools_Opened(object sender, RoutedEventArgs e) {
		if (DataContext is MainMenuViewModel model) {
			if (model.UpdateNetplayMenu() && e.Source is MenuItem item) {
				//Force a refresh of the tools menu to ensure
				//the "Select controller" submenu gets updated
				IEnumerable? items = item.ItemsSource;
				item.ItemsSource = null;
				item.ItemsSource = items;
			}
		}
	}
}
