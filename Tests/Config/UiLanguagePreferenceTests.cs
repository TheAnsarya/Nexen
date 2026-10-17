using System.Text.Json;
using Nexen.Config;
using Xunit;

namespace Nexen.Tests.Config;

public sealed class UiLanguagePreferenceTests {
	[Theory]
	[InlineData(UiLanguage.English, "en")]
	[InlineData(UiLanguage.Spanish, "es")]
	[InlineData(UiLanguage.Japanese, "ja")]
	public void PreferencesConfig_GetLanguageCode_MapsExpectedCodes(UiLanguage language, string expectedCode) {
		Assert.Equal(expectedCode, PreferencesConfig.GetLanguageCode(language));
	}

	[Theory]
	[InlineData(UiLanguage.English)]
	[InlineData(UiLanguage.Spanish)]
	[InlineData(UiLanguage.Japanese)]
	public void Configuration_RoundTrip_PreservesUiLanguagePreference(UiLanguage selectedLanguage) {
		Configuration config = Configuration.CreateConfig();
		config.Preferences.UiLanguage = selectedLanguage;

		string json = JsonSerializer.Serialize(config, typeof(Configuration), NexenSerializerContext.Default);
		Configuration? roundTrip = (Configuration?)JsonSerializer.Deserialize(json, typeof(Configuration), NexenSerializerContext.Default);

		Assert.NotNull(roundTrip);
		Assert.Equal(selectedLanguage, roundTrip!.Preferences.UiLanguage);
	}
}
