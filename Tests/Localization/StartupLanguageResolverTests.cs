using Nexen.Config;
using Nexen.Localization;
using Xunit;

namespace Nexen.Tests.Localization;

[Collection("LocalizationResourceTests")]
public sealed class StartupLanguageResolverTests {
	[Fact]
	public void ResolveLanguageCode_DesignMode_UsesEnglishFallback() {
		string code = StartupLanguageResolver.ResolveLanguageCode(true, false, () => UiLanguage.Spanish);

		Assert.Equal("en", code);
	}

	[Fact]
	public void ResolveLanguageCode_SetupWindow_UsesEnglishFallback() {
		string code = StartupLanguageResolver.ResolveLanguageCode(false, true, () => UiLanguage.Japanese);

		Assert.Equal("en", code);
	}

	[Theory]
	[InlineData(UiLanguage.English, "en")]
	[InlineData(UiLanguage.Spanish, "es")]
	[InlineData(UiLanguage.Japanese, "ja")]
	public void ResolveLanguageCode_ConfiguredLanguage_UsesConfiguredCode(UiLanguage configuredLanguage, string expectedCode) {
		string code = StartupLanguageResolver.ResolveLanguageCode(false, false, () => configuredLanguage);

		Assert.Equal(expectedCode, code);
	}

	[Fact]
	public void ResolveLanguageCode_ConfigReadFailure_UsesEnglishFallback() {
		string code = StartupLanguageResolver.ResolveLanguageCode(false, false, () => throw new InvalidOperationException("config read failed"));

		Assert.Equal("en", code);
	}

	[Fact]
	public void ResolveLanguageCode_StartupPath_SpanishLoadsSpanishMainMenuLabel() {
		string code = StartupLanguageResolver.ResolveLanguageCode(false, false, () => UiLanguage.Spanish);
		ResourceHelper.LoadResources(code);

		Assert.Equal("es", code);
		Assert.Equal("_Archivo", ResourceHelper.GetViewLabel("MainMenuView", "mnuFile"));
	}

	[Fact]
	public void ResolveLanguageCode_StartupPath_JapaneseLoadsJapaneseMainMenuLabel() {
		string code = StartupLanguageResolver.ResolveLanguageCode(false, false, () => UiLanguage.Japanese);
		ResourceHelper.LoadResources(code);

		Assert.Equal("ja", code);
		Assert.Equal("_ファイル", ResourceHelper.GetViewLabel("MainMenuView", "mnuFile"));
	}

	[Fact]
	public void ResolveLanguageCode_DesignModeFallback_LoadsEnglishLabel() {
		string code = StartupLanguageResolver.ResolveLanguageCode(true, false, () => UiLanguage.Japanese);
		ResourceHelper.LoadResources(code);

		Assert.Equal("en", code);
		Assert.Equal("_File", ResourceHelper.GetViewLabel("MainMenuView", "mnuFile"));
	}

	[Fact]
	public void LoadResources_UnknownLanguageCode_FallsBackToEnglishLabels() {
		ResourceHelper.LoadResources("zz");

		Assert.Equal("_File", ResourceHelper.GetViewLabel("MainMenuView", "mnuFile"));
	}

	[Fact]
	public void ResolveLanguageCode_SetupWindowForcesEnglish_AndLoadsEnglishLabel() {
		string code = StartupLanguageResolver.ResolveLanguageCode(false, true, () => UiLanguage.Spanish);
		ResourceHelper.LoadResources(code);

		Assert.Equal("en", code);
		Assert.Equal("_File", ResourceHelper.GetViewLabel("MainMenuView", "mnuFile"));
	}

	[Fact]
	public void GetAvailableLanguageDisplayNames_ContainsExpectedLanguages() {
		string[] displayNames = ResourceHelper.GetAvailableLanguageDisplayNames();

		Assert.Contains("English", displayNames);
		Assert.Contains("Spanish", displayNames);
		Assert.Contains("Japanese", displayNames);
	}

	[Fact]
	public void LoadResources_MixedCaseWhitespaceLanguageCode_LoadsSpanishLabel() {
		ResourceHelper.LoadResources(" Es ");

		Assert.Equal("_Archivo", ResourceHelper.GetViewLabel("MainMenuView", "mnuFile"));
	}

