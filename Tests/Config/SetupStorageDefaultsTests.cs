using Nexen.Config;
using Xunit;

namespace Nexen.Tests.Config;

public sealed class SetupStorageDefaultsTests {
	[Theory]
	[InlineData(true)]
	[InlineData(false)]
	public void Apply_PersistsStoragePreferenceMetadata(bool storeInUserProfile) {
		var config = Configuration.CreateConfig();
		config.Preferences.PreferUserProfileStorage = !storeInUserProfile;

		SetupStorageDefaults.Apply(config, storeInUserProfile);

		Assert.Equal(storeInUserProfile, config.Preferences.PreferUserProfileStorage);
	}
}
