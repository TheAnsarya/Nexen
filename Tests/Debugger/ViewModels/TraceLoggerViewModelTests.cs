using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using Xunit;

namespace Nexen.Tests.Debugger.ViewModels;

public class TraceLoggerViewModelTests {
	[Fact]
	public void GetCpuSpecificTokens_GenesisContainsExpectedRegisters() {
		string[] tokens = TraceLoggerOptionTab.GetCpuSpecificTokens(CpuType.Genesis);

		Assert.Contains("D0", tokens);
		Assert.Contains("A6", tokens);
		Assert.Contains("SP", tokens);
		Assert.Contains("SR", tokens);
	}

	[Fact]
	public void GetCpuSpecificTokens_UnknownCpuTypeFallsBackWithoutThrowing() {
		CpuType unknownCpuType = (CpuType)255;

		string[] tokens = TraceLoggerOptionTab.GetCpuSpecificTokens(unknownCpuType);

		Assert.Equal(3, tokens.Length);
		Assert.Contains("PC", tokens);
		Assert.Contains("ByteCode", tokens);
		Assert.Contains("Disassembly", tokens);
	}
}
