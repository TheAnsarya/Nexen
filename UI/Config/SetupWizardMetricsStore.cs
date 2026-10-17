using System;
using System.IO;
using System.Text.Json;

namespace Nexen.Config;

public sealed class SetupWizardMetrics {
	public int LaunchCount { get; set; }
	public int ResumeLaunchCount { get; set; }
	public int CompletionCount { get; set; }
	public int CancelCount { get; set; }
	public int StartFreshCount { get; set; }
	public int ProfileToggleCount { get; set; }
	public int StorageToggleCount { get; set; }
	public int CustomizeToggleCount { get; set; }
	public int TotalBacktrackCount { get; set; }
}

public static class SetupWizardMetricsStore {
	private const string MetricsFileName = "setup-wizard-metrics.json";

	public static SetupWizardMetrics Load(string? baseFolder = null) {
		try {
			string path = GetMetricsFilePath(baseFolder);
			if (!File.Exists(path)) {
				return new SetupWizardMetrics();
			}

			string json = File.ReadAllText(path);
			return (SetupWizardMetrics?)JsonSerializer.Deserialize(json, typeof(SetupWizardMetrics), NexenSerializerContext.Default) ?? new SetupWizardMetrics();
		} catch {
			return new SetupWizardMetrics();
		}
	}

	public static void Save(SetupWizardMetrics metrics, string? baseFolder = null) {
		string path = GetMetricsFilePath(baseFolder);
		string? dir = Path.GetDirectoryName(path);
		if (!string.IsNullOrWhiteSpace(dir)) {
			Directory.CreateDirectory(dir);
		}

		string json = JsonSerializer.Serialize(metrics, typeof(SetupWizardMetrics), NexenSerializerContext.Default);
		File.WriteAllText(path, json);
	}

	public static void RecordLaunch(bool resumedDraft, string? baseFolder = null) {
		SetupWizardMetrics metrics = Load(baseFolder);
		metrics.LaunchCount++;
		if (resumedDraft) {
			metrics.ResumeLaunchCount++;
		}
		Save(metrics, baseFolder);
	}

	public static void RecordCompletion(int backtrackCount, string? baseFolder = null) {
		SetupWizardMetrics metrics = Load(baseFolder);
		metrics.CompletionCount++;
		metrics.TotalBacktrackCount += Math.Max(0, backtrackCount);
		Save(metrics, baseFolder);
	}

	public static void RecordCancel(int backtrackCount, string? baseFolder = null) {
		SetupWizardMetrics metrics = Load(baseFolder);
		metrics.CancelCount++;
		metrics.TotalBacktrackCount += Math.Max(0, backtrackCount);
		Save(metrics, baseFolder);
	}

	public static void RecordStartFresh(string? baseFolder = null) {
		SetupWizardMetrics metrics = Load(baseFolder);
		metrics.StartFreshCount++;
		Save(metrics, baseFolder);
	}

	public static void RecordProfileToggle(string? baseFolder = null) {
		SetupWizardMetrics metrics = Load(baseFolder);
		metrics.ProfileToggleCount++;
		Save(metrics, baseFolder);
	}

	public static void RecordStorageToggle(string? baseFolder = null) {
		SetupWizardMetrics metrics = Load(baseFolder);
		metrics.StorageToggleCount++;
		Save(metrics, baseFolder);
	}

	public static void RecordCustomizeToggle(string? baseFolder = null) {
		SetupWizardMetrics metrics = Load(baseFolder);
		metrics.CustomizeToggleCount++;
		Save(metrics, baseFolder);
	}

	public static string GetMetricsFilePath(string? baseFolder = null) {
		string root = string.IsNullOrWhiteSpace(baseFolder) ? ConfigManager.DefaultDocumentsFolder : baseFolder;
		return Path.Combine(root, MetricsFileName);
	}
}
