using System;
using System.IO;
using System.Text.Json;

namespace Nexen.Config;

public sealed class SetupWizardResumeState {
	public PrimaryUsageProfile PrimaryUsageProfile { get; set; } = PrimaryUsageProfile.Playing;
	public bool StoreInUserProfile { get; set; } = true;
	public bool CustomizeInputMappingsNow { get; set; }
	public bool EnableXboxMappings { get; set; } = true;
	public bool EnablePsMappings { get; set; }
	public bool EnableWasdMappings { get; set; }
	public bool EnableArrowMappings { get; set; } = true;
	public bool CreateShortcut { get; set; } = true;
	public bool CheckForUpdates { get; set; } = true;
}

public static class SetupWizardResumeStateStore {
	private const string StateFileName = "setup-wizard-state.json";

	public static SetupWizardResumeState? Load(string? baseFolder = null) {
		try {
			string path = GetStateFilePath(baseFolder);
			if (!File.Exists(path)) {
				return null;
			}

			string json = File.ReadAllText(path);
			return (SetupWizardResumeState?)JsonSerializer.Deserialize(json, typeof(SetupWizardResumeState), NexenSerializerContext.Default);
		} catch {
			return null;
		}
	}

	public static void Save(SetupWizardResumeState state, string? baseFolder = null) {
		string path = GetStateFilePath(baseFolder);
		string? dir = Path.GetDirectoryName(path);
		if (!string.IsNullOrWhiteSpace(dir)) {
			Directory.CreateDirectory(dir);
		}

		string json = JsonSerializer.Serialize(state, typeof(SetupWizardResumeState), NexenSerializerContext.Default);
		File.WriteAllText(path, json);
	}

	public static void Clear(string? baseFolder = null) {
		string path = GetStateFilePath(baseFolder);
		if (File.Exists(path)) {
			File.Delete(path);
		}
	}

	public static string GetStateFilePath(string? baseFolder = null) {
		string root = string.IsNullOrWhiteSpace(baseFolder) ? ConfigManager.DefaultDocumentsFolder : baseFolder;
		return Path.Combine(root, StateFileName);
	}
}
