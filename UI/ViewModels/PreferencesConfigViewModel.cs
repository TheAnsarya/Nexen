using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Styling;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Localization;
using Nexen.Utilities;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the preferences configuration tab.
/// </summary>
public sealed partial class PreferencesConfigViewModel : DisposableViewModel {
	private UiLanguage _lastUiLanguage;

	/// <summary>Gets or sets the current preferences configuration.</summary>
	[Reactive] public partial PreferencesConfig Config { get; set; }

	/// <summary>Gets or sets the original preferences configuration for revert.</summary>
	[Reactive] public partial PreferencesConfig OriginalConfig { get; set; }

	/// <summary>Gets the data storage location path.</summary>
	public string DataStorageLocation { get; }

	/// <summary>Gets a comma-separated list of bundled UI languages.</summary>
	public string AvailableLanguagesSummary => string.Join(", ", ResourceHelper.GetAvailableLanguageDisplayNames());

	/// <summary>Gets whether the current platform is macOS.</summary>
	public bool IsOsx { get; }

	/// <summary>Gets or sets the list of shortcut key bindings.</summary>
	public List<ShortcutKeyInfo> ShortcutKeys { get; set; }

	/// <summary>Gets or sets available theme profile names.</summary>
	[Reactive] public partial List<string> ThemeProfileNames { get; set; }

	/// <summary>Gets or sets the selected theme profile name.</summary>
	[Reactive] public partial string SelectedThemeProfileName { get; set; }

	/// <summary>Gets or sets selected profile startup background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileStartupBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile startup text color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileStartupTextColor { get; set; }

	/// <summary>Gets or sets selected profile primary action color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileStartupPrimaryActionColor { get; set; }

	/// <summary>Gets or sets selected profile startup background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileStartupBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile startup text preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileStartupTextBrush { get; set; }

	/// <summary>Gets or sets selected profile startup primary action preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileStartupPrimaryActionBrush { get; set; }

	/// <summary>Gets or sets selected profile menu background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileMenuBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile menu highlight color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileMenuHighlightColor { get; set; }

	/// <summary>Gets or sets selected profile accent color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileAccentColor { get; set; }

	/// <summary>Gets or sets selected profile menu background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileMenuBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile menu highlight preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileMenuHighlightBrush { get; set; }

	/// <summary>Gets or sets selected profile accent preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileAccentBrush { get; set; }

	/// <summary>Gets or sets selected profile control hover background as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileControlHoverBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile control pressed background as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileControlPressedBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile control hover border as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileControlHoverBorderColor { get; set; }

	/// <summary>Gets or sets selected profile control pressed border as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileControlPressedBorderColor { get; set; }

	/// <summary>Gets or sets selected profile control hover background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileControlHoverBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile control pressed background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileControlPressedBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile control hover border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileControlHoverBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile control pressed border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileControlPressedBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile sidebar border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileSidebarBorderColor { get; set; }

	/// <summary>Gets or sets selected profile dock tab strip background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileDockTabStripBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile dock tab hover color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileDockTabHoverColor { get; set; }

	/// <summary>Gets or sets selected profile dock tab active background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileDockTabActiveBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile dock tab active border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileDockTabActiveBorderColor { get; set; }

	/// <summary>Gets or sets selected profile checkbox hover border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileCheckBoxHoverBorderColor { get; set; }

	/// <summary>Gets or sets selected profile checkbox pressed background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileCheckBoxPressedBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile checkbox pressed border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileCheckBoxPressedBorderColor { get; set; }

	/// <summary>Gets or sets selected profile radio button hover border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileRadioButtonHoverBorderColor { get; set; }

	/// <summary>Gets or sets selected profile radio button pressed background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileRadioButtonPressedBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile radio button pressed border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileRadioButtonPressedBorderColor { get; set; }

	/// <summary>Gets or sets selected profile slider hover track color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileSliderHoverTrackColor { get; set; }

	/// <summary>Gets or sets selected profile slider pressed track color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileSliderPressedTrackColor { get; set; }

	/// <summary>Gets or sets selected profile text selection color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileTextSelectionColor { get; set; }

	/// <summary>Gets or sets selected profile disabled text-control background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileTextControlDisabledBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile tooltip background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileToolTipBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile menu flyout background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileMenuFlyoutBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile menu flyout border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileMenuFlyoutBorderColor { get; set; }

	/// <summary>Gets or sets selected profile combo dropdown background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileComboBoxDropDownBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile combo dropdown border color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileComboBoxDropDownBorderColor { get; set; }

	/// <summary>Gets or sets selected profile datagrid header background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileDataGridHeaderBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile datagrid header foreground color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileDataGridHeaderForegroundColor { get; set; }

	/// <summary>Gets or sets selected profile datagrid selected-row foreground color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileDataGridSelectedRowForegroundColor { get; set; }

	/// <summary>Gets or sets selected profile listbox hover background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileListBoxHoverBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile listbox selected background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileListBoxSelectedBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile listbox selected foreground color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileListBoxSelectedForegroundColor { get; set; }

	/// <summary>Gets or sets selected profile treeview hover background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileTreeViewHoverBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile treeview selected background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileTreeViewSelectedBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile treeview selected foreground color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileTreeViewSelectedForegroundColor { get; set; }

	/// <summary>Gets or sets selected profile navigationview hover background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileNavigationViewHoverBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile navigationview selected background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileNavigationViewSelectedBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile navigationview selected foreground color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileNavigationViewSelectedForegroundColor { get; set; }

	/// <summary>Gets or sets selected profile listview hover background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileListViewHoverBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile listview selected background color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileListViewSelectedBackgroundColor { get; set; }

	/// <summary>Gets or sets selected profile listview selected foreground color as ARGB hex.</summary>
	[Reactive] public partial string SelectedProfileListViewSelectedForegroundColor { get; set; }

	/// <summary>Gets or sets selected profile sidebar border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileSidebarBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile dock tab strip background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileDockTabStripBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile dock tab hover preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileDockTabHoverBrush { get; set; }

