using System.IO;
using System.Text.RegularExpressions;
using Xunit;

namespace Nexen.Tests.UI;

/// <summary>
/// Guards against accidental removal of explicit scroll containers from dense UI surfaces.
/// These are lightweight markup tests until broader UI automation coverage lands.
/// </summary>
public sealed class UiScrollabilityMarkupTests {
	[Theory]
	[InlineData("UI/Debugger/Windows/DebuggerConfigWindow.axaml")]
	[InlineData("UI/Windows/SetupWizardWindow.axaml")]
	public void DenseUiSurface_ContainsScrollViewer(string relativePath) {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, relativePath.Replace('/', Path.DirectorySeparatorChar));

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("<ScrollViewer", markup);
	}

	[Fact]
	public void SetupWizardWindow_IsResizableAndHasMinimumSize() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "SetupWizardWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"900\"", markup);
		Assert.Contains("MinHeight=\"900\"", markup);
		int minHeightTouchTargetCount = Regex.Matches(markup, "MinHeight=\"(42|54|66)\"").Count;
		Assert.True(minHeightTouchTargetCount >= 3, $"Expected at least 3 touch-target controls with MinHeight >= 42, found {minHeightTouchTargetCount}.");
	}

	[Fact]
	public void ConfigWindow_IsResizableAndUsesLargerTouchTargets() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"700\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
		Assert.Contains("MinHeight=\"36\"", markup);
		Assert.Contains("Name=\"SettingsTabControl\"", markup);
		Assert.Contains("Background=\"{DynamicResource SettingsTabStripBackgroundBrush}\"", markup);
		Assert.DoesNotContain("Background=\"#20888888\"", markup);
	}

	[Fact]
	public void DebuggerConfigWindow_IsResizableAndUsesLargerTouchTargets() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("CanResize=\"True\"", markup);
		Assert.Contains("MinWidth=\"700\"", markup);
		Assert.Contains("MinHeight=\"560\"", markup);
		Assert.Contains("MinHeight=\"36\"", markup);
		Assert.Contains("Name=\"SettingsTabControl\"", markup);
		Assert.Contains("Background=\"{DynamicResource SettingsTabStripBackgroundBrush}\"", markup);
		Assert.DoesNotContain("Background=\"#20888888\"", markup);
	}

	[Fact]
	public void ThemeStyles_DefineSettingsTabStripBackgroundToken() {
		string repoRoot = GetRepositoryRoot();
		string lightPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Light.xaml");
		string darkPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Dark.xaml");

		Assert.True(File.Exists(lightPath), $"Expected style file to exist: {lightPath}");
		Assert.True(File.Exists(darkPath), $"Expected style file to exist: {darkPath}");

		string lightMarkup = File.ReadAllText(lightPath);
		string darkMarkup = File.ReadAllText(darkPath);

		Assert.Contains("SettingsTabStripBackgroundColor", lightMarkup);
		Assert.Contains("SettingsTabStripBackgroundBrush", lightMarkup);
		Assert.Contains("SettingsTabStripBackgroundColor", darkMarkup);
		Assert.Contains("SettingsTabStripBackgroundBrush", darkMarkup);
	}

	[Fact]
	public void ConfigWindow_WiresResizePersistence() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml.cs");

		Assert.True(File.Exists(fullPath), $"Expected code-behind file to exist: {fullPath}");

		string source = File.ReadAllText(fullPath);
		Assert.Contains("ConfigManager.Config.ConfigWindow.LoadWindowSettings(this);", source);
		Assert.Contains("ConfigManager.Config.ConfigWindow.SaveWindowSettings(this);", source);
		Assert.Contains("CompactTabBreakpointWidth = 700", source);
		Assert.Contains("TabStripPlacement = ClientSize.Width < CompactTabBreakpointWidth ? Avalonia.Controls.Dock.Top : Avalonia.Controls.Dock.Left;", source);
		Assert.Contains("NexenMsgBox.Show(this, \"ResetSettingsConfirmation\"", source);
	}

	[Fact]
	public void DebuggerConfigWindow_WiresResizePersistence() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml.cs");

		Assert.True(File.Exists(fullPath), $"Expected code-behind file to exist: {fullPath}");

		string source = File.ReadAllText(fullPath);
		Assert.Contains("ConfigManager.Config.Debug.DebuggerConfigWindow.LoadWindowSettings(this);", source);
		Assert.Contains("ConfigManager.Config.Debug.DebuggerConfigWindow.SaveWindowSettings(this);", source);
		Assert.Contains("CompactTabBreakpointWidth = 900", source);
		Assert.Contains("TabStripPlacement = ClientSize.Width < CompactTabBreakpointWidth ? Avalonia.Controls.Dock.Top : Avalonia.Controls.Dock.Left;", source);
	}

	[Fact]
	public void DebuggerConfigWindow_IntegrationTabContainsClarificationHints() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("{l:Translate lblIntegrationDefaultsHint}", markup);
		Assert.Contains("{l:Translate lblAutoExportHint}", markup);
		Assert.Contains("{l:Translate lblPansyLifecycleHint}", markup);
		Assert.Contains("{l:Translate lblPansyCompressionHint}", markup);
	}

	[Fact]
	public void DebuggerAuxiliaryWindows_UseLargerTouchTargets() {
		string repoRoot = GetRepositoryRoot();
		string memoryFindPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "MemoryViewerFindWindow.axaml");
		string pansyProgressPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "PansyExportProgressWindow.axaml");
		string debuggerWindowPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerWindow.axaml");

		Assert.True(File.Exists(memoryFindPath), $"Expected markup file to exist: {memoryFindPath}");
		Assert.True(File.Exists(pansyProgressPath), $"Expected markup file to exist: {pansyProgressPath}");
		Assert.True(File.Exists(debuggerWindowPath), $"Expected markup file to exist: {debuggerWindowPath}");

		string memoryFindMarkup = File.ReadAllText(memoryFindPath);
		string pansyProgressMarkup = File.ReadAllText(pansyProgressPath);
		string debuggerWindowMarkup = File.ReadAllText(debuggerWindowPath);

		Assert.Contains("Height=\"36\"", memoryFindMarkup);
		Assert.Contains("MinHeight=\"36\"", memoryFindMarkup);
		Assert.Contains("Height=\"24\"", pansyProgressMarkup);
		Assert.Contains("Height=\"12\"", pansyProgressMarkup);
		Assert.Contains("MinHeight=\"30\"", debuggerWindowMarkup);
	}

	[Fact]
	public void ConfigWindow_DataTemplatesUseExplicitScrollViewers() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		int scrollViewerCount = Regex.Matches(markup, "<ScrollViewer AllowAutoHide=\"False\" HorizontalScrollBarVisibility=\"Auto\" Padding=\"0 0 2 0\">").Count;
		Assert.True(scrollViewerCount >= 16, $"Expected at least 16 standardized ScrollViewer wrappers, found {scrollViewerCount}.");
	}

	[Fact]
	public void SettingsWindows_TabHeadersKeepIconCoverage() {
		string repoRoot = GetRepositoryRoot();
		string configPath = Path.Combine(repoRoot, "UI", "Windows", "ConfigWindow.axaml");
		string debuggerPath = Path.Combine(repoRoot, "UI", "Debugger", "Windows", "DebuggerConfigWindow.axaml");

		Assert.True(File.Exists(configPath), $"Expected markup file to exist: {configPath}");
		Assert.True(File.Exists(debuggerPath), $"Expected markup file to exist: {debuggerPath}");

		string configMarkup = File.ReadAllText(configPath);
		string debuggerMarkup = File.ReadAllText(debuggerPath);

		int configHeaderCount = Regex.Matches(configMarkup, "<TabItem.Header>").Count;
		int configHeaderImages = Regex.Matches(configMarkup, "<TabItem.Header>[\\s\\S]*?<Image ").Count;
		int debuggerHeaderCount = Regex.Matches(debuggerMarkup, "<TabItem.Header>").Count;
		int debuggerHeaderImages = Regex.Matches(debuggerMarkup, "<TabItem.Header>[\\s\\S]*?<Image ").Count;

		Assert.True(configHeaderCount >= 18, $"Expected at least 18 ConfigWindow tab headers, found {configHeaderCount}.");
		Assert.True(configHeaderImages >= 16, $"Expected at least 16 ConfigWindow iconized headers, found {configHeaderImages}.");
		Assert.True(debuggerHeaderCount >= 7, $"Expected at least 7 DebuggerConfigWindow tab headers, found {debuggerHeaderCount}.");
		Assert.True(debuggerHeaderImages >= 5, $"Expected at least 5 DebuggerConfigWindow iconized headers, found {debuggerHeaderImages}.");

		Assert.DoesNotContain("<TabItem.Header>\r\n\t\t\t\t\t<TextBlock", configMarkup);
		Assert.DoesNotContain("<TabItem.Header>\r\n\t\t\t\t\t<TextBlock", debuggerMarkup);
	}

	[Fact]
	public void PreferencesView_ContainsThemeSelector() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Views", "PreferencesConfigView.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		Assert.Contains("Text=\"{l:Translate lblTheme}\"", markup);
		Assert.Contains("SelectedItem=\"{Binding Config.Theme}\"", markup);
		Assert.Contains("lblThemeMenuBackground", markup);
		Assert.Contains("lblThemeMenuHighlight", markup);
		Assert.Contains("lblThemeAccentColor", markup);
		Assert.Contains("btnPickThemeMenuBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeMenuHighlightColor_OnClick", markup);
		Assert.Contains("btnPickThemeAccentColor_OnClick", markup);
		Assert.Contains("lblThemeControlHoverBackground", markup);
		Assert.Contains("lblThemeControlPressedBackground", markup);
		Assert.Contains("lblThemeControlHoverBorder", markup);
		Assert.Contains("lblThemeControlPressedBorder", markup);
		Assert.Contains("lblThemeSidebarBorderColor", markup);
		Assert.Contains("lblThemeDockTabStripBackground", markup);
		Assert.Contains("lblThemeDockTabHoverBackground", markup);
		Assert.Contains("lblThemeDockTabActiveBackground", markup);
		Assert.Contains("lblThemeDockTabActiveBorder", markup);
		Assert.Contains("lblThemeCheckBoxHoverBorder", markup);
		Assert.Contains("lblThemeCheckBoxPressedBackground", markup);
		Assert.Contains("lblThemeCheckBoxPressedBorder", markup);
		Assert.Contains("lblThemeRadioButtonHoverBorder", markup);
		Assert.Contains("lblThemeRadioButtonPressedBackground", markup);
		Assert.Contains("lblThemeRadioButtonPressedBorder", markup);
		Assert.Contains("lblThemeSliderHoverTrack", markup);
		Assert.Contains("lblThemeSliderPressedTrack", markup);
		Assert.Contains("lblThemeTextSelection", markup);
		Assert.Contains("lblThemeTextControlDisabledBackground", markup);
		Assert.Contains("lblThemeToolTipBackground", markup);
		Assert.Contains("lblThemeMenuFlyoutBackground", markup);
		Assert.Contains("lblThemeMenuFlyoutBorder", markup);
		Assert.Contains("lblThemeGroupStartup", markup);
		Assert.Contains("lblThemeGroupChrome", markup);
		Assert.Contains("lblThemeGroupControlStates", markup);
		Assert.Contains("lblThemeGroupInputsAndFlyouts", markup);
		Assert.Contains("lblThemeGroupComboAndDataGrid", markup);
		Assert.Contains("lblThemeGroupListAndTree", markup);
		Assert.Contains("lblThemeGroupNavigationAndListView", markup);
		Assert.Contains("lblThemeComboBoxDropDownBackground", markup);
		Assert.Contains("lblThemeComboBoxDropDownBorder", markup);
		Assert.Contains("lblThemeDataGridHeaderBackground", markup);
		Assert.Contains("lblThemeDataGridHeaderForeground", markup);
		Assert.Contains("lblThemeDataGridSelectedRowForeground", markup);
		Assert.Contains("lblThemeListBoxHoverBackground", markup);
		Assert.Contains("lblThemeListBoxSelectedBackground", markup);
		Assert.Contains("lblThemeListBoxSelectedForeground", markup);
		Assert.Contains("lblThemeTreeViewHoverBackground", markup);
		Assert.Contains("lblThemeTreeViewSelectedBackground", markup);
		Assert.Contains("lblThemeTreeViewSelectedForeground", markup);
		Assert.Contains("lblThemeNavigationViewHoverBackground", markup);
		Assert.Contains("lblThemeNavigationViewSelectedBackground", markup);
		Assert.Contains("lblThemeNavigationViewSelectedForeground", markup);
		Assert.Contains("lblThemeListViewHoverBackground", markup);
		Assert.Contains("lblThemeListViewSelectedBackground", markup);
		Assert.Contains("lblThemeListViewSelectedForeground", markup);
		Assert.Contains("btnPickThemeControlHoverBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeControlPressedBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeControlHoverBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeControlPressedBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeSidebarBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeDockTabStripBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeDockTabHoverColor_OnClick", markup);
		Assert.Contains("btnPickThemeDockTabActiveBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeDockTabActiveBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeCheckBoxHoverBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeCheckBoxPressedBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeCheckBoxPressedBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeRadioButtonHoverBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeRadioButtonPressedBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeRadioButtonPressedBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeSliderHoverTrackColor_OnClick", markup);
		Assert.Contains("btnPickThemeSliderPressedTrackColor_OnClick", markup);
		Assert.Contains("btnPickThemeTextSelectionColor_OnClick", markup);
		Assert.Contains("btnPickThemeTextControlDisabledBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeToolTipBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeMenuFlyoutBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeMenuFlyoutBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeComboBoxDropDownBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeComboBoxDropDownBorderColor_OnClick", markup);
		Assert.Contains("btnPickThemeDataGridHeaderBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeDataGridHeaderForegroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeDataGridSelectedRowForegroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeListBoxHoverBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeListBoxSelectedBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeListBoxSelectedForegroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeTreeViewHoverBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeTreeViewSelectedBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeTreeViewSelectedForegroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeNavigationViewHoverBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeNavigationViewSelectedBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeNavigationViewSelectedForegroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeListViewHoverBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeListViewSelectedBackgroundColor_OnClick", markup);
		Assert.Contains("btnPickThemeListViewSelectedForegroundColor_OnClick", markup);
	}

	[Fact]
	public void PreferencesView_ThemeGroups_AppearInExpectedSemanticOrder() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Views", "PreferencesConfigView.axaml");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");

		string markup = File.ReadAllText(fullPath);
		int startupIndex = markup.IndexOf("lblThemeGroupStartup", StringComparison.Ordinal);
		int chromeIndex = markup.IndexOf("lblThemeGroupChrome", StringComparison.Ordinal);
		int controlStatesIndex = markup.IndexOf("lblThemeGroupControlStates", StringComparison.Ordinal);
		int inputsFlyoutsIndex = markup.IndexOf("lblThemeGroupInputsAndFlyouts", StringComparison.Ordinal);
		int comboDataGridIndex = markup.IndexOf("lblThemeGroupComboAndDataGrid", StringComparison.Ordinal);
		int listTreeIndex = markup.IndexOf("lblThemeGroupListAndTree", StringComparison.Ordinal);
		int navigationListViewIndex = markup.IndexOf("lblThemeGroupNavigationAndListView", StringComparison.Ordinal);

		Assert.True(startupIndex >= 0, "Expected startup group header to exist.");
		Assert.True(chromeIndex > startupIndex, "Expected chrome group after startup.");
		Assert.True(controlStatesIndex > chromeIndex, "Expected control states group after chrome.");
		Assert.True(inputsFlyoutsIndex > controlStatesIndex, "Expected inputs/flyouts group after control states.");
		Assert.True(comboDataGridIndex > inputsFlyoutsIndex, "Expected combo/datagrid group after inputs/flyouts.");
		Assert.True(listTreeIndex > comboDataGridIndex, "Expected list/tree group after combo/datagrid.");
		Assert.True(navigationListViewIndex > listTreeIndex, "Expected navigation/listview group after list/tree.");
	}

	[Fact]
	public void ThemeStyles_DefineSemanticListTreeNavigationAndListViewKeys_InLightAndDarkThemes() {
		string repoRoot = GetRepositoryRoot();
		string lightPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Light.xaml");
		string darkPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Dark.xaml");

		Assert.True(File.Exists(lightPath), $"Expected style file to exist: {lightPath}");
		Assert.True(File.Exists(darkPath), $"Expected style file to exist: {darkPath}");

		string lightMarkup = File.ReadAllText(lightPath);
		string darkMarkup = File.ReadAllText(darkPath);

		string[] requiredKeys = [
			"ListBoxItemBackgroundPointerOver",
			"ListBoxItemBackgroundSelected",
			"ListBoxItemForegroundSelected",
			"TreeViewItemBackgroundPointerOver",
			"TreeViewItemBackgroundSelected",
			"TreeViewItemForegroundSelected",
			"NavigationViewItemBackgroundPointerOver",
			"NavigationViewItemBackgroundSelected",
			"NavigationViewItemForegroundSelected",
			"ListViewItemBackgroundPointerOver",
			"ListViewItemBackgroundSelected",
			"ListViewItemForegroundSelected"
		];

		foreach (string key in requiredKeys) {
			Assert.Contains(key, lightMarkup);
			Assert.Contains(key, darkMarkup);
		}
	}

	[Fact]
	public void NexenThemeManager_ContainsSemanticOverrideMappings_ForAllCurrentFamilies() {
		string repoRoot = GetRepositoryRoot();
		string managerPath = Path.Combine(repoRoot, "UI", "Utilities", "NexenThemeManager.cs");

		Assert.True(File.Exists(managerPath), $"Expected source file to exist: {managerPath}");

		string source = File.ReadAllText(managerPath);
		Assert.Contains("ApplyColorOverride(app, \"ComboBoxDropDownBackground\"", source);
		Assert.Contains("ApplyColorOverride(app, \"ComboBoxDropDownBorderBrush\"", source);
		Assert.Contains("ApplyBrushOverride(app, \"DataGridColumnHeaderBackgroundBrush\"", source);
		Assert.Contains("ApplyBrushOverride(app, \"DataGridColumnHeaderForegroundBrush\"", source);
		Assert.Contains("ApplyBrushOverride(app, \"DataGridRowSelectedForegroundBrush\"", source);
		Assert.Contains("ApplyColorOverride(app, \"ListBoxItemBackgroundPointerOver\"", source);
		Assert.Contains("ApplyColorOverride(app, \"ListBoxItemBackgroundSelected\"", source);
		Assert.Contains("ApplyColorOverride(app, \"ListBoxItemForegroundSelected\"", source);
		Assert.Contains("ApplyColorOverride(app, \"TreeViewItemBackgroundPointerOver\"", source);
		Assert.Contains("ApplyColorOverride(app, \"TreeViewItemBackgroundSelected\"", source);
		Assert.Contains("ApplyColorOverride(app, \"TreeViewItemForegroundSelected\"", source);
		Assert.Contains("ApplyColorOverride(app, \"NavigationViewItemBackgroundPointerOver\"", source);
		Assert.Contains("ApplyColorOverride(app, \"NavigationViewItemBackgroundSelected\"", source);
		Assert.Contains("ApplyColorOverride(app, \"NavigationViewItemForegroundSelected\"", source);
		Assert.Contains("ApplyColorOverride(app, \"ListViewItemBackgroundPointerOver\"", source);
		Assert.Contains("ApplyColorOverride(app, \"ListViewItemBackgroundSelected\"", source);
		Assert.Contains("ApplyColorOverride(app, \"ListViewItemForegroundSelected\"", source);
	}

	[Fact]
	public void PreferencesThemeCustomization_WiresSemanticBindingsAndHandlers_ForNavigationAndListView() {
		string repoRoot = GetRepositoryRoot();
		string vmPath = Path.Combine(repoRoot, "UI", "ViewModels", "PreferencesConfigViewModel.cs");
		string viewPath = Path.Combine(repoRoot, "UI", "Views", "PreferencesConfigView.axaml");
		string codeBehindPath = Path.Combine(repoRoot, "UI", "Views", "PreferencesConfigView.axaml.cs");

		Assert.True(File.Exists(vmPath), $"Expected source file to exist: {vmPath}");
		Assert.True(File.Exists(viewPath), $"Expected markup file to exist: {viewPath}");
		Assert.True(File.Exists(codeBehindPath), $"Expected code-behind file to exist: {codeBehindPath}");

		string vmSource = File.ReadAllText(vmPath);
		string viewMarkup = File.ReadAllText(viewPath);
		string codeBehindSource = File.ReadAllText(codeBehindPath);

		Assert.Contains("SetSelectedProfileNavigationViewAndListViewSemanticColors", vmSource);
		Assert.Contains("SelectedProfileNavigationViewHoverBackgroundColor", vmSource);
		Assert.Contains("SelectedProfileNavigationViewSelectedBackgroundColor", vmSource);
		Assert.Contains("SelectedProfileNavigationViewSelectedForegroundColor", vmSource);
		Assert.Contains("SelectedProfileListViewHoverBackgroundColor", vmSource);
		Assert.Contains("SelectedProfileListViewSelectedBackgroundColor", vmSource);
		Assert.Contains("SelectedProfileListViewSelectedForegroundColor", vmSource);

		Assert.Contains("{Binding SelectedProfileNavigationViewHoverBackgroundColor}", viewMarkup);
		Assert.Contains("{Binding SelectedProfileNavigationViewSelectedBackgroundColor}", viewMarkup);
		Assert.Contains("{Binding SelectedProfileNavigationViewSelectedForegroundColor}", viewMarkup);
		Assert.Contains("{Binding SelectedProfileListViewHoverBackgroundColor}", viewMarkup);
		Assert.Contains("{Binding SelectedProfileListViewSelectedBackgroundColor}", viewMarkup);
		Assert.Contains("{Binding SelectedProfileListViewSelectedForegroundColor}", viewMarkup);

		Assert.Contains("btnPickThemeNavigationViewHoverBackgroundColor_OnClick", codeBehindSource);
		Assert.Contains("btnPickThemeNavigationViewSelectedBackgroundColor_OnClick", codeBehindSource);
		Assert.Contains("btnPickThemeNavigationViewSelectedForegroundColor_OnClick", codeBehindSource);
		Assert.Contains("btnPickThemeListViewHoverBackgroundColor_OnClick", codeBehindSource);
		Assert.Contains("btnPickThemeListViewSelectedBackgroundColor_OnClick", codeBehindSource);
		Assert.Contains("btnPickThemeListViewSelectedForegroundColor_OnClick", codeBehindSource);
	}

	[Fact]
	public void MainMenuView_ContainsRightAlignedGlobeLanguageMenu() {
		string repoRoot = GetRepositoryRoot();
		string fullPath = Path.Combine(repoRoot, "UI", "Views", "MainMenuView.axaml");
		string codeBehindPath = Path.Combine(repoRoot, "UI", "Views", "MainMenuView.axaml.cs");

		Assert.True(File.Exists(fullPath), $"Expected markup file to exist: {fullPath}");
		Assert.True(File.Exists(codeBehindPath), $"Expected code-behind file to exist: {codeBehindPath}");

		string markup = File.ReadAllText(fullPath);
		string codeBehind = File.ReadAllText(codeBehindPath);
		Assert.Contains("Name=\"LanguageMenu\"", markup);
		Assert.Contains("DockPanel.Dock=\"Right\"", markup);
		Assert.Contains("Name=\"mnuLanguageRoot\"", markup);
		Assert.DoesNotContain("Header=\"🌐\"", markup);
		Assert.Contains("\"🌐 \" + languageText", codeBehind);
		Assert.Contains("GetLocalizedLanguageTextOrFallback", codeBehind);
		Assert.DoesNotContain("mnuLanguageEnglish", markup);
		Assert.DoesNotContain("mnuLanguageSpanish", markup);
		Assert.DoesNotContain("mnuLanguageJapanese", markup);
	}

	[Fact]
	public void MenuThemeTokens_DefaultToBrownOrangePalette() {
		string repoRoot = GetRepositoryRoot();
		string lightPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Light.xaml");
		string darkPath = Path.Combine(repoRoot, "UI", "Styles", "NexenStyles.Dark.xaml");

		Assert.True(File.Exists(lightPath), $"Expected style file to exist: {lightPath}");
		Assert.True(File.Exists(darkPath), $"Expected style file to exist: {darkPath}");

		string lightMarkup = File.ReadAllText(lightPath);
		string darkMarkup = File.ReadAllText(darkPath);

		Assert.Contains("<Color x:Key=\"NexenMenuBackground\">#f3e5d7</Color>", lightMarkup);
		Assert.Contains("<Color x:Key=\"NexenMenuBorderHighlight\">#9f5c22</Color>", lightMarkup);
		Assert.Contains("<Color x:Key=\"ButtonBorderBrushPointerOver\">#9f5c22</Color>", lightMarkup);
		Assert.Contains("<Color x:Key=\"NexenMenuBackground\">#55281a10</Color>", darkMarkup);
		Assert.Contains("<Color x:Key=\"ThemeAccentColor\">#ccc87423</Color>", darkMarkup);
		Assert.Contains("<Color x:Key=\"HighlightColor\">#ffc87423</Color>", darkMarkup);
		Assert.Contains("<Color x:Key=\"SettingsTabStripBorderColor\">#80808080</Color>", lightMarkup);
		Assert.Contains("<Color x:Key=\"NexenDockTabPointerOverColor\">#40808080</Color>", lightMarkup);
		Assert.Contains("<Color x:Key=\"SettingsTabStripBorderColor\">#80808080</Color>", darkMarkup);
		Assert.Contains("<Color x:Key=\"NexenDockTabActiveBorderColor\">#ff505050</Color>", darkMarkup);
	}

	[Fact]
	public void DialogUtilities_UseResolvedParentWindowAndDoNotThrowInvalidParent() {
		string repoRoot = GetRepositoryRoot();
		string msgBoxPath = Path.Combine(repoRoot, "UI", "Utilities", "NexenMsgBox.cs");
		string fileDialogPath = Path.Combine(repoRoot, "UI", "Utilities", "FileDialogHelper.cs");

		Assert.True(File.Exists(msgBoxPath), $"Expected source file to exist: {msgBoxPath}");
		Assert.True(File.Exists(fileDialogPath), $"Expected source file to exist: {fileDialogPath}");

		string msgBoxSource = File.ReadAllText(msgBoxPath);
		string fileDialogSource = File.ReadAllText(fileDialogPath);

		Assert.Contains("ApplicationHelper.ResolveParentWindow(parent)", msgBoxSource);
		Assert.Contains("ApplicationHelper.ResolveParentWindow(parent)", fileDialogSource);
		Assert.DoesNotContain("throw new Exception(\"Invalid parent window\")", msgBoxSource);
		Assert.DoesNotContain("throw new Exception(\"Invalid parent window\")", fileDialogSource);
	}

	[Fact]
	public void FirmwareAndPaletteControls_AvoidVisualRootForMessageDialogs() {
		string repoRoot = GetRepositoryRoot();
		string firmwarePath = Path.Combine(repoRoot, "UI", "Controls", "FirmwareSelect.axaml.cs");
		string palettePath = Path.Combine(repoRoot, "UI", "Controls", "PaletteConfig.axaml.cs");

		Assert.True(File.Exists(firmwarePath), $"Expected source file to exist: {firmwarePath}");
		Assert.True(File.Exists(palettePath), $"Expected source file to exist: {palettePath}");

		string firmwareSource = File.ReadAllText(firmwarePath);
		string paletteSource = File.ReadAllText(palettePath);

		Assert.Contains("NexenMsgBox.Show(this, \"PromptDeleteFirmware\"", firmwareSource);
		Assert.DoesNotContain("NexenMsgBox.Show(VisualRoot, \"PromptDeleteFirmware\"", firmwareSource);
		Assert.Contains("NexenMsgBox.Show(this, \"InvalidPaletteFile\"", paletteSource);
		Assert.DoesNotContain("NexenMsgBox.Show(VisualRoot, \"InvalidPaletteFile\"", paletteSource);
	}

	private static string GetRepositoryRoot() {
		string? current = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));

		while (!string.IsNullOrEmpty(current)) {
			if (File.Exists(Path.Combine(current, "Nexen.sln"))) {
				return current;
			}

			string? parent = Directory.GetParent(current)?.FullName;
			if (string.Equals(parent, current, System.StringComparison.OrdinalIgnoreCase)) {
				break;
			}
			current = parent;
		}

		throw new DirectoryNotFoundException("Could not locate Nexen repository root.");
	}
}
