using System;
using System.IO;
using Nexen.Config;
using Xunit;

namespace Nexen.Tests.Config;

public sealed class SetupWizardResumeStateStoreTests {
	[Fact]
	public void SaveLoadAndClear_RoundTripsState() {
		string tempRoot = Path.Combine(Path.GetTempPath(), "nexen-setup-draft-tests", Guid.NewGuid().ToString("N"));
		try {
			var state = new SetupWizardResumeState {
				PrimaryUsageProfile = PrimaryUsageProfile.Debugging,
				StoreInUserProfile = false,
				CustomizeInputMappingsNow = true,
				EnableXboxMappings = false,
				EnablePsMappings = true,
				EnableWasdMappings = true,
				EnableArrowMappings = false,
				CreateShortcut = false,
				CheckForUpdates = false,
			};

			SetupWizardResumeStateStore.Save(state, tempRoot);
			SetupWizardResumeState? loaded = SetupWizardResumeStateStore.Load(tempRoot);

			Assert.NotNull(loaded);
			Assert.Equal(PrimaryUsageProfile.Debugging, loaded!.PrimaryUsageProfile);
			Assert.False(loaded.StoreInUserProfile);
			Assert.True(loaded.CustomizeInputMappingsNow);
			Assert.False(loaded.EnableXboxMappings);
			Assert.True(loaded.EnablePsMappings);
			Assert.True(loaded.EnableWasdMappings);
			Assert.False(loaded.EnableArrowMappings);
			Assert.False(loaded.CreateShortcut);
			Assert.False(loaded.CheckForUpdates);

			SetupWizardResumeStateStore.Clear(tempRoot);
			Assert.Null(SetupWizardResumeStateStore.Load(tempRoot));
		} finally {
			if (Directory.Exists(tempRoot)) {
				Directory.Delete(tempRoot, true);
			}
		}
	}

	[Fact]
	public void Load_InvalidJson_ReturnsNull() {
		string tempRoot = Path.Combine(Path.GetTempPath(), "nexen-setup-draft-tests", Guid.NewGuid().ToString("N"));
		try {
			Directory.CreateDirectory(tempRoot);
			File.WriteAllText(SetupWizardResumeStateStore.GetStateFilePath(tempRoot), "{not json}");

			Assert.Null(SetupWizardResumeStateStore.Load(tempRoot));
		} finally {
			if (Directory.Exists(tempRoot)) {
				Directory.Delete(tempRoot, true);
			}
		}
	}
}
