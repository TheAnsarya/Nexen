using System.Reflection;
using Nexen.Services;
using Xunit;

namespace Nexen.Tests.Services;

/// <summary>
/// Tests for <see cref="EmulatorState"/> computed property guards.
/// Validates that menu enable/disable logic is correct for all state combinations.
/// </summary>
public class EmulatorStateTests {
	/// <summary>
	/// Creates a fresh EmulatorState instance via reflection (bypasses private constructor).
	/// Each test gets its own instance to avoid singleton pollution.
	/// </summary>
	private static EmulatorState CreateFreshState() {
		return (EmulatorState)Activator.CreateInstance(
			typeof(EmulatorState),
			BindingFlags.Instance | BindingFlags.NonPublic,
			binder: null, args: null, culture: null)!;
	}

	private static void SetProperty(EmulatorState state, string propertyName, object value) {
		var prop = typeof(EmulatorState).GetProperty(propertyName);
		Assert.NotNull(prop);
		prop!.SetValue(state, value);
	}

	#region IsNetplayActive Tests

	[Theory]
	[InlineData(false, false, false)]
	[InlineData(true, false, true)]
	[InlineData(false, true, true)]
	[InlineData(true, true, true)]
	public void IsNetplayActive_CombinesClientAndServer(bool isClient, bool isServer, bool expected) {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsNetplayClient), isClient);
		SetProperty(state, nameof(EmulatorState.IsNetplayServer), isServer);

		Assert.Equal(expected, state.IsNetplayActive);
	}

	#endregion

	#region CanSaveState Tests

	[Theory]
	[InlineData(false, false, false)]
	[InlineData(true, false, true)]
	[InlineData(false, true, false)]
	[InlineData(true, true, false)]
	public void CanSaveState_RequiresRomLoaded_BlockedByNetplayClient(bool romLoaded, bool netplayClient, bool expected) {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), romLoaded);
		SetProperty(state, nameof(EmulatorState.IsNetplayClient), netplayClient);

		Assert.Equal(expected, state.CanSaveState);
	}

	#endregion

	#region CanLoadState Tests

	[Theory]
	[InlineData(false, false, false)]
	[InlineData(true, false, true)]
	[InlineData(false, true, false)]
	[InlineData(true, true, false)]
	public void CanLoadState_RequiresRomLoaded_BlockedByMoviePlaying(bool romLoaded, bool moviePlaying, bool expected) {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), romLoaded);
		SetProperty(state, nameof(EmulatorState.IsMoviePlaying), moviePlaying);

		Assert.Equal(expected, state.CanLoadState);
	}

	#endregion

	#region CanRecordMovie Tests

	[Theory]
	[InlineData(false, false, false, false)]
	[InlineData(true, false, false, true)]
	[InlineData(true, true, false, false)]
	[InlineData(true, false, true, false)]
	[InlineData(true, true, true, false)]
	[InlineData(false, true, false, false)]
	[InlineData(false, false, true, false)]
	public void CanRecordMovie_RequiresRomLoaded_BlockedByRecordingOrPlaying(
		bool romLoaded, bool movieRecording, bool moviePlaying, bool expected) {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), romLoaded);
		SetProperty(state, nameof(EmulatorState.IsMovieRecording), movieRecording);
		SetProperty(state, nameof(EmulatorState.IsMoviePlaying), moviePlaying);

		Assert.Equal(expected, state.CanRecordMovie);
	}

	#endregion

	#region CanPlayMovie Tests

	[Theory]
	[InlineData(false, false, false, false)]
	[InlineData(true, false, false, true)]
	[InlineData(true, true, false, false)]
	[InlineData(true, false, true, false)]
	[InlineData(true, true, true, false)]
	public void CanPlayMovie_RequiresRomLoaded_BlockedByRecordingOrPlaying(
		bool romLoaded, bool movieRecording, bool moviePlaying, bool expected) {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), romLoaded);
		SetProperty(state, nameof(EmulatorState.IsMovieRecording), movieRecording);
		SetProperty(state, nameof(EmulatorState.IsMoviePlaying), moviePlaying);

		Assert.Equal(expected, state.CanPlayMovie);
	}

	#endregion

	#region CanRecordVideo Tests

	[Theory]
	[InlineData(false, false, false)]
	[InlineData(true, false, true)]
	[InlineData(false, true, false)]
	[InlineData(true, true, false)]
	public void CanRecordVideo_RequiresRomLoaded_BlockedByAviRecording(bool romLoaded, bool aviRecording, bool expected) {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), romLoaded);
		SetProperty(state, nameof(EmulatorState.IsAviRecording), aviRecording);

		Assert.Equal(expected, state.CanRecordVideo);
	}

	#endregion

	#region CanRecordAudio Tests

	[Theory]
	[InlineData(false, false, false)]
	[InlineData(true, false, true)]
	[InlineData(false, true, false)]
	[InlineData(true, true, false)]
	public void CanRecordAudio_RequiresRomLoaded_BlockedByWaveRecording(bool romLoaded, bool waveRecording, bool expected) {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), romLoaded);
		SetProperty(state, nameof(EmulatorState.IsWaveRecording), waveRecording);

		Assert.Equal(expected, state.CanRecordAudio);
	}

	#endregion

	#region Default State Tests

	[Fact]
	public void FreshState_AllFlagsAreFalse() {
		var state = CreateFreshState();

		Assert.False(state.IsRomLoaded);
		Assert.False(state.IsRunning);
		Assert.False(state.IsPaused);
		Assert.False(state.IsMoviePlaying);
		Assert.False(state.IsMovieRecording);
		Assert.False(state.IsAviRecording);
		Assert.False(state.IsWaveRecording);
		Assert.False(state.IsNetplayClient);
		Assert.False(state.IsNetplayServer);
		Assert.False(state.HasSaveStates);
		Assert.False(state.HasRecentFiles);
	}

	[Fact]
	public void FreshState_AllComputedGuardsAreFalse() {
		var state = CreateFreshState();

		Assert.False(state.IsNetplayActive);
		Assert.False(state.CanSaveState);
		Assert.False(state.CanLoadState);
		Assert.False(state.CanRecordMovie);
		Assert.False(state.CanPlayMovie);
		Assert.False(state.CanRecordVideo);
		Assert.False(state.CanRecordAudio);
	}

	[Fact]
	public void RomLoaded_AllActionsBecomeAvailable() {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), true);

		Assert.True(state.CanSaveState);
		Assert.True(state.CanLoadState);
		Assert.True(state.CanRecordMovie);
		Assert.True(state.CanPlayMovie);
		Assert.True(state.CanRecordVideo);
		Assert.True(state.CanRecordAudio);
	}

	#endregion

	#region Cross-Concern Interaction Tests

	[Fact]
	public void NetplayClient_BlocksSaveState_ButNotLoadOrRecord() {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), true);
		SetProperty(state, nameof(EmulatorState.IsNetplayClient), true);

		Assert.False(state.CanSaveState);
		Assert.True(state.CanLoadState);
		Assert.True(state.CanRecordMovie);
		Assert.True(state.CanRecordVideo);
	}

	[Fact]
	public void MoviePlaying_BlocksLoadAndRecordMovie_ButNotSaveOrVideo() {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), true);
		SetProperty(state, nameof(EmulatorState.IsMoviePlaying), true);

		Assert.True(state.CanSaveState);
		Assert.False(state.CanLoadState);
		Assert.False(state.CanRecordMovie);
		Assert.False(state.CanPlayMovie);
		Assert.True(state.CanRecordVideo);
		Assert.True(state.CanRecordAudio);
	}

	[Fact]
	public void MovieRecording_BlocksRecordAndPlayMovie_ButNotSaveOrLoad() {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), true);
		SetProperty(state, nameof(EmulatorState.IsMovieRecording), true);

		Assert.True(state.CanSaveState);
		Assert.True(state.CanLoadState);
		Assert.False(state.CanRecordMovie);
		Assert.False(state.CanPlayMovie);
		Assert.True(state.CanRecordVideo);
	}

	[Fact]
	public void AllRecordingActive_AllRecordGuardsBlocked() {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), true);
		SetProperty(state, nameof(EmulatorState.IsMovieRecording), true);
		SetProperty(state, nameof(EmulatorState.IsAviRecording), true);
		SetProperty(state, nameof(EmulatorState.IsWaveRecording), true);

		Assert.False(state.CanRecordMovie);
		Assert.False(state.CanPlayMovie);
		Assert.False(state.CanRecordVideo);
		Assert.False(state.CanRecordAudio);
		// Save/load still allowed
		Assert.True(state.CanSaveState);
		Assert.True(state.CanLoadState);
	}

	[Fact]
	public void NetplayClient_Plus_MoviePlaying_CombinedBlocking() {
		var state = CreateFreshState();
		SetProperty(state, nameof(EmulatorState.IsRomLoaded), true);
		SetProperty(state, nameof(EmulatorState.IsNetplayClient), true);
		SetProperty(state, nameof(EmulatorState.IsMoviePlaying), true);

		Assert.False(state.CanSaveState);  // blocked by netplay client
		Assert.False(state.CanLoadState);  // blocked by movie playing
		Assert.False(state.CanRecordMovie); // blocked by movie playing
		Assert.False(state.CanPlayMovie);  // blocked by movie playing
		Assert.True(state.CanRecordVideo); // neither blocks it
		Assert.True(state.CanRecordAudio); // neither blocks it
		Assert.True(state.IsNetplayActive);
	}

	#endregion
}
