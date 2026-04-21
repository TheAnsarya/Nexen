using Nexen.ViewModels;
using Nexen.Config;
using Xunit;

namespace Nexen.Tests.ViewModels;

public sealed class SetupWizardViewModelTests {
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
