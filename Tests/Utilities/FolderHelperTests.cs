using Nexen.Utilities;
using Xunit;

namespace Nexen.Tests.Utilities;

public sealed class FolderHelperTests {
	[Theory]
	[InlineData("Sonic The Hedgehog (W) (REV00) [!].bin")]
	[InlineData("Sonic The Hedgehog (W) (REV00) [!].BIN")]
	public void IsRomFile_AcceptsBinExtension(string fileName) {
		Assert.True(FolderHelper.IsRomFile(fileName));
	}

	[Fact]
	public void IsRomFile_RejectsNonRomExtension() {
		Assert.False(FolderHelper.IsRomFile("readme.txt"));
	}

	[Fact]
	public void GetRomExtensionsForSystem_GenesisIncludesBin() {
		string[] extensions = FolderHelper.GetRomExtensionsForSystem("genesis");

		Assert.Contains(".bin", extensions);
		Assert.Contains(".md", extensions);
	}

	[Fact]
	public void GetRomExtensionsForSystem_ChannelFIncludesChfAndBin() {
		string[] extensions = FolderHelper.GetRomExtensionsForSystem("channelf");

		Assert.Contains(".chf", extensions);
		Assert.Contains(".bin", extensions);
	}

	[Fact]
	public void GetRomFilePatternsForSystem_UnknownSystemReturnsEmpty() {
		Assert.Empty(FolderHelper.GetRomFilePatternsForSystem("unknown-system"));
	}
}
