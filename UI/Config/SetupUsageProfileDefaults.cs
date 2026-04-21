using ReactiveUI;

namespace Nexen.Config;

/// <summary>
/// Applies deterministic first-run defaults based on the user's primary usage intent.
/// </summary>
public static class SetupUsageProfileDefaults {
	public static void Apply(Configuration config, PrimaryUsageProfile profile) {
		config.Preferences.PrimaryUsageProfile = profile;

		switch (profile) {
			case PrimaryUsageProfile.Debugging:
				ApplyDebuggingDefaults(config);
				break;
			case PrimaryUsageProfile.Playing:
			default:
				ApplyPlayingDefaults(config);
				break;
		}
	}

	private static void ApplyPlayingDefaults(Configuration config) {
		config.Debug.Integration.BackgroundCdlRecording = false;
		config.Debug.Integration.AutoExportPansy = false;
		config.Debug.Integration.SavePansyOnRomUnload = false;
		config.Debug.Integration.PansyUseCompression = false;
		config.Debug.Integration.EnableFileWatching = false;
		config.Debug.Integration.AutoReloadOnExternalChange = false;
		config.Preferences.ShowDebugInfo = false;
	}

	private static void ApplyDebuggingDefaults(Configuration config) {
		config.Debug.Integration.BackgroundCdlRecording = true;
		config.Debug.Integration.AutoExportPansy = true;
		config.Debug.Integration.SavePansyOnRomUnload = true;
		config.Debug.Integration.PansyUseCompression = false;
		config.Debug.Integration.EnableFileWatching = true;
		config.Debug.Integration.AutoReloadOnExternalChange = true;
		config.Preferences.ShowDebugInfo = true;
	}
}
