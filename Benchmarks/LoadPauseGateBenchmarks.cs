using BenchmarkDotNet.Attributes;
using Nexen.Windows;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks pause gating logic with and without ROM-load-in-progress.
/// </summary>
[MemoryDiagnoser]
public class LoadPauseGateBenchmarks {
	[Benchmark(Baseline = true)]
	public bool PauseGate_ActiveGame_MenuOpen_NoLoad() {
		return MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: true,
			isRomLoadInProgress: false,
			isConfigWindow: false,
			isMainMenuOpen: true
		);
	}

	[Benchmark]
	public bool PauseGate_ActiveGame_MenuOpen_LoadInProgress() {
		return MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: true,
			isRomLoadInProgress: true,
			isConfigWindow: false,
			isMainMenuOpen: true
		);
	}

	[Benchmark]
	public bool PauseGate_ActiveGame_ConfigWindow_LoadInProgress() {
		return MainWindow.ShouldPauseInMenusAndConfig(
			pauseWhenInMenusAndConfig: true,
			isGameRunning: true,
			isRomLoadInProgress: true,
			isConfigWindow: true,
			isMainMenuOpen: false
		);
	}
}
