using Nexen.ViewModels;
using Nexen.Config;
using Xunit;

namespace Nexen.Tests.ViewModels;

public sealed class SetupWizardViewModelTests {
	[Fact]
	public void CustomizeInputMappingsNow_DefaultsToFalse() {
		var vm = new SetupWizardViewModel();

		Assert.False(vm.CustomizeInputMappingsNow);
	}

	[Fact]
	public void UsePlayingProfile_SelectsPlayingProfile() {
		var vm = new SetupWizardViewModel {
			PrimaryUsageProfile = PrimaryUsageProfile.Debugging
		};

		vm.UsePlayingProfile = true;

		Assert.Equal(PrimaryUsageProfile.Playing, vm.PrimaryUsageProfile);
	}

	[Fact]
	public void UseDebuggingProfile_SelectsDebuggingProfile() {
		var vm = new SetupWizardViewModel {
			PrimaryUsageProfile = PrimaryUsageProfile.Playing
		};

		vm.UseDebuggingProfile = true;

		Assert.Equal(PrimaryUsageProfile.Debugging, vm.PrimaryUsageProfile);
	}

	[Fact]
	public void ResetStorageToRecommended_ForcesUserProfileStorage() {
		var vm = new SetupWizardViewModel {
			StoreInUserProfile = false
		};

		vm.ResetStorageToRecommended();

		Assert.True(vm.StoreInUserProfile);
	}

	[Fact]
	public void StoreInUserProfile_UpdatesResolvedInstallLocation() {
		var vm = new SetupWizardViewModel();

		vm.StoreInUserProfile = true;
		Assert.Equal(ConfigManager.DefaultDocumentsFolder, vm.InstallLocation);

		vm.StoreInUserProfile = false;
		Assert.Equal(ConfigManager.DefaultPortableFolder, vm.InstallLocation);
	}
}
