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
using Mesen.Interop;
using Mesen.Utilities;

namespace Mesen.Config;

public static partial class ConfigManager {
	private static Configuration? _config;
	private static readonly object _initLock = new();

	public static string DefaultPortableFolder => Path.GetDirectoryName(Program.ExePath) ?? "./";
	public static string DefaultDocumentsFolder {
		get {
			Environment.SpecialFolder folder = OperatingSystem.IsWindows() ? Environment.SpecialFolder.MyDocuments : Environment.SpecialFolder.ApplicationData;
			return Path.Combine(Environment.GetFolderPath(folder, Environment.SpecialFolderOption.Create), "Mesen2");
		}
	}

	public static string DefaultAviFolder => Path.Combine(HomeFolder, "Avi");
	public static string DefaultMovieFolder => Path.Combine(HomeFolder, "Movies");
	public static string DefaultSaveDataFolder => Path.Combine(HomeFolder, "Saves");
	public static string DefaultSaveStateFolder => Path.Combine(HomeFolder, "SaveStates");
	public static string DefaultScreenshotFolder => Path.Combine(HomeFolder, "Screenshots");
	public static string DefaultWaveFolder => Path.Combine(HomeFolder, "Wave");

	public static bool DisableSaveSettings { get; internal set; }

	public static string GetConfigFile() => Path.Combine(HomeFolder, "settings.json");

	public static void CreateConfig(bool portable) {
		string homeFolder = portable ? DefaultPortableFolder : DefaultDocumentsFolder;
		DependencyHelper.ExtractNativeDependencies(homeFolder);
		HomeFolder = homeFolder;
		Config.Save();
	}

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

	public static MesenTheme ActiveTheme { get; set; }

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

	public static void ResetHomeFolder() {
		HomeFolder = null!;
	}

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

	public static string AviFolder => GetFolder(DefaultAviFolder, Config.Preferences.AviFolder, Config.Preferences.OverrideAviFolder);
	public static string MovieFolder => GetFolder(DefaultMovieFolder, Config.Preferences.MovieFolder, Config.Preferences.OverrideMovieFolder);
	public static string SaveFolder => GetFolder(DefaultSaveDataFolder, Config.Preferences.SaveDataFolder, Config.Preferences.OverrideSaveDataFolder);
	public static string SaveStateFolder => GetFolder(DefaultSaveStateFolder, Config.Preferences.SaveStateFolder, Config.Preferences.OverrideSaveStateFolder);
	public static string ScreenshotFolder => GetFolder(DefaultScreenshotFolder, Config.Preferences.ScreenshotFolder, Config.Preferences.OverrideScreenshotFolder);
	public static string WaveFolder => GetFolder(DefaultWaveFolder, Config.Preferences.WaveFolder, Config.Preferences.OverrideWaveFolder);

	public static string CheatFolder => GetFolder(Path.Combine(HomeFolder, "Cheats"), null, false);
	public static string GameConfigFolder => GetFolder(Path.Combine(HomeFolder, "GameConfig"), null, false);
	public static string SatellaviewFolder => GetFolder(Path.Combine(HomeFolder, "Satellaview"), null, false);

	public static string DebuggerFolder => GetFolder(Path.Combine(HomeFolder, "Debugger"), null, false);
	public static string FirmwareFolder => GetFolder(Path.Combine(HomeFolder, "Firmware"), null, false);
	public static string BackupFolder => GetFolder(Path.Combine(HomeFolder, "Backups"), null, false);
	public static string TestFolder => GetFolder(Path.Combine(HomeFolder, "Tests"), null, false);
	public static string HdPackFolder => GetFolder(Path.Combine(HomeFolder, "HdPacks"), null, false);
	public static string RecentGamesFolder => GetFolder(Path.Combine(HomeFolder, "RecentGames"), null, false);

	public static string ConfigFile {
		get {
			if (!Directory.Exists(HomeFolder)) {
				Directory.CreateDirectory(HomeFolder);
			}

			return Path.Combine(HomeFolder, "settings.json");
		}
	}

	public static Configuration Config {
		get {
			LoadConfig();
			return _config!;
		}
	}

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

	public static void RestartMesen() {
		ProcessModule? mainModule = Process.GetCurrentProcess().MainModule;
		if (mainModule?.FileName == null) {
			return;
		}

		SingleInstance.Instance.Dispose();
		Process.Start(mainModule.FileName);
	}
}