	/// <summary>Gets or sets selected profile dock tab active background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileDockTabActiveBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile dock tab active border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileDockTabActiveBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile checkbox hover border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileCheckBoxHoverBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile checkbox pressed background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileCheckBoxPressedBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile checkbox pressed border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileCheckBoxPressedBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile radio button hover border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileRadioButtonHoverBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile radio button pressed background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileRadioButtonPressedBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile radio button pressed border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileRadioButtonPressedBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile slider hover track preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileSliderHoverTrackBrush { get; set; }

	/// <summary>Gets or sets selected profile slider pressed track preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileSliderPressedTrackBrush { get; set; }

	/// <summary>Gets or sets selected profile text selection preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileTextSelectionBrush { get; set; }

	/// <summary>Gets or sets selected profile disabled text-control background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileTextControlDisabledBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile tooltip background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileToolTipBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile menu flyout background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileMenuFlyoutBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile menu flyout border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileMenuFlyoutBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile combo dropdown background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileComboBoxDropDownBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile combo dropdown border preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileComboBoxDropDownBorderBrush { get; set; }

	/// <summary>Gets or sets selected profile datagrid header background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileDataGridHeaderBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile datagrid header foreground preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileDataGridHeaderForegroundBrush { get; set; }

	/// <summary>Gets or sets selected profile datagrid selected-row foreground preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileDataGridSelectedRowForegroundBrush { get; set; }

	/// <summary>Gets or sets selected profile listbox hover background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileListBoxHoverBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile listbox selected background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileListBoxSelectedBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile listbox selected foreground preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileListBoxSelectedForegroundBrush { get; set; }

	/// <summary>Gets or sets selected profile treeview hover background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileTreeViewHoverBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile treeview selected background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileTreeViewSelectedBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile treeview selected foreground preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileTreeViewSelectedForegroundBrush { get; set; }

	/// <summary>Gets or sets selected profile navigationview hover background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileNavigationViewHoverBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile navigationview selected background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileNavigationViewSelectedBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile navigationview selected foreground preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileNavigationViewSelectedForegroundBrush { get; set; }

	/// <summary>Gets or sets selected profile listview hover background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileListViewHoverBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile listview selected background preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileListViewSelectedBackgroundBrush { get; set; }

	/// <summary>Gets or sets selected profile listview selected foreground preview brush.</summary>
	[Reactive] public partial IBrush SelectedProfileListViewSelectedForegroundBrush { get; set; }

	/// <summary>Gets or sets the number of tokens that differ from canonical defaults.</summary>
	[Reactive] public partial int SelectedThemeProfileDivergenceCount { get; set; }

	/// <summary>Gets or sets whether selected profile matches canonical defaults.</summary>
	[Reactive] public partial bool SelectedThemeProfileMatchesDefaults { get; set; }

	/// <summary>Gets or sets whether selected profile has customized tokens.</summary>
	[Reactive] public partial bool SelectedThemeProfileIsCustomized { get; set; }

	/// <summary>Gets or sets comma-separated customized token names.</summary>
	[Reactive] public partial string SelectedThemeProfileCustomizedTokensSummary { get; set; }

	/// <summary>Gets or sets customized font token summary.</summary>
	[Reactive] public partial string SelectedThemeProfileCustomizedFontTokensSummary { get; set; }

	/// <summary>Gets or sets customized startup token summary.</summary>
	[Reactive] public partial string SelectedThemeProfileCustomizedStartupTokensSummary { get; set; }

	/// <summary>Gets or sets customized setup token summary.</summary>
	[Reactive] public partial string SelectedThemeProfileCustomizedSetupTokensSummary { get; set; }

	/// <summary>Gets or sets whether customized font tokens are present.</summary>
	[Reactive] public partial bool HasCustomizedFontTokens { get; set; }

	/// <summary>Gets or sets whether customized startup tokens are present.</summary>
	[Reactive] public partial bool HasCustomizedStartupTokens { get; set; }

	/// <summary>Gets or sets whether customized setup tokens are present.</summary>
	[Reactive] public partial bool HasCustomizedSetupTokens { get; set; }

	/// <summary>Gets or sets target name for profile rename.</summary>
	[Reactive] public partial string RenameThemeProfileName { get; set; }

	/// <summary>Gets or sets target name for profile duplication.</summary>
	[Reactive] public partial string DuplicateThemeProfileName { get; set; }

