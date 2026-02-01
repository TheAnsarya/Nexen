using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Avalonia.Controls;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Config;

/// <summary>
/// Central configuration management for the Nexen emulator.
/// Handles configuration loading, saving, folder paths, and command-line switch processing.
/// </summary>
/// <remarks>
/// <para>
/// ConfigManager supports two installation modes:
/// <list type="bullet">
/// <item><description>Portable mode: Configuration stored alongside the executable</description></item>
/// <item><description>Documents mode: Configuration stored in user's Documents/AppData folder</description></item>
/// </list>
/// </para>
/// <para>
/// The configuration file (settings.json) is automatically loaded on first access and
/// saved when modified. Thread-safe initialization is ensured via double-checked locking.
/// </para>
/// </remarks>
public static partial class ConfigManager {
	/// <summary>
	/// The loaded configuration instance. Lazily initialized on first access.
	/// </summary>
	private static Configuration? _config;

	/// <summary>
	/// Lock object for thread-safe configuration initialization.
	/// </summary>
	private static readonly object _initLock = new();

	/// <summary>
	/// Gets the default folder path for portable installation (same directory as executable).
	/// </summary>
	public static string DefaultPortableFolder => Path.GetDirectoryName(Program.ExePath) ?? "./";

	/// <summary>
	/// Gets the default folder path for documents-based installation.
	/// On Windows, uses MyDocuments; on other platforms, uses ApplicationData.
	/// </summary>
	public static string DefaultDocumentsFolder {
		get {
			Environment.SpecialFolder folder = OperatingSystem.IsWindows() ? Environment.SpecialFolder.MyDocuments : Environment.SpecialFolder.ApplicationData;
			return Path.Combine(Environment.GetFolderPath(folder, Environment.SpecialFolderOption.Create), "Nexen");
		}
	}

	/// <summary>Gets the default folder path for AVI video recordings.</summary>
	public static string DefaultAviFolder => Path.Combine(HomeFolder, "Avi");

	/// <summary>Gets the default folder path for movie/input recordings.</summary>
	public static string DefaultMovieFolder => Path.Combine(HomeFolder, "Movies");

	/// <summary>Gets the default folder path for save data (battery saves, SRAM).</summary>
	public static string DefaultSaveDataFolder => Path.Combine(HomeFolder, "Saves");

	/// <summary>Gets the default folder path for save states.</summary>
	public static string DefaultSaveStateFolder => Path.Combine(HomeFolder, "SaveStates");

	/// <summary>Gets the default folder path for screenshots.</summary>
	public static string DefaultScreenshotFolder => Path.Combine(HomeFolder, "Screenshots");

	/// <summary>Gets the default folder path for WAV audio recordings.</summary>
	public static string DefaultWaveFolder => Path.Combine(HomeFolder, "Wave");

	/// <summary>
	/// Gets or sets whether settings saving is disabled.
	/// Used during testing or when running with restricted permissions.
	/// </summary>
	public static bool DisableSaveSettings { get; internal set; }

	/// <summary>
	/// Gets the full path to the configuration file.
	/// </summary>
	/// <returns>The absolute path to settings.json in the home folder.</returns>
	public static string GetConfigFile() => Path.Combine(HomeFolder, "settings.json");

	/// <summary>
	/// Creates a new configuration file in either portable or documents mode.
	/// Extracts native dependencies and initializes the home folder.
	/// </summary>
	/// <param name="portable">
	/// If <c>true</c>, creates configuration in the portable folder (same as executable).
	/// If <c>false</c>, creates configuration in the documents folder.
	/// </param>
	public static void CreateConfig(bool portable) {
		string homeFolder = portable ? DefaultPortableFolder : DefaultDocumentsFolder;
		DependencyHelper.ExtractNativeDependencies(homeFolder);
		HomeFolder = homeFolder;
		Config.Save();
	}

