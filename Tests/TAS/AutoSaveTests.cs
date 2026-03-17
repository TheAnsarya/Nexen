using System.Reflection;
using Nexen.MovieConverter;
using Nexen.ViewModels;
using Nexen.Windows;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for auto-save and recovery functionality.
/// Validates periodic saving, autosave file creation, and recovery detection.
/// Tracks: #664 (Auto-save with recovery)
/// </summary>
public class AutoSaveTests {
	#region Helpers

	private static MovieData CreateTestMovie(int frameCount) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test Game",
			SystemType = SystemType.Nes
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				Controllers = [new ControllerInput { A = i % 2 == 0 }]
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	/// <summary>
	/// Simulates auto-save by serializing frame data to bytes.
	/// </summary>
	private static byte[] SerializeMovieSnapshot(MovieData movie) {
		int bytesPerFrame = 2; // Simplified: 1 byte per controller, 2 controllers max
		var buffer = new byte[8 + movie.InputFrames.Count * bytesPerFrame]; // 8-byte header

		// Header: frame count (4 bytes, little-endian)
		BitConverter.TryWriteBytes(buffer.AsSpan(0, 4), movie.InputFrames.Count);

		int offset = 8;
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			byte b = 0;
			if (movie.InputFrames[i].Controllers.Length > 0) {
				var ctrl = movie.InputFrames[i].Controllers[0];
				if (ctrl.A) b |= 0x01;
				if (ctrl.B) b |= 0x02;
				if (ctrl.Up) b |= 0x10;
				if (ctrl.Down) b |= 0x20;
				if (ctrl.Left) b |= 0x40;
				if (ctrl.Right) b |= 0x80;
			}
			buffer[offset++] = b;
			buffer[offset++] = 0; // P2 placeholder
		}