	/// <summary>
	/// Initializes a new instance of the <see cref="PreferencesConfigViewModel"/> class.
	/// </summary>
	public PreferencesConfigViewModel() {
		Config = ConfigManager.Config.Preferences;
		_lastUiLanguage = Config.UiLanguage;
		OriginalConfig = Config.Clone();
		ThemeProfileNames = [];
		SelectedThemeProfileName = "";
		SelectedProfileStartupBackgroundColor = "";
		SelectedProfileStartupTextColor = "";
		SelectedProfileStartupPrimaryActionColor = "";
		SelectedProfileStartupBackgroundBrush = Brushes.Transparent;
		SelectedProfileStartupTextBrush = Brushes.Transparent;
		SelectedProfileStartupPrimaryActionBrush = Brushes.Transparent;
		SelectedProfileMenuBackgroundColor = "";
		SelectedProfileMenuHighlightColor = "";
		SelectedProfileAccentColor = "";
		SelectedProfileMenuBackgroundBrush = Brushes.Transparent;
		SelectedProfileMenuHighlightBrush = Brushes.Transparent;
		SelectedProfileAccentBrush = Brushes.Transparent;
		SelectedProfileControlHoverBackgroundColor = "";
		SelectedProfileControlPressedBackgroundColor = "";
		SelectedProfileControlHoverBorderColor = "";
		SelectedProfileControlPressedBorderColor = "";
		SelectedProfileControlHoverBackgroundBrush = Brushes.Transparent;
		SelectedProfileControlPressedBackgroundBrush = Brushes.Transparent;
		SelectedProfileControlHoverBorderBrush = Brushes.Transparent;
		SelectedProfileControlPressedBorderBrush = Brushes.Transparent;
		SelectedProfileSidebarBorderColor = "";
		SelectedProfileDockTabStripBackgroundColor = "";
		SelectedProfileDockTabHoverColor = "";
		SelectedProfileDockTabActiveBackgroundColor = "";
		SelectedProfileDockTabActiveBorderColor = "";
		SelectedProfileCheckBoxHoverBorderColor = "";
		SelectedProfileCheckBoxPressedBackgroundColor = "";
		SelectedProfileCheckBoxPressedBorderColor = "";
		SelectedProfileRadioButtonHoverBorderColor = "";
		SelectedProfileRadioButtonPressedBackgroundColor = "";
		SelectedProfileRadioButtonPressedBorderColor = "";
		SelectedProfileSliderHoverTrackColor = "";
		SelectedProfileSliderPressedTrackColor = "";
		SelectedProfileTextSelectionColor = "";
		SelectedProfileTextControlDisabledBackgroundColor = "";
		SelectedProfileToolTipBackgroundColor = "";
		SelectedProfileMenuFlyoutBackgroundColor = "";
		SelectedProfileMenuFlyoutBorderColor = "";
		SelectedProfileComboBoxDropDownBackgroundColor = "";
		SelectedProfileComboBoxDropDownBorderColor = "";
		SelectedProfileDataGridHeaderBackgroundColor = "";
		SelectedProfileDataGridHeaderForegroundColor = "";
		SelectedProfileDataGridSelectedRowForegroundColor = "";
		SelectedProfileListBoxHoverBackgroundColor = "";
		SelectedProfileListBoxSelectedBackgroundColor = "";
		SelectedProfileListBoxSelectedForegroundColor = "";
		SelectedProfileTreeViewHoverBackgroundColor = "";
		SelectedProfileTreeViewSelectedBackgroundColor = "";
		SelectedProfileTreeViewSelectedForegroundColor = "";
		SelectedProfileNavigationViewHoverBackgroundColor = "";
		SelectedProfileNavigationViewSelectedBackgroundColor = "";
		SelectedProfileNavigationViewSelectedForegroundColor = "";
		SelectedProfileListViewHoverBackgroundColor = "";
		SelectedProfileListViewSelectedBackgroundColor = "";
		SelectedProfileListViewSelectedForegroundColor = "";
		SelectedProfileSidebarBorderBrush = Brushes.Transparent;
		SelectedProfileDockTabStripBackgroundBrush = Brushes.Transparent;
		SelectedProfileDockTabHoverBrush = Brushes.Transparent;
		SelectedProfileDockTabActiveBackgroundBrush = Brushes.Transparent;
		SelectedProfileDockTabActiveBorderBrush = Brushes.Transparent;
		SelectedProfileCheckBoxHoverBorderBrush = Brushes.Transparent;
		SelectedProfileCheckBoxPressedBackgroundBrush = Brushes.Transparent;
		SelectedProfileCheckBoxPressedBorderBrush = Brushes.Transparent;
		SelectedProfileRadioButtonHoverBorderBrush = Brushes.Transparent;
		SelectedProfileRadioButtonPressedBackgroundBrush = Brushes.Transparent;
		SelectedProfileRadioButtonPressedBorderBrush = Brushes.Transparent;
		SelectedProfileSliderHoverTrackBrush = Brushes.Transparent;
		SelectedProfileSliderPressedTrackBrush = Brushes.Transparent;
		SelectedProfileTextSelectionBrush = Brushes.Transparent;
		SelectedProfileTextControlDisabledBackgroundBrush = Brushes.Transparent;
		SelectedProfileToolTipBackgroundBrush = Brushes.Transparent;
		SelectedProfileMenuFlyoutBackgroundBrush = Brushes.Transparent;
		SelectedProfileMenuFlyoutBorderBrush = Brushes.Transparent;
		SelectedProfileComboBoxDropDownBackgroundBrush = Brushes.Transparent;
		SelectedProfileComboBoxDropDownBorderBrush = Brushes.Transparent;
		SelectedProfileDataGridHeaderBackgroundBrush = Brushes.Transparent;
		SelectedProfileDataGridHeaderForegroundBrush = Brushes.Transparent;
		SelectedProfileDataGridSelectedRowForegroundBrush = Brushes.Transparent;
		SelectedProfileListBoxHoverBackgroundBrush = Brushes.Transparent;
		SelectedProfileListBoxSelectedBackgroundBrush = Brushes.Transparent;
		SelectedProfileListBoxSelectedForegroundBrush = Brushes.Transparent;
		SelectedProfileTreeViewHoverBackgroundBrush = Brushes.Transparent;
		SelectedProfileTreeViewSelectedBackgroundBrush = Brushes.Transparent;
		SelectedProfileTreeViewSelectedForegroundBrush = Brushes.Transparent;
		SelectedProfileNavigationViewHoverBackgroundBrush = Brushes.Transparent;
		SelectedProfileNavigationViewSelectedBackgroundBrush = Brushes.Transparent;
		SelectedProfileNavigationViewSelectedForegroundBrush = Brushes.Transparent;
		SelectedProfileListViewHoverBackgroundBrush = Brushes.Transparent;
		SelectedProfileListViewSelectedBackgroundBrush = Brushes.Transparent;
		SelectedProfileListViewSelectedForegroundBrush = Brushes.Transparent;
		SelectedThemeProfileDivergenceCount = 0;
		SelectedThemeProfileMatchesDefaults = true;
		SelectedThemeProfileIsCustomized = false;
		SelectedThemeProfileCustomizedTokensSummary = "";
		SelectedThemeProfileCustomizedFontTokensSummary = "";
		SelectedThemeProfileCustomizedStartupTokensSummary = "";
		SelectedThemeProfileCustomizedSetupTokensSummary = "";
		HasCustomizedFontTokens = false;
		HasCustomizedStartupTokens = false;
		HasCustomizedSetupTokens = false;
		RenameThemeProfileName = "";
		DuplicateThemeProfileName = "";
		RefreshThemeProfiles();

		IsOsx = OperatingSystem.IsMacOS();
		DataStorageLocation = ConfigManager.HomeFolder;

		EmulatorShortcut[] displayOrder = new EmulatorShortcut[] {
			EmulatorShortcut.FastForward,
			EmulatorShortcut.ToggleFastForward,
			EmulatorShortcut.Rewind,
			EmulatorShortcut.ToggleRewind,
			EmulatorShortcut.RewindTenSecs,
			EmulatorShortcut.RewindOneMin,

			EmulatorShortcut.Pause,
			EmulatorShortcut.Reset,
			EmulatorShortcut.PowerCycle,
			EmulatorShortcut.ReloadRom,
			EmulatorShortcut.PowerOff,
			EmulatorShortcut.Exit,

			EmulatorShortcut.ToggleRecordVideo,
			EmulatorShortcut.ToggleRecordAudio,
			EmulatorShortcut.ToggleRecordMovie,

			EmulatorShortcut.TakeScreenshot,
			EmulatorShortcut.RunSingleFrame,

			EmulatorShortcut.SetScale1x,
			EmulatorShortcut.SetScale2x,
			EmulatorShortcut.SetScale3x,
			EmulatorShortcut.SetScale4x,
			EmulatorShortcut.SetScale5x,
			EmulatorShortcut.SetScale6x,
			EmulatorShortcut.SetScale7x,
			EmulatorShortcut.SetScale8x,
			EmulatorShortcut.SetScale9x,
			EmulatorShortcut.SetScale10x,
			EmulatorShortcut.ToggleFullscreen,

			EmulatorShortcut.ToggleDebugInfo,
			EmulatorShortcut.ToggleFps,
			EmulatorShortcut.ToggleGameTimer,
			EmulatorShortcut.ToggleFrameCounter,
			EmulatorShortcut.ToggleAlwaysOnTop,
			EmulatorShortcut.ToggleCheats,
			EmulatorShortcut.ToggleOsd,

			EmulatorShortcut.ToggleBgLayer1,
			EmulatorShortcut.ToggleBgLayer2,
			EmulatorShortcut.ToggleBgLayer3,
			EmulatorShortcut.ToggleBgLayer4,
			EmulatorShortcut.ToggleSprites1,
			EmulatorShortcut.ToggleSprites2,
			EmulatorShortcut.EnableAllLayers,

			EmulatorShortcut.ToggleLagCounter,
			EmulatorShortcut.ResetLagCounter,

			EmulatorShortcut.ToggleAudio,
			EmulatorShortcut.IncreaseVolume,
			EmulatorShortcut.DecreaseVolume,

			EmulatorShortcut.PreviousTrack,
			EmulatorShortcut.NextTrack,

			EmulatorShortcut.MaxSpeed,
			EmulatorShortcut.IncreaseSpeed,
			EmulatorShortcut.DecreaseSpeed,

			EmulatorShortcut.OpenFile,

			EmulatorShortcut.InputBarcode,
			EmulatorShortcut.LoadTape,
			EmulatorShortcut.RecordTape,
			EmulatorShortcut.StopRecordTape,

			// Infinite save state system (replaces slot-based system)
			EmulatorShortcut.QuickSaveTimestamped,
			EmulatorShortcut.OpenSaveStatePicker,
			EmulatorShortcut.SaveDesignatedSlot,
			EmulatorShortcut.LoadDesignatedSlot,
			EmulatorShortcut.SaveDesignatedSlot2,
			EmulatorShortcut.LoadDesignatedSlot2,
			EmulatorShortcut.SaveDesignatedSlot3,
			EmulatorShortcut.LoadDesignatedSlot3,
			EmulatorShortcut.SaveStateToFile,
			EmulatorShortcut.SaveStateDialog,
			EmulatorShortcut.LoadStateFromFile,
			EmulatorShortcut.LoadStateDialog,
			EmulatorShortcut.LoadLastSession
		};

		Dictionary<EmulatorShortcut, ShortcutKeyInfo> shortcuts = new Dictionary<EmulatorShortcut, ShortcutKeyInfo>();
		foreach (ShortcutKeyInfo shortcut in Config.ShortcutKeys) {
			shortcuts[shortcut.Shortcut] = shortcut;
		}

		ShortcutKeys = [];
		for (int i = 0; i < displayOrder.Length; i++) {
			if (shortcuts.TryGetValue(displayOrder[i], out var shortcut)) {
				ShortcutKeys.Add(shortcut);
			}
		}

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => {
			if (_lastUiLanguage != Config.UiLanguage) {
				_lastUiLanguage = Config.UiLanguage;
				ResourceHelper.LoadResources(PreferencesConfig.GetLanguageCode(Config.UiLanguage));
			}

			Config.ApplyConfig();
			PreferencesConfig.UpdateTheme();
			RefreshSelectedThemeProfileDetails();
		}));
	}

	public void RefreshThemeProfiles() {
		Config.EnsureThemeProfiles();
		ThemeProfileNames = Config.ThemeProfiles.Select(profile => profile.Name).ToList();
		if (string.IsNullOrWhiteSpace(SelectedThemeProfileName) || !ThemeProfileNames.Contains(SelectedThemeProfileName)) {
			SelectedThemeProfileName = Config.ActiveThemeProfileName;
		}

		RefreshSelectedThemeProfileDetails();
	}

	public ThemeProfile? GetSelectedThemeProfile() {
		return Config.GetThemeProfileByName(SelectedThemeProfileName);
	}

	public void ActivateSelectedThemeProfile() {
		Config.SetActiveThemeProfile(SelectedThemeProfileName);
		RefreshThemeProfiles();
	}

	public void SaveCurrentToSelectedThemeProfile() {
		Config.SaveCurrentToProfile(SelectedThemeProfileName);
		RefreshThemeProfiles();
	}

	public bool DeleteSelectedThemeProfile() {
		bool removed = Config.DeleteThemeProfile(SelectedThemeProfileName);
		RefreshThemeProfiles();
		return removed;
	}

	public bool RenameSelectedThemeProfile() {
		bool renamed = Config.RenameThemeProfile(SelectedThemeProfileName, RenameThemeProfileName);
		if (renamed) {
			SelectedThemeProfileName = RenameThemeProfileName.Trim();
		}

		RefreshThemeProfiles();
		return renamed;
	}

	public bool DuplicateSelectedThemeProfile() {
		bool duplicated = Config.DuplicateThemeProfile(SelectedThemeProfileName, DuplicateThemeProfileName);
		RefreshThemeProfiles();
		return duplicated;
	}

	public bool ResetSelectedThemeProfileToDefaults() {
		bool reset = Config.ResetThemeProfileToDefaults(SelectedThemeProfileName);
		RefreshThemeProfiles();
		return reset;
	}

	public bool ApplyLightPresetToSelectedThemeProfile() {
		bool applied = Config.ApplyThemePresetToProfile(SelectedThemeProfileName, NexenTheme.Light);
		RefreshThemeProfiles();
		return applied;
	}

	public bool ApplyDarkPresetToSelectedThemeProfile() {
		bool applied = Config.ApplyThemePresetToProfile(SelectedThemeProfileName, NexenTheme.Dark);
		RefreshThemeProfiles();
		return applied;
	}

	public void UpsertImportedThemeProfile(ThemeProfile profile, bool activateProfile) {
		Config.UpsertThemeProfile(profile, activateProfile);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileColors(string background, string text, string primaryAction) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.StartupWindowBackgroundColor = background;
		selectedProfile.StartupTextColor = text;
		selectedProfile.StartupPrimaryActionColor = primaryAction;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileUiChromeColors(string menuBackground, string menuHighlight, string accent) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.MenuBackgroundColor = menuBackground;
		selectedProfile.MenuBackgroundHighlightColor = menuHighlight;
		selectedProfile.ThemeAccentColor = accent;
		selectedProfile.HighlightColor = accent;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileControlChromeColors(string hoverBackground, string pressedBackground, string hoverBorder, string pressedBorder) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.ControlPointerOverBackgroundColor = hoverBackground;
		selectedProfile.ControlPressedBackgroundColor = pressedBackground;
		selectedProfile.ControlPointerOverBorderColor = hoverBorder;
		selectedProfile.ControlPressedBorderColor = pressedBorder;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileSidebarTabChromeColors(string sidebarBorder, string tabStripBackground, string tabHover, string tabActiveBackground, string tabActiveBorder) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.SettingsTabStripBorderColor = sidebarBorder;
		selectedProfile.DockTabStripBackgroundColor = tabStripBackground;
		selectedProfile.DockTabPointerOverColor = tabHover;
		selectedProfile.DockTabActiveBackgroundColor = tabActiveBackground;
		selectedProfile.DockTabActiveBorderColor = tabActiveBorder;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileControlStateSemanticColors(string checkBoxHoverBorder, string checkBoxPressedBackground, string checkBoxPressedBorder, string radioHoverBorder, string radioPressedBackground, string radioPressedBorder, string sliderHoverTrack, string sliderPressedTrack) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.CheckBoxPointerOverBorderColor = checkBoxHoverBorder;
		selectedProfile.CheckBoxPressedBackgroundColor = checkBoxPressedBackground;
		selectedProfile.CheckBoxPressedBorderColor = checkBoxPressedBorder;
		selectedProfile.RadioButtonPointerOverBorderColor = radioHoverBorder;
		selectedProfile.RadioButtonPressedBackgroundColor = radioPressedBackground;
		selectedProfile.RadioButtonPressedBorderColor = radioPressedBorder;
		selectedProfile.SliderTrackPointerOverColor = sliderHoverTrack;
		selectedProfile.SliderTrackPressedColor = sliderPressedTrack;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileTextInputAndFlyoutSemanticColors(string textSelection, string textControlDisabledBackground, string toolTipBackground, string menuFlyoutBackground, string menuFlyoutBorder) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.TextBoxSelectionColor = textSelection;
		selectedProfile.TextControlDisabledBackgroundColor = textControlDisabledBackground;
		selectedProfile.ToolTipBackgroundColor = toolTipBackground;
		selectedProfile.MenuFlyoutBackgroundColor = menuFlyoutBackground;
		selectedProfile.MenuFlyoutBorderColor = menuFlyoutBorder;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileComboDropDownAndDataGridSemanticColors(string comboDropDownBackground, string comboDropDownBorder, string dataGridHeaderBackground, string dataGridHeaderForeground, string dataGridSelectedRowForeground) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.ComboBoxDropDownBackgroundColor = comboDropDownBackground;
		selectedProfile.ComboBoxDropDownBorderColor = comboDropDownBorder;
		selectedProfile.DataGridHeaderBackgroundColor = dataGridHeaderBackground;
		selectedProfile.DataGridHeaderForegroundColor = dataGridHeaderForeground;
		selectedProfile.DataGridSelectedRowForegroundColor = dataGridSelectedRowForeground;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileListBoxAndTreeViewSemanticColors(string listBoxHoverBackground, string listBoxSelectedBackground, string listBoxSelectedForeground, string treeViewHoverBackground, string treeViewSelectedBackground, string treeViewSelectedForeground) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.ListBoxItemHoverBackgroundColor = listBoxHoverBackground;
		selectedProfile.ListBoxItemSelectedBackgroundColor = listBoxSelectedBackground;
		selectedProfile.ListBoxItemSelectedForegroundColor = listBoxSelectedForeground;
		selectedProfile.TreeViewItemHoverBackgroundColor = treeViewHoverBackground;
		selectedProfile.TreeViewItemSelectedBackgroundColor = treeViewSelectedBackground;
		selectedProfile.TreeViewItemSelectedForegroundColor = treeViewSelectedForeground;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	public void SetSelectedProfileNavigationViewAndListViewSemanticColors(string navigationViewHoverBackground, string navigationViewSelectedBackground, string navigationViewSelectedForeground, string listViewHoverBackground, string listViewSelectedBackground, string listViewSelectedForeground) {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			return;
		}

		selectedProfile.NavigationViewItemHoverBackgroundColor = navigationViewHoverBackground;
		selectedProfile.NavigationViewItemSelectedBackgroundColor = navigationViewSelectedBackground;
		selectedProfile.NavigationViewItemSelectedForegroundColor = navigationViewSelectedForeground;
		selectedProfile.ListViewItemHoverBackgroundColor = listViewHoverBackground;
		selectedProfile.ListViewItemSelectedBackgroundColor = listViewSelectedBackground;
		selectedProfile.ListViewItemSelectedForegroundColor = listViewSelectedForeground;
		Config.SetActiveThemeProfile(selectedProfile.Name);
		RefreshThemeProfiles();
	}

	private void RefreshSelectedThemeProfileDetails() {
		ThemeProfile? selectedProfile = GetSelectedThemeProfile();
		if (selectedProfile is null) {
			SelectedProfileStartupBackgroundColor = "";
			SelectedProfileStartupTextColor = "";
			SelectedProfileStartupPrimaryActionColor = "";
			SelectedProfileStartupBackgroundBrush = Brushes.Transparent;
			SelectedProfileStartupTextBrush = Brushes.Transparent;
			SelectedProfileStartupPrimaryActionBrush = Brushes.Transparent;
			SelectedProfileMenuBackgroundColor = "";
			SelectedProfileMenuHighlightColor = "";
			SelectedProfileAccentColor = "";
			SelectedProfileMenuBackgroundBrush = Brushes.Transparent;
			SelectedProfileMenuHighlightBrush = Brushes.Transparent;
			SelectedProfileAccentBrush = Brushes.Transparent;
			SelectedProfileControlHoverBackgroundColor = "";
			SelectedProfileControlPressedBackgroundColor = "";
			SelectedProfileControlHoverBorderColor = "";
			SelectedProfileControlPressedBorderColor = "";
			SelectedProfileControlHoverBackgroundBrush = Brushes.Transparent;
			SelectedProfileControlPressedBackgroundBrush = Brushes.Transparent;
			SelectedProfileControlHoverBorderBrush = Brushes.Transparent;
			SelectedProfileControlPressedBorderBrush = Brushes.Transparent;
			SelectedProfileSidebarBorderColor = "";
			SelectedProfileDockTabStripBackgroundColor = "";
			SelectedProfileDockTabHoverColor = "";
			SelectedProfileDockTabActiveBackgroundColor = "";
			SelectedProfileDockTabActiveBorderColor = "";
			SelectedProfileCheckBoxHoverBorderColor = "";
			SelectedProfileCheckBoxPressedBackgroundColor = "";
			SelectedProfileCheckBoxPressedBorderColor = "";
			SelectedProfileRadioButtonHoverBorderColor = "";
			SelectedProfileRadioButtonPressedBackgroundColor = "";
			SelectedProfileRadioButtonPressedBorderColor = "";
			SelectedProfileSliderHoverTrackColor = "";
			SelectedProfileSliderPressedTrackColor = "";
			SelectedProfileTextSelectionColor = "";
			SelectedProfileTextControlDisabledBackgroundColor = "";
			SelectedProfileToolTipBackgroundColor = "";
			SelectedProfileMenuFlyoutBackgroundColor = "";
			SelectedProfileMenuFlyoutBorderColor = "";
			SelectedProfileComboBoxDropDownBackgroundColor = "";
			SelectedProfileComboBoxDropDownBorderColor = "";
			SelectedProfileDataGridHeaderBackgroundColor = "";
			SelectedProfileDataGridHeaderForegroundColor = "";
			SelectedProfileDataGridSelectedRowForegroundColor = "";
			SelectedProfileListBoxHoverBackgroundColor = "";
			SelectedProfileListBoxSelectedBackgroundColor = "";
			SelectedProfileListBoxSelectedForegroundColor = "";
			SelectedProfileTreeViewHoverBackgroundColor = "";
			SelectedProfileTreeViewSelectedBackgroundColor = "";
			SelectedProfileTreeViewSelectedForegroundColor = "";
			SelectedProfileNavigationViewHoverBackgroundColor = "";
			SelectedProfileNavigationViewSelectedBackgroundColor = "";
			SelectedProfileNavigationViewSelectedForegroundColor = "";
			SelectedProfileListViewHoverBackgroundColor = "";
			SelectedProfileListViewSelectedBackgroundColor = "";
			SelectedProfileListViewSelectedForegroundColor = "";
			SelectedProfileSidebarBorderBrush = Brushes.Transparent;
			SelectedProfileDockTabStripBackgroundBrush = Brushes.Transparent;
			SelectedProfileDockTabHoverBrush = Brushes.Transparent;
			SelectedProfileDockTabActiveBackgroundBrush = Brushes.Transparent;
			SelectedProfileDockTabActiveBorderBrush = Brushes.Transparent;
			SelectedProfileCheckBoxHoverBorderBrush = Brushes.Transparent;
			SelectedProfileCheckBoxPressedBackgroundBrush = Brushes.Transparent;
			SelectedProfileCheckBoxPressedBorderBrush = Brushes.Transparent;
			SelectedProfileRadioButtonHoverBorderBrush = Brushes.Transparent;
			SelectedProfileRadioButtonPressedBackgroundBrush = Brushes.Transparent;
			SelectedProfileRadioButtonPressedBorderBrush = Brushes.Transparent;
			SelectedProfileSliderHoverTrackBrush = Brushes.Transparent;
			SelectedProfileSliderPressedTrackBrush = Brushes.Transparent;
			SelectedProfileTextSelectionBrush = Brushes.Transparent;
			SelectedProfileTextControlDisabledBackgroundBrush = Brushes.Transparent;
			SelectedProfileToolTipBackgroundBrush = Brushes.Transparent;
			SelectedProfileMenuFlyoutBackgroundBrush = Brushes.Transparent;
			SelectedProfileMenuFlyoutBorderBrush = Brushes.Transparent;
			SelectedProfileComboBoxDropDownBackgroundBrush = Brushes.Transparent;
			SelectedProfileComboBoxDropDownBorderBrush = Brushes.Transparent;
			SelectedProfileDataGridHeaderBackgroundBrush = Brushes.Transparent;
			SelectedProfileDataGridHeaderForegroundBrush = Brushes.Transparent;
			SelectedProfileDataGridSelectedRowForegroundBrush = Brushes.Transparent;
			SelectedProfileListBoxHoverBackgroundBrush = Brushes.Transparent;
			SelectedProfileListBoxSelectedBackgroundBrush = Brushes.Transparent;
			SelectedProfileListBoxSelectedForegroundBrush = Brushes.Transparent;
			SelectedProfileTreeViewHoverBackgroundBrush = Brushes.Transparent;
			SelectedProfileTreeViewSelectedBackgroundBrush = Brushes.Transparent;
			SelectedProfileTreeViewSelectedForegroundBrush = Brushes.Transparent;
			SelectedProfileNavigationViewHoverBackgroundBrush = Brushes.Transparent;
			SelectedProfileNavigationViewSelectedBackgroundBrush = Brushes.Transparent;
			SelectedProfileNavigationViewSelectedForegroundBrush = Brushes.Transparent;
			SelectedProfileListViewHoverBackgroundBrush = Brushes.Transparent;
			SelectedProfileListViewSelectedBackgroundBrush = Brushes.Transparent;
			SelectedProfileListViewSelectedForegroundBrush = Brushes.Transparent;
			SelectedThemeProfileDivergenceCount = 0;
			SelectedThemeProfileMatchesDefaults = true;
			SelectedThemeProfileIsCustomized = false;
			SelectedThemeProfileCustomizedTokensSummary = "";
			SelectedThemeProfileCustomizedFontTokensSummary = "";
			SelectedThemeProfileCustomizedStartupTokensSummary = "";
			SelectedThemeProfileCustomizedSetupTokensSummary = "";
			HasCustomizedFontTokens = false;
			HasCustomizedStartupTokens = false;
			HasCustomizedSetupTokens = false;
			return;
		}

		SelectedProfileStartupBackgroundColor = selectedProfile.StartupWindowBackgroundColor;
		SelectedProfileStartupTextColor = selectedProfile.StartupTextColor;
		SelectedProfileStartupPrimaryActionColor = selectedProfile.StartupPrimaryActionColor;
		SelectedProfileStartupBackgroundBrush = CreatePreviewBrush(selectedProfile.StartupWindowBackgroundColor);
		SelectedProfileStartupTextBrush = CreatePreviewBrush(selectedProfile.StartupTextColor);
		SelectedProfileStartupPrimaryActionBrush = CreatePreviewBrush(selectedProfile.StartupPrimaryActionColor);
		SelectedProfileMenuBackgroundColor = selectedProfile.MenuBackgroundColor;
		SelectedProfileMenuHighlightColor = selectedProfile.MenuBackgroundHighlightColor;
		SelectedProfileAccentColor = selectedProfile.ThemeAccentColor;
		SelectedProfileMenuBackgroundBrush = CreatePreviewBrush(selectedProfile.MenuBackgroundColor);
		SelectedProfileMenuHighlightBrush = CreatePreviewBrush(selectedProfile.MenuBackgroundHighlightColor);
		SelectedProfileAccentBrush = CreatePreviewBrush(selectedProfile.ThemeAccentColor);
		SelectedProfileControlHoverBackgroundColor = selectedProfile.ControlPointerOverBackgroundColor;
		SelectedProfileControlPressedBackgroundColor = selectedProfile.ControlPressedBackgroundColor;
		SelectedProfileControlHoverBorderColor = selectedProfile.ControlPointerOverBorderColor;
		SelectedProfileControlPressedBorderColor = selectedProfile.ControlPressedBorderColor;
		SelectedProfileControlHoverBackgroundBrush = CreatePreviewBrush(selectedProfile.ControlPointerOverBackgroundColor);
		SelectedProfileControlPressedBackgroundBrush = CreatePreviewBrush(selectedProfile.ControlPressedBackgroundColor);
		SelectedProfileControlHoverBorderBrush = CreatePreviewBrush(selectedProfile.ControlPointerOverBorderColor);
		SelectedProfileControlPressedBorderBrush = CreatePreviewBrush(selectedProfile.ControlPressedBorderColor);
		SelectedProfileSidebarBorderColor = selectedProfile.SettingsTabStripBorderColor;
		SelectedProfileDockTabStripBackgroundColor = selectedProfile.DockTabStripBackgroundColor;
		SelectedProfileDockTabHoverColor = selectedProfile.DockTabPointerOverColor;
		SelectedProfileDockTabActiveBackgroundColor = selectedProfile.DockTabActiveBackgroundColor;
		SelectedProfileDockTabActiveBorderColor = selectedProfile.DockTabActiveBorderColor;
		SelectedProfileCheckBoxHoverBorderColor = selectedProfile.CheckBoxPointerOverBorderColor;
		SelectedProfileCheckBoxPressedBackgroundColor = selectedProfile.CheckBoxPressedBackgroundColor;
		SelectedProfileCheckBoxPressedBorderColor = selectedProfile.CheckBoxPressedBorderColor;
		SelectedProfileRadioButtonHoverBorderColor = selectedProfile.RadioButtonPointerOverBorderColor;
		SelectedProfileRadioButtonPressedBackgroundColor = selectedProfile.RadioButtonPressedBackgroundColor;
		SelectedProfileRadioButtonPressedBorderColor = selectedProfile.RadioButtonPressedBorderColor;
		SelectedProfileSliderHoverTrackColor = selectedProfile.SliderTrackPointerOverColor;
		SelectedProfileSliderPressedTrackColor = selectedProfile.SliderTrackPressedColor;
		SelectedProfileTextSelectionColor = selectedProfile.TextBoxSelectionColor;
		SelectedProfileTextControlDisabledBackgroundColor = selectedProfile.TextControlDisabledBackgroundColor;
		SelectedProfileToolTipBackgroundColor = selectedProfile.ToolTipBackgroundColor;
		SelectedProfileMenuFlyoutBackgroundColor = selectedProfile.MenuFlyoutBackgroundColor;
		SelectedProfileMenuFlyoutBorderColor = selectedProfile.MenuFlyoutBorderColor;
		SelectedProfileComboBoxDropDownBackgroundColor = selectedProfile.ComboBoxDropDownBackgroundColor;
		SelectedProfileComboBoxDropDownBorderColor = selectedProfile.ComboBoxDropDownBorderColor;
		SelectedProfileDataGridHeaderBackgroundColor = selectedProfile.DataGridHeaderBackgroundColor;
		SelectedProfileDataGridHeaderForegroundColor = selectedProfile.DataGridHeaderForegroundColor;
		SelectedProfileDataGridSelectedRowForegroundColor = selectedProfile.DataGridSelectedRowForegroundColor;
		SelectedProfileListBoxHoverBackgroundColor = selectedProfile.ListBoxItemHoverBackgroundColor;
		SelectedProfileListBoxSelectedBackgroundColor = selectedProfile.ListBoxItemSelectedBackgroundColor;
		SelectedProfileListBoxSelectedForegroundColor = selectedProfile.ListBoxItemSelectedForegroundColor;
		SelectedProfileTreeViewHoverBackgroundColor = selectedProfile.TreeViewItemHoverBackgroundColor;
		SelectedProfileTreeViewSelectedBackgroundColor = selectedProfile.TreeViewItemSelectedBackgroundColor;
		SelectedProfileTreeViewSelectedForegroundColor = selectedProfile.TreeViewItemSelectedForegroundColor;
		SelectedProfileNavigationViewHoverBackgroundColor = selectedProfile.NavigationViewItemHoverBackgroundColor;
		SelectedProfileNavigationViewSelectedBackgroundColor = selectedProfile.NavigationViewItemSelectedBackgroundColor;
		SelectedProfileNavigationViewSelectedForegroundColor = selectedProfile.NavigationViewItemSelectedForegroundColor;
		SelectedProfileListViewHoverBackgroundColor = selectedProfile.ListViewItemHoverBackgroundColor;
		SelectedProfileListViewSelectedBackgroundColor = selectedProfile.ListViewItemSelectedBackgroundColor;
		SelectedProfileListViewSelectedForegroundColor = selectedProfile.ListViewItemSelectedForegroundColor;
		SelectedProfileSidebarBorderBrush = CreatePreviewBrush(selectedProfile.SettingsTabStripBorderColor);
		SelectedProfileDockTabStripBackgroundBrush = CreatePreviewBrush(selectedProfile.DockTabStripBackgroundColor);
		SelectedProfileDockTabHoverBrush = CreatePreviewBrush(selectedProfile.DockTabPointerOverColor);
		SelectedProfileDockTabActiveBackgroundBrush = CreatePreviewBrush(selectedProfile.DockTabActiveBackgroundColor);
		SelectedProfileDockTabActiveBorderBrush = CreatePreviewBrush(selectedProfile.DockTabActiveBorderColor);
		SelectedProfileCheckBoxHoverBorderBrush = CreatePreviewBrush(selectedProfile.CheckBoxPointerOverBorderColor);
		SelectedProfileCheckBoxPressedBackgroundBrush = CreatePreviewBrush(selectedProfile.CheckBoxPressedBackgroundColor);
		SelectedProfileCheckBoxPressedBorderBrush = CreatePreviewBrush(selectedProfile.CheckBoxPressedBorderColor);
		SelectedProfileRadioButtonHoverBorderBrush = CreatePreviewBrush(selectedProfile.RadioButtonPointerOverBorderColor);
		SelectedProfileRadioButtonPressedBackgroundBrush = CreatePreviewBrush(selectedProfile.RadioButtonPressedBackgroundColor);
		SelectedProfileRadioButtonPressedBorderBrush = CreatePreviewBrush(selectedProfile.RadioButtonPressedBorderColor);
		SelectedProfileSliderHoverTrackBrush = CreatePreviewBrush(selectedProfile.SliderTrackPointerOverColor);
		SelectedProfileSliderPressedTrackBrush = CreatePreviewBrush(selectedProfile.SliderTrackPressedColor);
		SelectedProfileTextSelectionBrush = CreatePreviewBrush(selectedProfile.TextBoxSelectionColor);
		SelectedProfileTextControlDisabledBackgroundBrush = CreatePreviewBrush(selectedProfile.TextControlDisabledBackgroundColor);
		SelectedProfileToolTipBackgroundBrush = CreatePreviewBrush(selectedProfile.ToolTipBackgroundColor);
		SelectedProfileMenuFlyoutBackgroundBrush = CreatePreviewBrush(selectedProfile.MenuFlyoutBackgroundColor);
		SelectedProfileMenuFlyoutBorderBrush = CreatePreviewBrush(selectedProfile.MenuFlyoutBorderColor);
		SelectedProfileComboBoxDropDownBackgroundBrush = CreatePreviewBrush(selectedProfile.ComboBoxDropDownBackgroundColor);
		SelectedProfileComboBoxDropDownBorderBrush = CreatePreviewBrush(selectedProfile.ComboBoxDropDownBorderColor);
		SelectedProfileDataGridHeaderBackgroundBrush = CreatePreviewBrush(selectedProfile.DataGridHeaderBackgroundColor);
		SelectedProfileDataGridHeaderForegroundBrush = CreatePreviewBrush(selectedProfile.DataGridHeaderForegroundColor);
		SelectedProfileDataGridSelectedRowForegroundBrush = CreatePreviewBrush(selectedProfile.DataGridSelectedRowForegroundColor);
		SelectedProfileListBoxHoverBackgroundBrush = CreatePreviewBrush(selectedProfile.ListBoxItemHoverBackgroundColor);
		SelectedProfileListBoxSelectedBackgroundBrush = CreatePreviewBrush(selectedProfile.ListBoxItemSelectedBackgroundColor);
		SelectedProfileListBoxSelectedForegroundBrush = CreatePreviewBrush(selectedProfile.ListBoxItemSelectedForegroundColor);
		SelectedProfileTreeViewHoverBackgroundBrush = CreatePreviewBrush(selectedProfile.TreeViewItemHoverBackgroundColor);
		SelectedProfileTreeViewSelectedBackgroundBrush = CreatePreviewBrush(selectedProfile.TreeViewItemSelectedBackgroundColor);
		SelectedProfileTreeViewSelectedForegroundBrush = CreatePreviewBrush(selectedProfile.TreeViewItemSelectedForegroundColor);
		SelectedProfileNavigationViewHoverBackgroundBrush = CreatePreviewBrush(selectedProfile.NavigationViewItemHoverBackgroundColor);
		SelectedProfileNavigationViewSelectedBackgroundBrush = CreatePreviewBrush(selectedProfile.NavigationViewItemSelectedBackgroundColor);
		SelectedProfileNavigationViewSelectedForegroundBrush = CreatePreviewBrush(selectedProfile.NavigationViewItemSelectedForegroundColor);
		SelectedProfileListViewHoverBackgroundBrush = CreatePreviewBrush(selectedProfile.ListViewItemHoverBackgroundColor);
		SelectedProfileListViewSelectedBackgroundBrush = CreatePreviewBrush(selectedProfile.ListViewItemSelectedBackgroundColor);
		SelectedProfileListViewSelectedForegroundBrush = CreatePreviewBrush(selectedProfile.ListViewItemSelectedForegroundColor);
		SelectedThemeProfileDivergenceCount = Config.GetThemeProfileDivergenceCount(selectedProfile.Name);
		SelectedThemeProfileMatchesDefaults = SelectedThemeProfileDivergenceCount == 0;
		SelectedThemeProfileIsCustomized = SelectedThemeProfileDivergenceCount > 0;
		List<string> customizedTokens = Config.GetThemeProfileCustomizedTokenNames(selectedProfile.Name);
		SelectedThemeProfileCustomizedTokensSummary = string.Join(", ", customizedTokens);
		List<string> fontTokens = customizedTokens.Where(token => token.Contains("Font")).ToList();
		List<string> startupTokens = customizedTokens.Where(token => token.StartsWith("Startup", StringComparison.Ordinal)).ToList();
		List<string> setupTokens = customizedTokens.Where(token => token.StartsWith("Setup", StringComparison.Ordinal)).ToList();
		SelectedThemeProfileCustomizedFontTokensSummary = string.Join(", ", fontTokens);
		SelectedThemeProfileCustomizedStartupTokensSummary = string.Join(", ", startupTokens);
		SelectedThemeProfileCustomizedSetupTokensSummary = string.Join(", ", setupTokens);
		HasCustomizedFontTokens = fontTokens.Count > 0;
		HasCustomizedStartupTokens = startupTokens.Count > 0;
		HasCustomizedSetupTokens = setupTokens.Count > 0;
		RenameThemeProfileName = selectedProfile.Name;
	}

	private static IBrush CreatePreviewBrush(string colorValue) {
		try {
			Color color = Color.Parse(colorValue);
			return new SolidColorBrush(color);
		} catch {
			return Brushes.Transparent;
		}
	}
}
