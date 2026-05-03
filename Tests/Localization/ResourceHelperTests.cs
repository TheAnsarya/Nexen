using Nexen.Localization;
using Xunit;

namespace Nexen.Tests.Localization;

public sealed class ResourceHelperTests {
	[Fact]
	public void GetAvailableLanguageCodes_IncludesEnglish() {
		string[] languageCodes = ResourceHelper.GetAvailableLanguageCodes();

		Assert.Contains("en", languageCodes);
	}

	[Fact]
	public void GetMessage_MissingKeyReturnsKeyInsteadOfPlaceholder() {
		ResourceHelper.LoadResources("en");

		string value = ResourceHelper.GetMessage("__definitely_missing_message_key__");

		Assert.Equal("__definitely_missing_message_key__", value);
		Assert.DoesNotContain("[[", value);
	}
}
