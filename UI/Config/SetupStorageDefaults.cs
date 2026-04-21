namespace Nexen.Config;

/// <summary>
/// Applies first-run storage preference metadata so later migrations/settings can reference the user's original intent.
/// </summary>
public static class SetupStorageDefaults {
	public static void Apply(Configuration config, bool storeInUserProfile) {
		config.Preferences.PreferUserProfileStorage = storeInUserProfile;
	}
}