	/// <summary>
	/// Loads the configuration from disk. Thread-safe with double-checked locking.
	/// If no configuration file exists, creates a default configuration.
	/// </summary>
	/// <remarks>
	/// This method is automatically called when accessing <see cref="Config"/>.
	/// In Avalonia design mode, always creates a default configuration.
	/// </remarks>
	public static void LoadConfig() {
		if (_config == null) {
			lock (_initLock) {
				if (_config == null) {
					_config = File.Exists(ConfigFile) && !Design.IsDesignMode ? Configuration.Deserialize(ConfigFile) : Configuration.CreateConfig();
					ActiveTheme = _config.Preferences.Theme;
				}
			}
		}
	}

	/// <summary>
	/// Gets or sets the currently active UI theme.
	/// </summary>
	public static NexenTheme ActiveTheme { get; set; }

	/// <summary>
	/// Applies a setting value to a property using reflection.
	/// Handles type conversion and validation via MinMax and ValidValues attributes.
	/// </summary>
	/// <param name="instance">The object instance containing the property.</param>
	/// <param name="property">The property to set.</param>
	/// <param name="value">The string value to parse and apply.</param>
	/// <returns><c>true</c> if the setting was applied successfully; otherwise, <c>false</c>.</returns>
	private static bool ApplySetting(object instance, PropertyInfo property, string value) {
		Type t = property.PropertyType;
		try {
			if (!property.CanWrite) {
				return false;
			}

			if (t == typeof(int) || t == typeof(uint) || t == typeof(double)) {
				if (property.GetCustomAttribute<MinMaxAttribute>() is MinMaxAttribute minMaxAttribute) {
					if (t == typeof(int)) {
						if (int.TryParse(value, out int result)) {
							if (result >= (int)minMaxAttribute.Min && result <= (int)minMaxAttribute.Max) {
								property.SetValue(instance, result);
							} else {
								return false;
							}
						}
					} else if (t == typeof(uint)) {
						if (uint.TryParse(value, out uint result)) {
							if (result >= (uint)(int)minMaxAttribute.Min && result <= (uint)(int)minMaxAttribute.Max) {
								property.SetValue(instance, result);
							} else {
								return false;
							}
						}
					} else if (t == typeof(double)) {
						if (double.TryParse(value, out double result)) {
							if (result >= (double)minMaxAttribute.Min && result <= (double)minMaxAttribute.Max) {
								property.SetValue(instance, result);
							} else {
								return false;
							}
						}
					}
				}
			} else if (t == typeof(bool)) {
				if (bool.TryParse(value, out bool boolValue)) {
					property.SetValue(instance, boolValue);
				} else {
					return false;
				}
			} else if (t.IsEnum) {
				if (Enum.TryParse(t, value, true, out object? enumValue)) {
					if (property.GetCustomAttribute<ValidValuesAttribute>() is ValidValuesAttribute validValuesAttribute) {
						if (validValuesAttribute.ValidValues.Contains(enumValue)) {
							property.SetValue(instance, enumValue);
						} else {
							return false;
						}
					} else {
						property.SetValue(instance, enumValue);
					}
				} else {
					return false;
				}
			}
		} catch {
			return false;
		}

		return true;
	}

	[GeneratedRegex("([a-z0-9_A-Z.]+)=([a-z0-9_A-Z.\\-]+)")]
	private static partial Regex SwitchRegex();

