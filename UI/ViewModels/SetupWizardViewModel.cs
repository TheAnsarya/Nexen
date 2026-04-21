using System;
using System.Diagnostics;
using System.IO;
using System.Reactive.Linq;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the first-run setup wizard.
/// Handles initial configuration of storage location, controller mappings, and shortcuts.
/// </summary>
public sealed partial class SetupWizardViewModel : ViewModelBase {
	private int _primaryUsageChangeCount;
	private int _storageChangeCount;
	private bool _suppressTracking;

	[Reactive] public partial bool HasResumedDraft { get; set; }

	/// <summary>Gets or sets the user's primary usage intent for first-run defaults.</summary>
	[Reactive] public partial PrimaryUsageProfile PrimaryUsageProfile { get; set; } = PrimaryUsageProfile.Playing;

	/// <summary>Gets or sets whether the playing profile is selected in the setup wizard.</summary>
	public bool UsePlayingProfile {
		get => PrimaryUsageProfile == PrimaryUsageProfile.Playing;
		set {
			if (value) {
				PrimaryUsageProfile = PrimaryUsageProfile.Playing;
			}
		}
	}

	/// <summary>Gets or sets whether the debugging profile is selected in the setup wizard.</summary>
	public bool UseDebuggingProfile {
		get => PrimaryUsageProfile == PrimaryUsageProfile.Debugging;
		set {
			if (value) {
				PrimaryUsageProfile = PrimaryUsageProfile.Debugging;
			}
		}
	}

	/// <summary>Gets or sets whether to store configuration in the user profile folder.</summary>
	[Reactive] public partial bool StoreInUserProfile { get; set; } = true;

	/// <summary>Gets or sets whether to enable default Xbox controller mappings.</summary>
	[Reactive] public partial bool EnableXboxMappings { get; set; } = true;

	/// <summary>Gets or sets whether to show optional controller customization during onboarding.</summary>
	[Reactive] public partial bool CustomizeInputMappingsNow { get; set; }

	/// <summary>Gets or sets whether to enable default PlayStation controller mappings.</summary>
	[Reactive] public partial bool EnablePsMappings { get; set; }

	/// <summary>Gets or sets whether to enable WASD keyboard mappings.</summary>
	[Reactive] public partial bool EnableWasdMappings { get; set; }

	/// <summary>Gets or sets whether to enable arrow key mappings.</summary>
	[Reactive] public partial bool EnableArrowMappings { get; set; } = true;

	/// <summary>Gets or sets the install/configuration folder location.</summary>
	[Reactive] public partial string InstallLocation { get; set; }

	/// <summary>Gets or sets whether to create a desktop shortcut.</summary>
	[Reactive] public partial bool CreateShortcut { get; set; } = true;

	/// <summary>Gets or sets whether to enable automatic update checks.</summary>
	[Reactive] public partial bool CheckForUpdates { get; set; } = true;

	/// <summary>Gets whether the current platform is macOS.</summary>
	[Reactive] public partial bool IsOsx { get; set; } = OperatingSystem.IsMacOS();

	/// <summary>Gets whether portable mode is available (disabled for AppImage on Linux).</summary>
	[Reactive] public partial bool CanUsePortableMode { get; set; } = !IsRunningInAppImage();

	/// <summary>
	/// Initializes a new instance of the <see cref="SetupWizardViewModel"/> class.
	/// </summary>
	public SetupWizardViewModel() {
		ApplyDefaultSelections();

		InstallLocation = ConfigManager.DefaultDocumentsFolder;

		// AppImage mounts to a read-only temp directory — portable mode cannot work.
		if (!CanUsePortableMode) {
			StoreInUserProfile = true;
		}

		this.WhenAnyValue(x => x.StoreInUserProfile).Subscribe(x => InstallLocation = StoreInUserProfile ? ConfigManager.DefaultDocumentsFolder : ConfigManager.DefaultPortableFolder);

		this.WhenAnyValue(x => x.PrimaryUsageProfile).Subscribe(_ => {
			this.RaisePropertyChanged(nameof(UsePlayingProfile));
			this.RaisePropertyChanged(nameof(UseDebuggingProfile));
		});

		this.WhenAnyValue(x => x.PrimaryUsageProfile).Skip(1).Subscribe(_ => {
			if (_suppressTracking) {
				return;
			}

			_primaryUsageChangeCount++;
			SetupWizardMetricsStore.RecordProfileToggle();
		});

		this.WhenAnyValue(x => x.StoreInUserProfile).Skip(1).Subscribe(_ => {
			if (_suppressTracking) {
				return;
			}

			_storageChangeCount++;
			SetupWizardMetricsStore.RecordStorageToggle();
		});

		this.WhenAnyValue(x => x.CustomizeInputMappingsNow).Skip(1).Subscribe(_ => {
			if (_suppressTracking) {
				return;
			}

			SetupWizardMetricsStore.RecordCustomizeToggle();
		});

		this.WhenAnyValue(x => x.EnableWasdMappings).Subscribe(x => {
			if (x) {
				EnableArrowMappings = false;
			}
		});

		this.WhenAnyValue(x => x.EnableArrowMappings).Subscribe(x => {
			if (x) {
				EnableWasdMappings = false;
			}
		});

		TryRestoreDraftState();
	}

