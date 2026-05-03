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
}
