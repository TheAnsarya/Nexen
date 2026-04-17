using System.Reflection;
using Nexen.MovieConverter;
using Nexen.TAS;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Comprehensive tests for <see cref="TasLuaApi"/>.
/// Covers movie info, input manipulation, greenzone, recording,
/// branches, search, and utility functions.
/// </summary>
public class TasLuaApiComprehensiveTests : IDisposable {
	private readonly TasEditorViewModel _vm;
	private readonly TasLuaApi _api;

	public TasLuaApiComprehensiveTests() {
		_vm = new TasEditorViewModel();
		_api = _vm.LuaApi;
	}

	public void Dispose() {
		_vm.Dispose();
	}

	#region Helpers

	private static MovieData CreateTestMovie(int frameCount, SystemType system = SystemType.Nes) {
		var movie = new MovieData {
			Author = "TasLuaTest",
			GameName = "Test Game",
			SystemType = system,
			Region = RegionType.NTSC,
			Description = "Test movie for Lua API"
		};

		for (int i = 0; i < frameCount; i++) {
			movie.InputFrames.Add(new InputFrame(i) {
				Controllers = [new ControllerInput()]
			});
		}

		return movie;
	}

	private void SetMovie(MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(_vm, movie);
		// Also set on Recorder directly since reactive subscription may not fire in test context
		_vm.Recorder.Movie = movie;
	}

	#endregion

	#region Movie Information

	[Fact]
	public void GetCurrentFrame_ReturnsPlaybackFrame() {
		SetMovie(CreateTestMovie(20));
		Assert.Equal(_vm.PlaybackFrame, _api.GetCurrentFrame());
	}

	[Fact]
	public void GetTotalFrames_ReturnsFrameCount() {
		SetMovie(CreateTestMovie(42));
		Assert.Equal(42, _api.GetTotalFrames());
	}

	[Fact]
	public void GetTotalFrames_NoMovie_ReturnsZero() {
		Assert.Equal(0, _api.GetTotalFrames());
	}

	[Fact]
	public void GetRerecordCount_ReturnsViewModelValue() {
		SetMovie(CreateTestMovie(10));
		Assert.Equal(_vm.RerecordCount, _api.GetRerecordCount());
	}

	[Fact]
	public void IsModified_Track_sUnsavedChanges() {
		SetMovie(CreateTestMovie(10));
		// Fresh movie should not be modified
		Assert.False(_api.IsModified());

		// After an action, it should be modified
		_api.InsertFrames(0, 1);
		Assert.True(_api.IsModified());
	}

	[Fact]
	public void GetSystemType_ReturnsCorrectType() {
		SetMovie(CreateTestMovie(5, SystemType.Snes));
		Assert.Equal("Snes", _api.GetSystemType());
	}

	[Fact]
	public void GetSystemType_NoMovie_ReturnsUnknown() {
		Assert.Equal("Unknown", _api.GetSystemType());
	}

	[Fact]
	public void GetAuthor_ReturnsAuthorName() {
		SetMovie(CreateTestMovie(5));
		Assert.Equal("TasLuaTest", _api.GetAuthor());
	}

	[Fact]
	public void GetAuthor_NoMovie_ReturnsEmpty() {
		Assert.Equal("", _api.GetAuthor());
	}

	[Fact]
	public void GetMovieInfo_ReturnsAllFields() {
		var movie = CreateTestMovie(15, SystemType.Nes);
		movie.RerecordCount = 100;
		movie.RomFileName = "test.nes";
		movie.Sha1Hash = "abc123";
		SetMovie(movie);

		var info = _api.GetMovieInfo();
		Assert.Equal(15, info["frameCount"]);
		Assert.Equal(100UL, info["rerecordCount"]);
		Assert.Equal("TasLuaTest", info["author"]);
		Assert.Equal("Nes", info["systemType"]);
		Assert.Equal("test.nes", info["romName"]);
		Assert.Equal("abc123", info["romChecksum"]);
	}

	[Fact]
	public void GetMovieInfo_NoMovie_ReturnsEmptyDictionary() {
		var info = _api.GetMovieInfo();
		Assert.Empty(info);
	}

	#endregion

	#region Input Manipulation — GetFrameInput

