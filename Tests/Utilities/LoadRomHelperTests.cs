using System;
using Nexen.Utilities;
using Xunit;

namespace Nexen.Tests.Utilities;

public sealed class LoadRomHelperTests {
	[Fact]
	public void BeginRomLoadOperationForTests_SetsInProgressAndClearsOnDispose() {
		LoadRomHelper.ResetRomLoadInProgressForTests();
		Assert.False(LoadRomHelper.IsRomLoadInProgress);
		Assert.Equal(0, LoadRomHelper.GetRomLoadInProgressCountForTests());

		using (LoadRomHelper.BeginRomLoadOperationForTests("TestLoad")) {
			Assert.True(LoadRomHelper.IsRomLoadInProgress);
			Assert.Equal(1, LoadRomHelper.GetRomLoadInProgressCountForTests());
		}

		Assert.False(LoadRomHelper.IsRomLoadInProgress);
		Assert.Equal(0, LoadRomHelper.GetRomLoadInProgressCountForTests());
	}

	[Fact]
	public void BeginRomLoadOperationForTests_NestedScopes_ReferenceCounted() {
		LoadRomHelper.ResetRomLoadInProgressForTests();

		IDisposable op1 = LoadRomHelper.BeginRomLoadOperationForTests("Outer");
		IDisposable op2 = LoadRomHelper.BeginRomLoadOperationForTests("Inner");

		Assert.True(LoadRomHelper.IsRomLoadInProgress);
		Assert.Equal(2, LoadRomHelper.GetRomLoadInProgressCountForTests());

		op2.Dispose();
		Assert.True(LoadRomHelper.IsRomLoadInProgress);
		Assert.Equal(1, LoadRomHelper.GetRomLoadInProgressCountForTests());

		op1.Dispose();
		Assert.False(LoadRomHelper.IsRomLoadInProgress);
		Assert.Equal(0, LoadRomHelper.GetRomLoadInProgressCountForTests());
	}

	[Fact]
	public void BeginRomLoadOperationForTests_DoubleDispose_DoesNotUnderflowCounter() {
		LoadRomHelper.ResetRomLoadInProgressForTests();
		IDisposable op = LoadRomHelper.BeginRomLoadOperationForTests("DoubleDispose");

		op.Dispose();
		op.Dispose();

		Assert.Equal(0, LoadRomHelper.GetRomLoadInProgressCountForTests());
		Assert.False(LoadRomHelper.IsRomLoadInProgress);
	}
}
