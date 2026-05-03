using System;
using Nexen.Config;

namespace Nexen.Localization;

public static class StartupLanguageResolver {
	public static string ResolveLanguageCode(bool isDesignMode, bool showConfigWindow, Func<UiLanguage> configuredLanguageProvider) {
		if (isDesignMode || showConfigWindow) {
			return "en";
		}

		try {
			return PreferencesConfig.GetLanguageCode(configuredLanguageProvider());
		} catch {
			return "en";
		}
	}
}
