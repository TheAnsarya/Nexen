using System;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Services;

/// <summary>
/// Global singleton providing reactive emulator state for menu enable/disable logic.
/// Subscribe to properties or use computed properties for menu item conditions.
/// </summary>
/// <remarks>
/// This service centralizes emulator state queries that were previously scattered
/// throughout menu item definitions. It provides a single source of truth and
/// enables reactive updates when state changes.
/// </remarks>
public sealed partial class EmulatorState : ReactiveObject {
	/// <summary>Singleton instance for global access.</summary>
	public static EmulatorState Instance { get; } = new();

	private EmulatorState() {
		// Private constructor for singleton pattern
	}

	// ========================================
	// Core State Flags
	// ========================================

	/// <summary>Gets whether a ROM is currently loaded (format is known).</summary>
	[Reactive] public partial bool IsRomLoaded { get; private set; }

	/// <summary>Gets whether the emulator is actively running (not paused, ROM loaded).</summary>
	[Reactive] public partial bool IsRunning { get; private set; }

	/// <summary>Gets whether the emulator is paused.</summary>
	[Reactive] public partial bool IsPaused { get; private set; }

	// ========================================
	// Recording/Playback State
	// ========================================

	/// <summary>Gets whether a movie is currently playing.</summary>
	[Reactive] public partial bool IsMoviePlaying { get; private set; }

	/// <summary>Gets whether a movie is currently being recorded.</summary>
	[Reactive] public partial bool IsMovieRecording { get; private set; }

	/// <summary>Gets whether AVI video is being recorded.</summary>
	[Reactive] public partial bool IsAviRecording { get; private set; }

	/// <summary>Gets whether WAV audio is being recorded.</summary>
	[Reactive] public partial bool IsWaveRecording { get; private set; }

	// ========================================
	// Netplay State
	// ========================================

	/// <summary>Gets whether connected to a netplay server as client.</summary>
	[Reactive] public partial bool IsNetplayClient { get; private set; }

	/// <summary>Gets whether running as a netplay server.</summary>
	[Reactive] public partial bool IsNetplayServer { get; private set; }

	/// <summary>Gets whether any netplay session is active (client or server).</summary>
	public bool IsNetplayActive => IsNetplayClient || IsNetplayServer;

	// ========================================
	// Content State
	// ========================================

	/// <summary>Gets whether any save states exist for the current ROM.</summary>
	[Reactive] public partial bool HasSaveStates { get; private set; }

	/// <summary>Gets whether the recent files list has entries.</summary>
	[Reactive] public partial bool HasRecentFiles { get; private set; }

	// ========================================
	// ROM-Specific Info
	// ========================================

	/// <summary>Gets the console type of the loaded ROM.</summary>
	[Reactive] public partial ConsoleType ConsoleType { get; private set; }

	/// <summary>Gets the format of the loaded ROM.</summary>
	[Reactive] public partial RomFormat RomFormat { get; private set; }

	// ========================================
	// Computed Properties for Common Checks
	// ========================================

	/// <summary>Gets whether save state is allowed (ROM loaded, not netplay client).</summary>
	public bool CanSaveState => IsRomLoaded && !IsNetplayClient;

	/// <summary>Gets whether load state is allowed (ROM loaded, not during movie playback).</summary>
	public bool CanLoadState => IsRomLoaded && !IsMoviePlaying;

	/// <summary>Gets whether movie recording can start (ROM loaded, not already recording/playing).</summary>
	public bool CanRecordMovie => IsRomLoaded && !IsMovieRecording && !IsMoviePlaying;

	/// <summary>Gets whether movie playback can start (ROM loaded, not already recording/playing).</summary>
	public bool CanPlayMovie => IsRomLoaded && !IsMovieRecording && !IsMoviePlaying;

	/// <summary>Gets whether video recording can start (ROM loaded, not already recording).</summary>
	public bool CanRecordVideo => IsRomLoaded && !IsAviRecording;

	/// <summary>Gets whether audio recording can start (ROM loaded, not already recording).</summary>
	public bool CanRecordAudio => IsRomLoaded && !IsWaveRecording;

	// ========================================
	// State Update Methods
	// ========================================

	/// <summary>
	/// Refreshes all state from the emulator APIs.
	/// Call this when ROM loads/unloads or other major state changes.
	/// </summary>
	public void RefreshAll() {
		var romInfo = EmuApi.GetRomInfo();

		IsRomLoaded = romInfo.Format != RomFormat.Unknown;
		IsRunning = EmuApi.IsRunning();
		IsPaused = EmuApi.IsPaused();

		IsMoviePlaying = RecordApi.MoviePlaying();
		IsMovieRecording = RecordApi.MovieRecording();
		IsAviRecording = RecordApi.AviIsRecording();
		IsWaveRecording = RecordApi.WaveIsRecording();

		IsNetplayClient = NetplayApi.IsConnected();
		IsNetplayServer = NetplayApi.IsServerRunning();

		HasSaveStates = EmuApi.GetSaveStateCount() > 0;
		HasRecentFiles = Config.ConfigManager.Config.RecentFiles.Items.Count > 0;

		ConsoleType = romInfo.ConsoleType;
		RomFormat = romInfo.Format;
	}

	/// <summary>
	/// Updates state for ROM load/unload events.
	/// </summary>
	/// <param name="romInfo">The ROM info for the loaded ROM, or default for unload.</param>
	public void OnRomChanged(RomInfo romInfo) {
		IsRomLoaded = romInfo.Format != RomFormat.Unknown;
		ConsoleType = romInfo.ConsoleType;
		RomFormat = romInfo.Format;
		IsRunning = IsRomLoaded && EmuApi.IsRunning();
		IsPaused = EmuApi.IsPaused();
		HasSaveStates = IsRomLoaded && EmuApi.GetSaveStateCount() > 0;
	}

	/// <summary>
	/// Updates state when pause state changes.
	/// </summary>
	public void OnPauseChanged() {
		IsPaused = EmuApi.IsPaused();
		IsRunning = IsRomLoaded && !IsPaused;
	}

	/// <summary>
	/// Updates state when movie playback/recording state changes.
	/// </summary>
	public void OnMovieStateChanged() {
		IsMoviePlaying = RecordApi.MoviePlaying();
		IsMovieRecording = RecordApi.MovieRecording();
	}

	/// <summary>
	/// Updates state when netplay state changes.
	/// </summary>
	public void OnNetplayStateChanged() {
		IsNetplayClient = NetplayApi.IsConnected();
		IsNetplayServer = NetplayApi.IsServerRunning();
	}

	/// <summary>
	/// Updates state when recording state changes.
	/// </summary>
	public void OnRecordingStateChanged() {
		IsAviRecording = RecordApi.AviIsRecording();
		IsWaveRecording = RecordApi.WaveIsRecording();
	}

	/// <summary>
	/// Updates state when recent files list changes.
	/// </summary>
	public void OnRecentFilesChanged() {
		HasRecentFiles = Config.ConfigManager.Config.RecentFiles.Items.Count > 0;
	}

	/// <summary>
	/// Updates state when save states may have changed.
	/// </summary>
	public void OnSaveStatesChanged() {
		HasSaveStates = IsRomLoaded && EmuApi.GetSaveStateCount() > 0;
	}
}