	[Fact]
	public void GetFrameInput_ValidFrame_ReturnsDictionary() {
		var movie = CreateTestMovie(10);
		movie.InputFrames[3].Controllers[0].A = true;
		movie.InputFrames[3].Controllers[0].Up = true;
		SetMovie(movie);

		var input = _api.GetFrameInput(3);
		Assert.NotNull(input);
		Assert.True(input["a"]);
		Assert.True(input["up"]);
		Assert.False(input["b"]);
		Assert.False(input["down"]);
	}

	[Fact]
	public void GetFrameInput_AllButtons_ReturnsAll12Keys() {
		SetMovie(CreateTestMovie(5));
		var input = _api.GetFrameInput(0);
		Assert.NotNull(input);
		Assert.Equal(12, input.Count);
		Assert.Contains("a", input.Keys);
		Assert.Contains("b", input.Keys);
		Assert.Contains("x", input.Keys);
		Assert.Contains("y", input.Keys);
		Assert.Contains("l", input.Keys);
		Assert.Contains("r", input.Keys);
		Assert.Contains("select", input.Keys);
		Assert.Contains("start", input.Keys);
		Assert.Contains("up", input.Keys);
		Assert.Contains("down", input.Keys);
		Assert.Contains("left", input.Keys);
		Assert.Contains("right", input.Keys);
	}

	[Fact]
	public void GetFrameInput_NegativeFrame_ReturnsNull() {
		SetMovie(CreateTestMovie(10));
		Assert.Null(_api.GetFrameInput(-1));
	}

	[Fact]
	public void GetFrameInput_OutOfRangeFrame_ReturnsNull() {
		SetMovie(CreateTestMovie(10));
		Assert.Null(_api.GetFrameInput(10));
		Assert.Null(_api.GetFrameInput(100));
	}

	[Fact]
	public void GetFrameInput_NoMovie_ReturnsNull() {
		Assert.Null(_api.GetFrameInput(0));
	}

	[Fact]
	public void GetFrameInput_InvalidController_ReturnsNull() {
		SetMovie(CreateTestMovie(10));
		Assert.Null(_api.GetFrameInput(0, controller: 5));
	}

	#endregion

	#region Input Manipulation — SetFrameInput

	[Fact]
	public void SetFrameInput_SetsButtonStates() {
		SetMovie(CreateTestMovie(10));

		var input = new Dictionary<string, bool> {
			["a"] = true,
			["b"] = true,
			["start"] = true
		};
		bool result = _api.SetFrameInput(0, input);

		Assert.True(result);
		var readBack = _api.GetFrameInput(0);
		Assert.NotNull(readBack);
		Assert.True(readBack["a"]);
		Assert.True(readBack["b"]);
		Assert.True(readBack["start"]);
		Assert.False(readBack["up"]);
	}

	[Fact]
	public void SetFrameInput_PartialInput_OnlySetsSpecifiedButtons() {
		var movie = CreateTestMovie(10);
		movie.InputFrames[0].Controllers[0].Up = true;
		SetMovie(movie);

		// Set only A, should leave Up unchanged
		var input = new Dictionary<string, bool> { ["a"] = true };
		_api.SetFrameInput(0, input);

		var readBack = _api.GetFrameInput(0);
		Assert.NotNull(readBack);
		Assert.True(readBack["a"]);
		Assert.True(readBack["up"]); // Preserved
	}

