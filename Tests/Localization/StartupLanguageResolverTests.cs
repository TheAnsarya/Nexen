using Nexen.Config;
using Nexen.Localization;
using Xunit;

namespace Nexen.Tests.Localization;

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
}