	[Theory]
	[InlineData(null)]
	[InlineData("")]
	[InlineData("   ")]
	public void LoadResources_EmptyLanguageCode_UsesEnglishFallback(string? languageCode) {
		ResourceHelper.LoadResources(languageCode);

		Assert.Equal("_File", ResourceHelper.GetViewLabel("MainMenuView", "mnuFile"));
	}

	[Fact]
	public void GetAvailableLanguageCodes_ReturnsLowercaseCodes() {
		string[] codes = ResourceHelper.GetAvailableLanguageCodes();

		Assert.Contains("en", codes);
		Assert.Contains("es", codes);
		Assert.Contains("ja", codes);
		Assert.DoesNotContain(codes, code => code != code.ToLowerInvariant());
	}

	[Fact]
	public void GetLanguageDisplayName_UnknownCode_ReturnsCode() {
		string displayName = ResourceHelper.GetLanguageDisplayName("zz-unknown");

		Assert.Equal("zz-unknown", displayName);
	}

	[Fact]
	public void GetLanguageDisplayName_MixedCaseKnownCodeWithoutNormalization_ReturnsOriginalCode() {
		string displayName = ResourceHelper.GetLanguageDisplayName(" Ja ");

		Assert.Equal(" Ja ", displayName);
	}

	[Fact]
	public void GetAvailableLanguageCodes_AndDisplayNames_HaveMatchingCount() {
		string[] codes = ResourceHelper.GetAvailableLanguageCodes();
		string[] displayNames = ResourceHelper.GetAvailableLanguageDisplayNames();

		Assert.Equal(codes.Length, displayNames.Length);
	}

	[Fact]
	public void GetAvailableLanguageDisplayNames_ContainsNoEmptyEntries() {
		string[] displayNames = ResourceHelper.GetAvailableLanguageDisplayNames();

		Assert.DoesNotContain(displayNames, string.IsNullOrWhiteSpace);
	}

	[Theory]
	[InlineData("es")]
	[InlineData("ja")]
	public void HighVisibilityMessages_AreLocalized(string languageCode) {
		ResourceHelper.LoadResources(languageCode);

		Assert.NotEqual("Save changes?", ResourceHelper.GetMessage("PromptSaveChanges"));
		Assert.NotEqual("Keep changes?", ResourceHelper.GetMessage("PromptKeepChanges"));
		Assert.NotEqual("An unexpected error has occurred.\n\nError details:\nsample", ResourceHelper.GetMessage("UnexpectedError", "sample"));
		Assert.NotEqual("The selected file is not a valid HD Pack.", ResourceHelper.GetMessage("InstallHdPackInvalidPack"));
		Assert.NotEqual("You are running the latest version of Nexen.", ResourceHelper.GetMessage("NexenUpToDate"));
		Assert.NotEqual("Load from file...", ResourceHelper.GetMessage("LoadFromFile"));
		Assert.NotEqual("Save to file...", ResourceHelper.GetMessage("SaveToFile"));
		Assert.NotEqual("Download failed - the file appears to be corrupted. Please visit the Nexen website to download the latest version manually.", ResourceHelper.GetMessage("UpdateDownloadFailed"));
	}

	[Theory]
	[InlineData("es")]
	[InlineData("ja")]
	public void HighTrafficControlsAndMessages_AreLocalized(string languageCode) {
		ResourceHelper.LoadResources(languageCode);

		Assert.NotEqual("Audio", ResourceHelper.GetViewLabel("NesConfigView", "tpgAudio"));
		Assert.NotEqual("Audio", ResourceHelper.GetViewLabel("ConfigWindow", "tabAudio"));
		Assert.NotEqual("Sprites #1:", ResourceHelper.GetViewLabel("GameboyConfigView", "lblGbObj0"));
		Assert.NotEqual("Sprites #2:", ResourceHelper.GetViewLabel("GameboyConfigView", "lblGbObj1"));
		if (languageCode == "ja") {
			Assert.NotEqual("No", ResourceHelper.GetMessage("btnNo"));
		}
	}
}