	[Fact]
	public void SetFrameInput_InvalidFrame_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.SetFrameInput(-1, new Dictionary<string, bool>()));
		Assert.False(_api.SetFrameInput(10, new Dictionary<string, bool>()));
	}

	[Fact]
	public void SetFrameInput_NoMovie_ReturnsFalse() {
		Assert.False(_api.SetFrameInput(0, new Dictionary<string, bool>()));
	}

	[Fact]
	public void SetFrameInput_InvalidController_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.SetFrameInput(0, new Dictionary<string, bool>(), controller: 5));
	}

	[Fact]
	public void SetFrameInput_IsUndoable() {
		SetMovie(CreateTestMovie(10));

		var input = new Dictionary<string, bool> { ["a"] = true };
		_api.SetFrameInput(0, input);

		// Should be undoable now
		Assert.True(_vm.CanUndo);

		_vm.Undo();

		var readBack = _api.GetFrameInput(0);
		Assert.NotNull(readBack);
		Assert.False(readBack["a"]); // Reverted
	}

	#endregion

	#region Input Manipulation — ClearFrameInput

	[Fact]
	public void ClearFrameInput_ClearsAllButtons() {
		var movie = CreateTestMovie(10);
		movie.InputFrames[3].Controllers[0].A = true;
		movie.InputFrames[3].Controllers[0].B = true;
		movie.InputFrames[3].Controllers[0].Up = true;
		SetMovie(movie);

		bool result = _api.ClearFrameInput(3);
		Assert.True(result);

		var readBack = _api.GetFrameInput(3);
		Assert.NotNull(readBack);
		Assert.False(readBack["a"]);
		Assert.False(readBack["b"]);
		Assert.False(readBack["up"]);
	}

	[Fact]
	public void ClearFrameInput_InvalidFrame_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.ClearFrameInput(-1));
		Assert.False(_api.ClearFrameInput(10));
	}

	[Fact]
	public void ClearFrameInput_NoMovie_ReturnsFalse() {
		Assert.False(_api.ClearFrameInput(0));
	}

	[Fact]
	public void ClearFrameInput_IsUndoable() {
		var movie = CreateTestMovie(10);
		movie.InputFrames[0].Controllers[0].A = true;
		SetMovie(movie);

		_api.ClearFrameInput(0);
		Assert.True(_vm.CanUndo);

		_vm.Undo();

		var readBack = _api.GetFrameInput(0);
		Assert.NotNull(readBack);
		Assert.True(readBack["a"]); // Restored
	}

	#endregion

	#region InsertFrames / DeleteFrames

	[Fact]
	public void InsertFrames_ValidPosition_IncreasesCount() {
		SetMovie(CreateTestMovie(10));
		bool result = _api.InsertFrames(5, 3);

		Assert.True(result);
		Assert.Equal(13, _api.GetTotalFrames());
	}

	[Fact]
	public void InsertFrames_AtEnd_AppendsFrames() {
		SetMovie(CreateTestMovie(10));
		bool result = _api.InsertFrames(10, 5);

		Assert.True(result);
		Assert.Equal(15, _api.GetTotalFrames());
	}

	[Fact]
	public void InsertFrames_AtBeginning_PrependFrames() {
		SetMovie(CreateTestMovie(10));
		bool result = _api.InsertFrames(0, 2);

		Assert.True(result);
		Assert.Equal(12, _api.GetTotalFrames());
	}

	[Fact]
	public void InsertFrames_ZeroCount_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.InsertFrames(0, 0));
	}

	[Fact]
	public void InsertFrames_NegativeCount_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.InsertFrames(0, -1));
	}

	[Fact]
	public void InsertFrames_NegativePosition_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.InsertFrames(-1, 5));
	}

	[Fact]
	public void InsertFrames_NoMovie_ReturnsFalse() {
		Assert.False(_api.InsertFrames(0, 1));
	}

	[Fact]
	public void InsertFrames_IsUndoable() {
		SetMovie(CreateTestMovie(10));
		_api.InsertFrames(5, 3);

		Assert.True(_vm.CanUndo);

		_vm.Undo();
		Assert.Equal(10, _api.GetTotalFrames());
	}

	[Fact]
	public void InsertFrames_PositionBeyondEnd_ClampsToEnd() {
		SetMovie(CreateTestMovie(10));
		bool result = _api.InsertFrames(100, 3);

		Assert.True(result);
		Assert.Equal(13, _api.GetTotalFrames());
	}

	[Fact]
	public void DeleteFrames_ValidRange_DecreasesCount() {
		SetMovie(CreateTestMovie(20));
		bool result = _api.DeleteFrames(5, 5);

		Assert.True(result);
		Assert.Equal(15, _api.GetTotalFrames());
	}

	[Fact]
	public void DeleteFrames_AtEnd_RemovesLast() {
		SetMovie(CreateTestMovie(10));
		bool result = _api.DeleteFrames(7, 3);

		Assert.True(result);
		Assert.Equal(7, _api.GetTotalFrames());
	}

	[Fact]
	public void DeleteFrames_ExceedsAvailable_ClampsCount() {
		SetMovie(CreateTestMovie(10));
		bool result = _api.DeleteFrames(5, 100);

		Assert.True(result);
		Assert.Equal(5, _api.GetTotalFrames());
	}

	[Fact]
	public void DeleteFrames_ZeroCount_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.DeleteFrames(0, 0));
	}

	[Fact]
	public void DeleteFrames_NegativeCount_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.DeleteFrames(0, -1));
	}

	[Fact]
	public void DeleteFrames_NegativePosition_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.DeleteFrames(-1, 5));
	}

	[Fact]
	public void DeleteFrames_PositionOutOfRange_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.DeleteFrames(10, 1));
	}

	[Fact]
	public void DeleteFrames_NoMovie_ReturnsFalse() {
		Assert.False(_api.DeleteFrames(0, 1));
	}

	[Fact]
	public void DeleteFrames_IsUndoable() {
		SetMovie(CreateTestMovie(20));
		_api.DeleteFrames(5, 5);

		Assert.True(_vm.CanUndo);

		_vm.Undo();
		Assert.Equal(20, _api.GetTotalFrames());
	}

	#endregion

	#region Greenzone Operations

	[Fact]
	public void GetGreenzoneInfo_ReturnsValidDictionary() {
		SetMovie(CreateTestMovie(10));
		var info = _api.GetGreenzoneInfo();

		Assert.Contains("savestateCount", info.Keys);
		Assert.Contains("memoryUsageMB", info.Keys);
		Assert.Contains("captureInterval", info.Keys);
		Assert.Contains("maxSavestates", info.Keys);
		Assert.Contains("compressionEnabled", info.Keys);
	}

	[Fact]
	public void HasSavestate_DefaultFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.HasSavestate(0));
		Assert.False(_api.HasSavestate(5));
	}

	[Fact]
	public void ClearGreenzone_DoesNotThrow() {
		SetMovie(CreateTestMovie(10));
		_api.ClearGreenzone();
		Assert.False(_api.HasSavestate(0));
	}

	#endregion

	#region Recording Operations (require native interop — test guards only)

	[Fact]
	public void IsRecording_NoMovie_DefaultFalse() {
		// Recorder exists even without movie
		Assert.False(_api.IsRecording());
	}

	// StartRecording/StopRecording/RecordCycle tests require NexenCore.dll
	// and cannot run in unit test context without the emulator.

	#endregion

	#region Branch Operations

	[Fact]
	public void GetBranches_EmptyByDefault() {
		SetMovie(CreateTestMovie(10));
		var branches = _api.GetBranches();
		Assert.Empty(branches);
	}

	[Fact]
	public void CreateBranch_ReturnsName() {
		SetMovie(CreateTestMovie(10));
		string name = _api.CreateBranch("test-branch");
		Assert.False(string.IsNullOrEmpty(name));
	}

	[Fact]
	public void CreateBranch_AppearsInGetBranches() {
		SetMovie(CreateTestMovie(10));
		_api.CreateBranch("br1");
		var branches = _api.GetBranches();
		Assert.Contains("br1", branches);
	}

	[Fact]
	public void CreateBranch_MultipleBranches_AllAppear() {
		SetMovie(CreateTestMovie(10));
		_api.CreateBranch("alpha");
		_api.CreateBranch("beta");
		_api.CreateBranch("gamma");

		var branches = _api.GetBranches();
		Assert.Contains("alpha", branches);
		Assert.Contains("beta", branches);
		Assert.Contains("gamma", branches);
	}

	[Fact]
	public void LoadBranch_ValidName_ReturnsTrue() {
		SetMovie(CreateTestMovie(10));
		_api.CreateBranch("load-me");
		bool result = _api.LoadBranch("load-me");
		Assert.True(result);
	}

	[Fact]
	public void LoadBranch_InvalidName_ReturnsFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.LoadBranch("nonexistent"));
	}

	[Fact]
	public void GetBranchInfo_ValidName_ReturnsDictionary() {
		SetMovie(CreateTestMovie(10));
		_api.CreateBranch("info-branch");

		var info = _api.GetBranchInfo("info-branch");
		Assert.NotNull(info);
		Assert.Equal("info-branch", info["name"]);
		Assert.Contains("createdAt", info.Keys);
		Assert.Contains("frameCount", info.Keys);
	}

	[Fact]
	public void GetBranchInfo_InvalidName_ReturnsNull() {
		SetMovie(CreateTestMovie(10));
		Assert.Null(_api.GetBranchInfo("nonexistent"));
	}

	[Fact]
	public void LoadBranch_RestoresMovieState() {
		SetMovie(CreateTestMovie(10));

		// Modify input
		var input = new Dictionary<string, bool> { ["a"] = true };
		_api.SetFrameInput(0, input);

		// Create branch with modified state
		_api.CreateBranch("modified");

		// Clear input
		_api.ClearFrameInput(0);
		var cleared = _api.GetFrameInput(0);
		Assert.NotNull(cleared);
		Assert.False(cleared["a"]);

		// Load branch — should restore
		_api.LoadBranch("modified");

		var restored = _api.GetFrameInput(0);
		Assert.NotNull(restored);
		Assert.True(restored["a"]);
	}

	#endregion

	#region Input Search

	[Fact]
	public void IsSearching_DefaultFalse() {
		SetMovie(CreateTestMovie(10));
		Assert.False(_api.IsSearching());
	}

	[Fact]
	public void BeginSearch_SetsIsSearching() {
		SetMovie(CreateTestMovie(10));
		_api.BeginSearch();
		Assert.True(_api.IsSearching());
	}

	[Fact]
	public void FinishSearch_ClearsIsSearching() {
		SetMovie(CreateTestMovie(10));
		_api.BeginSearch();
		_api.FinishSearch(loadBest: false);
		Assert.False(_api.IsSearching());
	}

	[Fact]
	public void SetSearchState_GetSearchState_Roundtrip() {
		SetMovie(CreateTestMovie(10));
		_api.BeginSearch();

		_api.SetSearchState("bestScore", 42);
		_api.SetSearchState("direction", "left");

		Assert.Equal(42, _api.GetSearchState("bestScore"));
		Assert.Equal("left", _api.GetSearchState("direction"));
	}

	[Fact]
	public void GetSearchState_MissingKey_ReturnsNull() {
		SetMovie(CreateTestMovie(10));
		Assert.Null(_api.GetSearchState("nonexistent"));
	}

	[Fact]
	public void BeginSearch_ClearsSearchState() {
		SetMovie(CreateTestMovie(10));
		_api.BeginSearch();
		_api.SetSearchState("key", "value");

		// Begin new search should clear state
		_api.FinishSearch(loadBest: false);
		_api.BeginSearch();

		Assert.Null(_api.GetSearchState("key"));
	}

	[Fact]
	public void BeginSearch_CreatesBranch() {
		SetMovie(CreateTestMovie(10));
		int branchCountBefore = _api.GetBranches().Count;

		_api.BeginSearch();

		// Should have created a Search_ branch
		var branches = _api.GetBranches();
		Assert.True(branches.Count > branchCountBefore);
		Assert.Contains(branches, b => b.StartsWith("Search_"));
	}

	[Fact]
	public void MarkBestResult_CreatesBestBranch() {
		SetMovie(CreateTestMovie(10));
		_api.BeginSearch();
		_api.MarkBestResult();

		var branches = _api.GetBranches();
		Assert.Contains(branches, b => b.StartsWith("Best_"));
	}

	#endregion

	#region Utility Functions

	[Fact]
	public void GenerateInputCombinations_Default_Returns64Combos() {
		// Default: a, b, up, down, left, right = 6 buttons = 2^6 = 64
		var combos = _api.GenerateInputCombinations();
		Assert.Equal(64, combos.Count);
	}

	[Fact]
	public void GenerateInputCombinations_TwoButtons_Returns4Combos() {
		var combos = _api.GenerateInputCombinations(["a", "b"]);
		Assert.Equal(4, combos.Count);

		// Should contain: {a:F,b:F}, {a:T,b:F}, {a:F,b:T}, {a:T,b:T}
		Assert.Contains(combos, c => !c["a"] && !c["b"]);
		Assert.Contains(combos, c => c["a"] && !c["b"]);
		Assert.Contains(combos, c => !c["a"] && c["b"]);
		Assert.Contains(combos, c => c["a"] && c["b"]);
	}

	[Fact]
	public void GenerateInputCombinations_SingleButton_Returns2Combos() {
		var combos = _api.GenerateInputCombinations(["a"]);
		Assert.Equal(2, combos.Count);
	}

	[Fact]
	public void GenerateInputCombinations_NullButtons_UsesDefault() {
		var combos = _api.GenerateInputCombinations(null);
		Assert.Equal(64, combos.Count);
	}

	[Fact]
	public void GenerateInputCombinations_AllCombosHaveCorrectKeys() {
		var buttons = new List<string> { "x", "y", "l" };
		var combos = _api.GenerateInputCombinations(buttons);

		Assert.Equal(8, combos.Count);
		foreach (var combo in combos) {
			Assert.Equal(3, combo.Count);
			Assert.Contains("x", combo.Keys);
			Assert.Contains("y", combo.Keys);
			Assert.Contains("l", combo.Keys);
		}
	}

	[Fact]
	public void GenerateInputCombinations_TooManyButtons_Throws() {
		var buttons = Enumerable.Range(0, 17).Select(i => $"btn{i}").ToList();
		Assert.Throws<ArgumentException>(() => _api.GenerateInputCombinations(buttons));
	}

	[Fact]
	public void GenerateInputCombinations_16Buttons_Succeeds() {
		var buttons = Enumerable.Range(0, 16).Select(i => $"btn{i}").ToList();
		var combos = _api.GenerateInputCombinations(buttons);
		Assert.Equal(65536, combos.Count);
	}

	#endregion

	#region Combined Workflows

	[Fact]
	public void InsertThenDelete_RestoresOriginalCount() {
		SetMovie(CreateTestMovie(10));

		_api.InsertFrames(5, 5);
		Assert.Equal(15, _api.GetTotalFrames());

		_api.DeleteFrames(5, 5);
		Assert.Equal(10, _api.GetTotalFrames());
	}

	[Fact]
	public void SetInput_InsertDeleteFrames_PreservesOtherFrames() {
		SetMovie(CreateTestMovie(10));

		// Set input on frame 0
		_api.SetFrameInput(0, new Dictionary<string, bool> { ["a"] = true });

		// Insert frames before it
		_api.InsertFrames(0, 2);

		// Frame 0's input should now be at frame 2
		var readBack = _api.GetFrameInput(2);
		Assert.NotNull(readBack);
		Assert.True(readBack["a"]);
	}

	[Fact]
	public void UndoRedo_MultipleOperations() {
		SetMovie(CreateTestMovie(10));

		// Insert
		_api.InsertFrames(5, 3);
		Assert.Equal(13, _api.GetTotalFrames());

		// Set input
		_api.SetFrameInput(5, new Dictionary<string, bool> { ["b"] = true });

		// Undo set input
		_vm.Undo();
		var readBack = _api.GetFrameInput(5);
		Assert.NotNull(readBack);
		Assert.False(readBack["b"]);

		// Undo insert
		_vm.Undo();
		Assert.Equal(10, _api.GetTotalFrames());

		// Redo both
		_vm.Redo();
		Assert.Equal(13, _api.GetTotalFrames());
	}

	[Fact]
	public void BranchWorkflow_CreateModifyLoadRestore() {
		SetMovie(CreateTestMovie(10));

		// Create initial branch
		_api.CreateBranch("initial");

		// Modify frame
		_api.SetFrameInput(0, new Dictionary<string, bool> { ["start"] = true });

		// Create modified branch
		_api.CreateBranch("modified");

		// Load initial — should restore
		_api.LoadBranch("initial");
		var readBack = _api.GetFrameInput(0);
		Assert.NotNull(readBack);
		Assert.False(readBack["start"]);

		// Load modified — should restore changes
		_api.LoadBranch("modified");
		readBack = _api.GetFrameInput(0);
		Assert.NotNull(readBack);
		Assert.True(readBack["start"]);
	}

	#endregion
}