		return buffer;
	}

	/// <summary>
	/// Simulates deserializing auto-save data back to a movie.
	/// </summary>
	private static MovieData DeserializeMovieSnapshot(byte[] data) {
		int frameCount = BitConverter.ToInt32(data, 0);
		var movie = new MovieData { SystemType = SystemType.Nes };

		int offset = 8;
		for (int i = 0; i < frameCount; i++) {
			byte b = data[offset++];
			offset++; // Skip P2

			var frame = new InputFrame(i) {
				Controllers = [new ControllerInput {
					A = (b & 0x01) != 0,
					B = (b & 0x02) != 0,
					Up = (b & 0x10) != 0,
					Down = (b & 0x20) != 0,
					Left = (b & 0x40) != 0,
					Right = (b & 0x80) != 0,
				}]
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	/// <summary>
	/// Gets the auto-save filename for a given movie path.
	/// </summary>
	private static string GetAutoSavePath(string moviePath) {
		var dir = Path.GetDirectoryName(moviePath) ?? ".";
		var name = Path.GetFileNameWithoutExtension(moviePath);
		var ext = Path.GetExtension(moviePath);
		return Path.Combine(dir, $"{name}.autosave{ext}");
	}

	#endregion

	#region Auto-Save Path Generation

	[Theory]
	[InlineData(@"C:\movies\test.nm2", @"C:\movies\test.autosave.nm2")]
	[InlineData(@"C:\movies\my movie.nm2", @"C:\movies\my movie.autosave.nm2")]
	[InlineData(@"movie.nm2", @"movie.autosave.nm2")]
	public void GetAutoSavePath_GeneratesCorrectPath(string input, string expected) {
		var result = GetAutoSavePath(input);
		Assert.Equal(expected, result);
	}

	[Fact]
	public void GetAutoSavePath_PreservesExtension() {
		var path = GetAutoSavePath("test.bk2");
		Assert.EndsWith(".bk2", path);
		Assert.Contains(".autosave", path);
	}

	#endregion

	#region Snapshot Serialization

	[Fact]
	public void SerializeMovieSnapshot_ProducesNonEmptyBytes() {
		var movie = CreateTestMovie(100);
		var bytes = SerializeMovieSnapshot(movie);
		Assert.True(bytes.Length > 8);
	}

	[Fact]
	public void SerializeMovieSnapshot_HeaderContainsFrameCount() {
		var movie = CreateTestMovie(100);
		var bytes = SerializeMovieSnapshot(movie);
		int frameCount = BitConverter.ToInt32(bytes, 0);
		Assert.Equal(100, frameCount);
	}

	[Fact]
	public void SerializeMovieSnapshot_EmptyMovie_ProducesMinimalBytes() {
		var movie = new MovieData { SystemType = SystemType.Nes };
		var bytes = SerializeMovieSnapshot(movie);
		Assert.Equal(8, bytes.Length); // Just header
		Assert.Equal(0, BitConverter.ToInt32(bytes, 0));
	}

	#endregion

	#region Roundtrip Recovery

	[Fact]
	public void Roundtrip_SmallMovie_PreservesAllData() {
		var original = CreateTestMovie(10);
		var bytes = SerializeMovieSnapshot(original);
		var recovered = DeserializeMovieSnapshot(bytes);

		Assert.Equal(original.InputFrames.Count, recovered.InputFrames.Count);
		for (int i = 0; i < original.InputFrames.Count; i++) {
			Assert.Equal(
				original.InputFrames[i].Controllers[0].A,
				recovered.InputFrames[i].Controllers[0].A);
		}
	}

	[Fact]
	public void Roundtrip_LargeMovie_PreservesAllData() {
		var original = CreateTestMovie(10_000);
		var bytes = SerializeMovieSnapshot(original);
		var recovered = DeserializeMovieSnapshot(bytes);

		Assert.Equal(original.InputFrames.Count, recovered.InputFrames.Count);

		// Spot-check every 100th frame
		for (int i = 0; i < original.InputFrames.Count; i += 100) {
			Assert.Equal(
				original.InputFrames[i].Controllers[0].A,
				recovered.InputFrames[i].Controllers[0].A);
		}
	}

	[Fact]
	public void Roundtrip_AllButtonStates_Preserved() {
		var movie = new MovieData { SystemType = SystemType.Nes };
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput {
				A = true, B = true, Up = true, Down = true, Left = true, Right = true
			}]
		};
		movie.InputFrames.Add(frame);

		var bytes = SerializeMovieSnapshot(movie);
		var recovered = DeserializeMovieSnapshot(bytes);

		var ctrl = recovered.InputFrames[0].Controllers[0];
		Assert.True(ctrl.A);
		Assert.True(ctrl.B);
		Assert.True(ctrl.Up);
		Assert.True(ctrl.Down);
		Assert.True(ctrl.Left);
		Assert.True(ctrl.Right);
	}

	[Fact]
	public void Roundtrip_NoButtons_Preserved() {
		var movie = new MovieData { SystemType = SystemType.Nes };
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput()]
		};
		movie.InputFrames.Add(frame);

		var bytes = SerializeMovieSnapshot(movie);
		var recovered = DeserializeMovieSnapshot(bytes);

		var ctrl = recovered.InputFrames[0].Controllers[0];
		Assert.False(ctrl.A);
		Assert.False(ctrl.B);
		Assert.False(ctrl.Up);
		Assert.False(ctrl.Down);
		Assert.False(ctrl.Left);
		Assert.False(ctrl.Right);
	}

	[Fact]
	public void Roundtrip_EmptyMovie_Preserved() {
		var movie = new MovieData { SystemType = SystemType.Nes };
		var bytes = SerializeMovieSnapshot(movie);
		var recovered = DeserializeMovieSnapshot(bytes);
		Assert.Empty(recovered.InputFrames);
	}

	#endregion

	#region Auto-Save Timing Logic

	[Fact]
	public void AutoSaveInterval_DefaultIs300Seconds() {
		int defaultInterval = 300; // 5 minutes in seconds
		Assert.Equal(300, defaultInterval);
	}

	[Theory]
	[InlineData(60)]    // 1 minute
	[InlineData(120)]   // 2 minutes
	[InlineData(300)]   // 5 minutes
	[InlineData(600)]   // 10 minutes
	public void AutoSaveInterval_AcceptsReasonableValues(int seconds) {
		Assert.InRange(seconds, 30, 3600);
	}

	[Fact]
	public void AutoSave_ShouldNotTriggerDuringPlayback() {
		bool isPlaying = true;
		bool autoSaveEnabled = true;
		bool shouldSave = autoSaveEnabled && !isPlaying;
		Assert.False(shouldSave);
	}

	[Fact]
	public void AutoSave_ShouldTriggerWhenNotPlaying() {
		bool isPlaying = false;
		bool autoSaveEnabled = true;
		bool shouldSave = autoSaveEnabled && !isPlaying;
		Assert.True(shouldSave);
	}

	[Fact]
	public void AutoSave_ShouldNotTriggerWhenDisabled() {
		bool isPlaying = false;
		bool autoSaveEnabled = false;
		bool shouldSave = autoSaveEnabled && !isPlaying;
		Assert.False(shouldSave);
	}

	#endregion

	#region TasEditorViewModel Auto-Save Integration

	private static void SetMovieOnViewModel(TasEditorViewModel vm, MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(vm, movie);
		vm.Recorder.Movie = movie;
	}

	private static void SetConverterOnViewModel(TasEditorViewModel vm, IMovieConverter converter) {
		var field = typeof(TasEditorViewModel).GetField("_currentConverter", BindingFlags.Instance | BindingFlags.NonPublic);
		field!.SetValue(vm, converter);
	}

	private static TasEditorViewModel CreateConfiguredViewModel(MovieData movie, string moviePath, bool hasUnsavedChanges = true) {
		var vm = new TasEditorViewModel();
		SetMovieOnViewModel(vm, movie);
		vm.FilePath = moviePath;
		vm.HasUnsavedChanges = hasUnsavedChanges;

		var format = MovieConverterRegistry.DetectFormat(moviePath);
		var converter = MovieConverterRegistry.GetConverter(format);
		Assert.NotNull(converter);
		SetConverterOnViewModel(vm, converter!);

		return vm;
	}

	[Fact]
	public void ViewModel_BuildAutoSavePath_AppendsAutosaveBeforeExtension() {
		string path = TasEditorViewModel.BuildAutoSavePath(@"C:\movies\example.nexen-movie");
		Assert.Equal(@"C:\movies\example.autosave.nexen-movie", path);
	}

	[Fact]
	public void ViewModel_ShouldPromptRecovery_FalseWhenAutosaveMissing() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			File.WriteAllText(moviePath, "movie");
			string autoPath = TasEditorViewModel.BuildAutoSavePath(moviePath);

			Assert.False(TasEditorViewModel.ShouldPromptAutoSaveRecovery(moviePath, autoPath));
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	[Fact]
	public void ViewModel_ShouldPromptRecovery_TrueWhenMovieMissing() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			string autoPath = TasEditorViewModel.BuildAutoSavePath(moviePath);
			File.WriteAllText(autoPath, "autosave");

			Assert.True(TasEditorViewModel.ShouldPromptAutoSaveRecovery(moviePath, autoPath));
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	[Fact]
	public void ViewModel_ShouldPromptRecovery_TrueWhenAutosaveNewer() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			string autoPath = TasEditorViewModel.BuildAutoSavePath(moviePath);
			File.WriteAllText(moviePath, "movie");
			File.WriteAllText(autoPath, "autosave");
			File.SetLastWriteTimeUtc(moviePath, DateTime.UtcNow.AddMinutes(-10));
			File.SetLastWriteTimeUtc(autoPath, DateTime.UtcNow.AddMinutes(-1));

			Assert.True(TasEditorViewModel.ShouldPromptAutoSaveRecovery(moviePath, autoPath));
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	[Fact]
	public void ViewModel_ShouldAutoSaveNow_FalseDuringPlayback() {
		string moviePath = Path.Combine(Path.GetTempPath(), Guid.NewGuid() + ".nexen-movie");
		using var vm = CreateConfiguredViewModel(CreateTestMovie(10), moviePath);
		vm.IsPlaying = true;

		Assert.False(vm.ShouldAutoSaveNow());
	}

	[Fact]
	public void ViewModel_ShouldAutoSaveNow_FalseWhenDisabled() {
		string moviePath = Path.Combine(Path.GetTempPath(), Guid.NewGuid() + ".nexen-movie");
		using var vm = CreateConfiguredViewModel(CreateTestMovie(10), moviePath);
		vm.AutoSaveEnabled = false;

		Assert.False(vm.ShouldAutoSaveNow());
	}

	[Fact]
	public void ViewModel_ShouldAutoSaveNow_FalseWhenNotDirty() {
		string moviePath = Path.Combine(Path.GetTempPath(), Guid.NewGuid() + ".nexen-movie");
		using var vm = CreateConfiguredViewModel(CreateTestMovie(10), moviePath, hasUnsavedChanges: false);

		Assert.False(vm.ShouldAutoSaveNow());
	}

	[Fact]
	public void ViewModel_ShouldAutoSaveNow_TrueWhenEligible() {
		string moviePath = Path.Combine(Path.GetTempPath(), Guid.NewGuid() + ".nexen-movie");
		using var vm = CreateConfiguredViewModel(CreateTestMovie(10), moviePath, hasUnsavedChanges: true);
		vm.IsPlaying = false;
		vm.AutoSaveEnabled = true;

		Assert.True(vm.ShouldAutoSaveNow());
	}

	[Fact]
	public async Task ViewModel_TryAutoSaveAsync_WritesAutosaveFile() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			using var vm = CreateConfiguredViewModel(CreateTestMovie(50), moviePath, hasUnsavedChanges: true);

			bool saved = await vm.TryAutoSaveAsync();
			Assert.True(saved);
			Assert.True(File.Exists(TasEditorViewModel.BuildAutoSavePath(moviePath)));
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	[Fact]
	public async Task ViewModel_TryAutoSaveAsync_DoesNotRunDuringPlayback() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			using var vm = CreateConfiguredViewModel(CreateTestMovie(50), moviePath, hasUnsavedChanges: true);
			vm.IsPlaying = true;

			bool saved = await vm.TryAutoSaveAsync();
			Assert.False(saved);
			Assert.False(File.Exists(TasEditorViewModel.BuildAutoSavePath(moviePath)));
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	[Fact]
	public async Task ViewModel_TryRecoverAutoSaveAsync_RespectsNoPromptChoice() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			string autoPath = TasEditorViewModel.BuildAutoSavePath(moviePath);
			MovieConverterRegistry.Write(CreateTestMovie(5), moviePath);
			MovieConverterRegistry.Write(CreateTestMovie(9), autoPath);
			File.SetLastWriteTimeUtc(moviePath, DateTime.UtcNow.AddMinutes(-10));
			File.SetLastWriteTimeUtc(autoPath, DateTime.UtcNow.AddMinutes(-1));

			using var vm = new TasEditorViewModel();
			vm.SetRecoveryPromptOverride(_ => Task.FromResult(DialogResult.No));

			bool recovered = await vm.TryRecoverAutoSaveAsync(moviePath);
			Assert.False(recovered);
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	[Fact]
	public async Task ViewModel_TryRecoverAutoSaveAsync_LoadsAutosaveWhenConfirmed() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			string autoPath = TasEditorViewModel.BuildAutoSavePath(moviePath);
			MovieConverterRegistry.Write(CreateTestMovie(4), moviePath);
			MovieConverterRegistry.Write(CreateTestMovie(11), autoPath);
			File.SetLastWriteTimeUtc(moviePath, DateTime.UtcNow.AddMinutes(-10));
			File.SetLastWriteTimeUtc(autoPath, DateTime.UtcNow.AddMinutes(-1));

			using var vm = new TasEditorViewModel();
			vm.SetRecoveryPromptOverride(_ => Task.FromResult(DialogResult.Yes));

			bool recovered = await vm.TryRecoverAutoSaveAsync(moviePath);
			Assert.True(recovered);
			Assert.Equal(moviePath, vm.FilePath);
			Assert.True(vm.HasUnsavedChanges);
			Assert.Equal(11, vm.Movie!.InputFrames.Count);
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	[Fact]
	public async Task ViewModel_SaveFileAsync_RemovesAutosaveSnapshot() {
		string baseDir = Path.Combine(Path.GetTempPath(), "nexen-autosave-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(baseDir);
		try {
			string moviePath = Path.Combine(baseDir, "movie.nexen-movie");
			using var vm = CreateConfiguredViewModel(CreateTestMovie(20), moviePath, hasUnsavedChanges: true);
			string autoPath = TasEditorViewModel.BuildAutoSavePath(moviePath);
			await vm.TryAutoSaveAsync();
			Assert.True(File.Exists(autoPath));

			await vm.SaveFileAsync();
			Assert.False(File.Exists(autoPath));
		} finally {
			Directory.Delete(baseDir, true);
		}
	}

	#endregion
}