	/// <summary>
	/// Processes a command-line switch argument in the format "path.to.setting=value".
	/// Navigates the configuration object graph and applies the value.
	/// </summary>
	/// <param name="switchArg">The switch argument string (e.g., "Video.Scale=3").</param>
	/// <returns><c>true</c> if the switch was processed successfully; otherwise, <c>false</c>.</returns>
	/// <example>
	/// <code>
	/// ConfigManager.ProcessSwitch("Video.Scale=3");
	/// ConfigManager.ProcessSwitch("Audio.Volume=75");
	/// </code>
	/// </example>
	[RequiresUnreferencedCode("Uses reflection to access properties dynamically")]
	public static bool ProcessSwitch(string switchArg) {
		Match match = SwitchRegex().Match(switchArg);
		if (match.Success) {
			string[] switchPath = match.Groups[1].Value.Split(".");
			string switchValue = match.Groups[2].Value;

			object? cfg = Config;
			PropertyInfo? property;
			for (int i = 0; i < switchPath.Length; i++) {
				property = cfg.GetType().GetProperty(switchPath[i], BindingFlags.Public | BindingFlags.Instance | BindingFlags.IgnoreCase);
				if (property == null) {
					//Invalid switch name
					return false;
				}

				if (i < switchPath.Length - 1) {
					cfg = property.GetValue(cfg);
					if (cfg == null) {
						//Invalid
						return false;
					}
				} else {
					return ApplySetting(cfg, property, switchValue);
				}
			}
		}

		return false;
	}

	/// <summary>
	/// Resets the home folder cache, forcing it to be re-detected on next access.
	/// </summary>
	public static void ResetHomeFolder() {
		HomeFolder = null!;
	}

	/// <summary>
	/// Gets or sets the home folder path where all Nexen data is stored.
	/// Auto-detects portable vs documents mode on first access based on presence of settings.json.
	/// </summary>
	/// <remarks>
	/// The home folder is determined by checking if a portable configuration exists.
	/// If settings.json exists in the executable directory, portable mode is used.
	/// Otherwise, the documents folder is used.
	/// </remarks>
	public static string HomeFolder {
		get {
			if (field == null) {
				string portableFolder = DefaultPortableFolder;
				string documentsFolder = DefaultDocumentsFolder;

				string portableConfig = Path.Combine(portableFolder, "settings.json");
				field = File.Exists(portableConfig) ? portableFolder : documentsFolder;

				Directory.CreateDirectory(field);
			}

			return field;
		}

		private set;
	} = null!;

	/// <summary>
	/// Gets a folder path, creating it if necessary. Falls back to default if creation fails.
	/// </summary>
	/// <param name="defaultFolderName">The default folder path to use.</param>
	/// <param name="overrideFolder">An optional override folder path.</param>
	/// <param name="useOverride">Whether to use the override folder if specified.</param>
	/// <returns>The resolved folder path (guaranteed to exist or fall back to default).</returns>
	public static string GetFolder(string defaultFolderName, string? overrideFolder, bool useOverride) {
		string folder = useOverride && overrideFolder != null ? overrideFolder : defaultFolderName;
		try {
			if (!Directory.Exists(folder)) {
				Directory.CreateDirectory(folder);
			}
		} catch {
			//If the folder doesn't exist and we couldn't create it, use the default folder
			EmuApi.WriteLogEntry("[UI] Folder could not be created: " + folder);
			folder = defaultFolderName;
		}

		return folder;
	}

	/// <summary>Gets the AVI folder, respecting user override settings.</summary>
	public static string AviFolder => GetFolder(DefaultAviFolder, Config.Preferences.AviFolder, Config.Preferences.OverrideAviFolder);

	/// <summary>Gets the movie folder, respecting user override settings.</summary>
	public static string MovieFolder => GetFolder(DefaultMovieFolder, Config.Preferences.MovieFolder, Config.Preferences.OverrideMovieFolder);

	/// <summary>Gets the save data folder, respecting user override settings.</summary>
	public static string SaveFolder => GetFolder(DefaultSaveDataFolder, Config.Preferences.SaveDataFolder, Config.Preferences.OverrideSaveDataFolder);

	/// <summary>Gets the save state folder, respecting user override settings.</summary>
	public static string SaveStateFolder => GetFolder(DefaultSaveStateFolder, Config.Preferences.SaveStateFolder, Config.Preferences.OverrideSaveStateFolder);

	/// <summary>Gets the screenshot folder, respecting user override settings.</summary>
	public static string ScreenshotFolder => GetFolder(DefaultScreenshotFolder, Config.Preferences.ScreenshotFolder, Config.Preferences.OverrideScreenshotFolder);

