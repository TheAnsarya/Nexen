using System;
using Nexen.Localization;
using Xunit;

namespace Nexen.Tests.Localization;

[Collection("LocalizationResourceTests")]
public sealed class LocalizationRuntimeRefreshTests {
	[Fact]
	public void LoadResources_RaisesLanguageChangedEvent() {
		int eventCount = 0;
		EventHandler handler = (_, _) => eventCount++;

		ResourceHelper.LanguageChanged += handler;
		try {
			ResourceHelper.LoadResources("en");
		} finally {
			ResourceHelper.LanguageChanged -= handler;
		}

		Assert.Equal(1, eventCount);
	}

	[Fact]
	public void LocalizationBindingSource_IndexerReflectsLanguageSwitch() {
		ResourceHelper.LoadResources("en");
		string englishMenuLabel = LocalizationBindingSource.Instance["MainMenuView|mnuFile"];

		ResourceHelper.LoadResources("es");
		string spanishMenuLabel = LocalizationBindingSource.Instance["MainMenuView|mnuFile"];

		Assert.Equal("_File", englishMenuLabel);
		Assert.Equal("_Archivo", spanishMenuLabel);
		Assert.NotEqual(englishMenuLabel, spanishMenuLabel);

		ResourceHelper.LoadResources("en");
	}
}