	/// <summary>
	/// Confirms the setup choices and initializes configuration.
	/// </summary>
	/// <param name="parent">The parent window for error dialogs.</param>
	/// <returns>True if setup completed successfully.</returns>
	public bool Confirm(Window parent) {
		string targetFolder = StoreInUserProfile ? ConfigManager.DefaultDocumentsFolder : ConfigManager.DefaultPortableFolder;
		string testFile = Path.Combine(targetFolder, "test.txt");
		try {
			if (!Directory.Exists(targetFolder)) {
				Directory.CreateDirectory(targetFolder);
			}

			File.WriteAllText(testFile, "test");
			File.Delete(testFile);
			InitializeConfig();
			SetupWizardResumeStateStore.Clear();
			HasResumedDraft = false;
			if (CreateShortcut) {
				CreateShortcutFile();
			}

			return true;
		} catch (Exception ex) {
			NexenMsgBox.Show(parent, "CannotWriteToFolder", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.ToString());
		}

		return false;
	}

	/// <summary>
	/// Resets storage selection to the recommended user-profile option.
	/// </summary>
	public void ResetStorageToRecommended() {
		StoreInUserProfile = true;
	}

	public void SaveResumeState() {
		var state = new SetupWizardResumeState {
			PrimaryUsageProfile = PrimaryUsageProfile,
			StoreInUserProfile = StoreInUserProfile,
			CustomizeInputMappingsNow = CustomizeInputMappingsNow,
			EnableXboxMappings = EnableXboxMappings,
			EnablePsMappings = EnablePsMappings,
			EnableWasdMappings = EnableWasdMappings,
			EnableArrowMappings = EnableArrowMappings,
			CreateShortcut = CreateShortcut,
			CheckForUpdates = CheckForUpdates,
		};

		SetupWizardResumeStateStore.Save(state);
	}

	public void DiscardResumeStateAndReset() {
		SetupWizardResumeStateStore.Clear();
		HasResumedDraft = false;
		ApplyDefaultSelections();
		if (!CanUsePortableMode) {
			StoreInUserProfile = true;
		}
		ResetBacktrackCounters();
	}

	public int GetBacktrackCount() {
		int primaryUsageBacktracks = Math.Max(0, _primaryUsageChangeCount);
		int storageBacktracks = Math.Max(0, _storageChangeCount);
		return primaryUsageBacktracks + storageBacktracks;
	}

	private void InitializeConfig() {
		ConfigManager.CreateConfig(!StoreInUserProfile);
		SetupUsageProfileDefaults.Apply(ConfigManager.Config, PrimaryUsageProfile);
		SetupStorageDefaults.Apply(ConfigManager.Config, StoreInUserProfile);
		DefaultKeyMappingType mappingType = DefaultKeyMappingType.None;
		if (EnableXboxMappings) {
			mappingType |= DefaultKeyMappingType.Xbox;
		}

		if (EnablePsMappings) {
			mappingType |= DefaultKeyMappingType.Ps4;
		}

		if (EnableWasdMappings) {
			mappingType |= DefaultKeyMappingType.WasdKeys;
		}

		if (EnableArrowMappings) {
			mappingType |= DefaultKeyMappingType.ArrowKeys;
		}

		ConfigManager.Config.DefaultKeyMappings = mappingType;
		ConfigManager.Config.Preferences.AutomaticallyCheckForUpdates = CheckForUpdates;
		ConfigManager.Config.Save();
	}

	private void ApplyDefaultSelections() {
		_suppressTracking = true;
		PrimaryUsageProfile = PrimaryUsageProfile.Playing;
		StoreInUserProfile = true;
		CustomizeInputMappingsNow = false;
		EnableXboxMappings = true;
		EnablePsMappings = false;
		EnableWasdMappings = false;
		EnableArrowMappings = true;
		CreateShortcut = true;
		CheckForUpdates = true;
		_suppressTracking = false;
	}

	private void TryRestoreDraftState() {
		SetupWizardResumeState? state = SetupWizardResumeStateStore.Load();
		if (state is null) {
			ResetBacktrackCounters();
			return;
		}

		_suppressTracking = true;
		PrimaryUsageProfile = state.PrimaryUsageProfile;
		StoreInUserProfile = CanUsePortableMode ? state.StoreInUserProfile : true;
		CustomizeInputMappingsNow = state.CustomizeInputMappingsNow;
		EnableXboxMappings = state.EnableXboxMappings;
		EnablePsMappings = state.EnablePsMappings;
		EnableWasdMappings = state.EnableWasdMappings;
		EnableArrowMappings = state.EnableArrowMappings;
		CreateShortcut = state.CreateShortcut;
		CheckForUpdates = state.CheckForUpdates;
		HasResumedDraft = true;
		_suppressTracking = false;
		ResetBacktrackCounters();
	}

	private void ResetBacktrackCounters() {
		_primaryUsageChangeCount = 0;
		_storageChangeCount = 0;
	}

	private void CreateShortcutFile() {
		if (OperatingSystem.IsMacOS()) {
			//TODO OSX
			return;
		}

		if (OperatingSystem.IsWindows()) {
			string linkPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "Nexen.url");
			FileHelper.WriteAllText(linkPath,
				"[InternetShortcut]" + Environment.NewLine +
				"URL=file:///" + Program.ExePath + Environment.NewLine +
				"IconIndex=0" + Environment.NewLine +
				"IconFile=" + Program.ExePath.Replace('\\', '/') + Environment.NewLine
			);
		} else {
			string shortcutFile = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Desktop), "nexen.desktop");
			FileAssociationHelper.CreateLinuxShortcutFile(shortcutFile);
			Process.Start("chmod", "744 " + shortcutFile);
		}
	}

	/// <summary>
	/// Detects whether the application is running inside a Linux AppImage.
	/// AppImages mount to a read-only temp directory, so portable/"same folder" mode cannot work.
	/// </summary>
	private static bool IsRunningInAppImage() {
		return !string.IsNullOrEmpty(Environment.GetEnvironmentVariable("APPIMAGE"));
	}
}
