using Nexen.Config;
using Xunit;

namespace Nexen.Tests.Config;

public sealed class SetupUsageProfileDefaultsTests {
	[Fact]
	public void Apply_PlayingProfile_DisablesDebuggerHeavyDefaults() {
		var config = Configuration.CreateConfig();
		config.Debug.Integration.BackgroundCdlRecording = true;
		config.Debug.Integration.AutoExportPansy = true;
		config.Debug.Integration.SavePansyOnRomUnload = true;
		config.Debug.Integration.EnableFileWatching = true;
		config.Debug.Integration.AutoReloadOnExternalChange = true;
		config.Preferences.ShowDebugInfo = true;

		SetupUsageProfileDefaults.Apply(config, PrimaryUsageProfile.Playing);

		Assert.Equal(PrimaryUsageProfile.Playing, config.Preferences.PrimaryUsageProfile);
		Assert.False(config.Debug.Integration.BackgroundCdlRecording);
		Assert.False(config.Debug.Integration.AutoExportPansy);
		Assert.False(config.Debug.Integration.SavePansyOnRomUnload);
		Assert.False(config.Debug.Integration.EnableFileWatching);
		Assert.False(config.Debug.Integration.AutoReloadOnExternalChange);
		Assert.False(config.Preferences.ShowDebugInfo);
	}

	[Fact]
	public void Apply_DebuggingProfile_EnablesDebuggerFriendlyDefaults() {
		var config = Configuration.CreateConfig();
		config.Debug.Integration.BackgroundCdlRecording = false;
		config.Debug.Integration.AutoExportPansy = false;
		config.Debug.Integration.SavePansyOnRomUnload = false;
		config.Debug.Integration.EnableFileWatching = false;
		config.Debug.Integration.AutoReloadOnExternalChange = false;
		config.Preferences.ShowDebugInfo = false;

		SetupUsageProfileDefaults.Apply(config, PrimaryUsageProfile.Debugging);

		Assert.Equal(PrimaryUsageProfile.Debugging, config.Preferences.PrimaryUsageProfile);
		Assert.True(config.Debug.Integration.BackgroundCdlRecording);
		Assert.True(config.Debug.Integration.AutoExportPansy);
		Assert.True(config.Debug.Integration.SavePansyOnRomUnload);
		Assert.True(config.Debug.Integration.EnableFileWatching);
		Assert.True(config.Debug.Integration.AutoReloadOnExternalChange);
		Assert.True(config.Preferences.ShowDebugInfo);
	}
}