	/// <summary>Gets the wave folder, respecting user override settings.</summary>
	public static string WaveFolder => GetFolder(DefaultWaveFolder, Config.Preferences.WaveFolder, Config.Preferences.OverrideWaveFolder);

	/// <summary>Gets the cheat codes folder.</summary>
	public static string CheatFolder => GetFolder(Path.Combine(HomeFolder, "Cheats"), null, false);

	/// <summary>Gets the per-game configuration folder.</summary>
	public static string GameConfigFolder => GetFolder(Path.Combine(HomeFolder, "GameConfig"), null, false);

	/// <summary>Gets the Satellaview data folder (SNES BS-X).</summary>
	public static string SatellaviewFolder => GetFolder(Path.Combine(HomeFolder, "Satellaview"), null, false);

	/// <summary>Gets the debugger data folder (labels, CDL files, Pansy exports).</summary>
	public static string DebuggerFolder => GetFolder(Path.Combine(HomeFolder, "Debugger"), null, false);

	/// <summary>Gets the firmware/BIOS folder.</summary>
	public static string FirmwareFolder => GetFolder(Path.Combine(HomeFolder, "Firmware"), null, false);

	/// <summary>Gets the backup folder for ROM modifications.</summary>
	public static string BackupFolder => GetFolder(Path.Combine(HomeFolder, "Backups"), null, false);

	/// <summary>Gets the test folder for automated testing.</summary>
	public static string TestFolder => GetFolder(Path.Combine(HomeFolder, "Tests"), null, false);

	/// <summary>Gets the HD pack folder for NES HD graphics packs.</summary>
	public static string HdPackFolder => GetFolder(Path.Combine(HomeFolder, "HdPacks"), null, false);

	/// <summary>Gets the recent games thumbnail folder.</summary>
	public static string RecentGamesFolder => GetFolder(Path.Combine(HomeFolder, "RecentGames"), null, false);

	/// <summary>
	/// Gets the full path to the configuration file, creating the home folder if needed.
	/// </summary>
	public static string ConfigFile {
		get {
			if (!Directory.Exists(HomeFolder)) {
				Directory.CreateDirectory(HomeFolder);
			}

			return Path.Combine(HomeFolder, "settings.json");
		}
	}

	/// <summary>
	/// Gets the loaded configuration instance. Automatically loads from disk on first access.
	/// </summary>
	/// <remarks>
	/// Thread-safe. Will create a default configuration if settings.json doesn't exist.
	/// </remarks>
	public static Configuration Config {
		get {
			LoadConfig();
			return _config!;
		}
	}

	/// <summary>
	/// Resets all settings to defaults, preserving default key mappings.
	/// </summary>
	/// <param name="initDefaults">
	/// If <c>true</c>, initializes system-specific defaults and marks config as upgraded.
	/// </param>
	public static void ResetSettings(bool initDefaults = true) {
		DefaultKeyMappingType defaultMappings = Config.DefaultKeyMappings;
		if (defaultMappings == DefaultKeyMappingType.None) {
			defaultMappings = DefaultKeyMappingType.Xbox | DefaultKeyMappingType.ArrowKeys;
		}

		_config = Configuration.CreateConfig();
		Config.DefaultKeyMappings = defaultMappings;
		if (initDefaults) {
			Config.InitializeDefaults();
			Config.ConfigUpgrade = (int)ConfigUpgradeHint.NextValue - 1;
		}

		Config.Save();
		Config.ApplyConfig();
	}

	/// <summary>
	/// Restarts the Nexen application by launching a new process and disposing the current instance.
	/// </summary>
	/// <remarks>
	/// Used after major configuration changes that require a full restart.
	/// </remarks>
	public static void RestartNexen() {
		ProcessModule? mainModule = Process.GetCurrentProcess().MainModule;
		if (mainModule?.FileName == null) {
			return;
		}

		SingleInstance.Instance.Dispose();
		Process.Start(mainModule.FileName);
	}
}
