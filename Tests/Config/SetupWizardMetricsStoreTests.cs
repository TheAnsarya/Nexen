using System;
using System.IO;
using Nexen.Config;
using Xunit;

namespace Nexen.Tests.Config;

public sealed class SetupWizardMetricsStoreTests {
	[Fact]
	public void MetricsStore_AccumulatesLaunchCancelAndCompletionCounters() {
		string tempRoot = Path.Combine(Path.GetTempPath(), "nexen-setup-metrics-tests", Guid.NewGuid().ToString("N"));
		try {
			SetupWizardMetricsStore.RecordLaunch(false, tempRoot);
			SetupWizardMetricsStore.RecordLaunch(true, tempRoot);
			SetupWizardMetricsStore.RecordProfileToggle(tempRoot);
			SetupWizardMetricsStore.RecordStorageToggle(tempRoot);
			SetupWizardMetricsStore.RecordCustomizeToggle(tempRoot);
			SetupWizardMetricsStore.RecordCancel(2, tempRoot);
			SetupWizardMetricsStore.RecordStartFresh(tempRoot);
			SetupWizardMetricsStore.RecordCompletion(3, tempRoot);

			SetupWizardMetrics metrics = SetupWizardMetricsStore.Load(tempRoot);
			Assert.Equal(2, metrics.LaunchCount);
			Assert.Equal(1, metrics.ResumeLaunchCount);
			Assert.Equal(1, metrics.CancelCount);
			Assert.Equal(1, metrics.CompletionCount);
			Assert.Equal(1, metrics.StartFreshCount);
			Assert.Equal(1, metrics.ProfileToggleCount);
			Assert.Equal(1, metrics.StorageToggleCount);
			Assert.Equal(1, metrics.CustomizeToggleCount);
			Assert.Equal(5, metrics.TotalBacktrackCount);
		} finally {
			if (Directory.Exists(tempRoot)) {
				Directory.Delete(tempRoot, true);
			}
		}
	}
}
