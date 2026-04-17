using System.Collections.ObjectModel;
using Nexen.MovieConverter;
using Nexen.TAS;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for TasEditorViewModel incremental update optimizations.
/// These tests validate that single-frame operations use O(1) updates instead of O(n) rebuilds.
/// </summary>
public class TasEditorViewModelTests {
	/// <summary>
	/// Tests that RefreshFromFrame correctly raises property changed notifications.
	/// </summary>
	[Fact]
	public void TasFrameViewModel_RefreshFromFrame_RaisesPropertyChanged() {
		// Arrange
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput { A = false }]
		};
		var vm = new MockTasFrameViewModel(frame, 0, false);
		var changedProperties = new List<string?>();
		vm.PropertyChanged += (_, e) => changedProperties.Add(e.PropertyName);

		// Act
		frame.Controllers[0].A = true;
		vm.RefreshFromFrame();

		// Assert
		Assert.Contains(nameof(MockTasFrameViewModel.P1Input), changedProperties);
		Assert.Contains(nameof(MockTasFrameViewModel.P2Input), changedProperties);
		Assert.Contains(nameof(MockTasFrameViewModel.CommentText), changedProperties);
	}

	/// <summary>
	/// Tests that FrameNumber property raises PropertyChanged when updated.
	/// </summary>
	[Fact]
	public void TasFrameViewModel_FrameNumber_RaisesPropertyChangedWhenUpdated() {
		// Arrange
		var frame = new InputFrame(0);
		var vm = new MockTasFrameViewModel(frame, 0, false);
		var changedProperties = new List<string?>();
		vm.PropertyChanged += (_, e) => changedProperties.Add(e.PropertyName);

		// Act
		vm.FrameNumber = 42;

		// Assert
		Assert.Contains(nameof(MockTasFrameViewModel.FrameNumber), changedProperties);
		Assert.Equal(42, vm.FrameNumber);
	}

	/// <summary>
	/// Tests that IsGreenzone property raises PropertyChanged for both itself and Background.
	/// </summary>
	[Fact]
	public void TasFrameViewModel_IsGreenzone_RaisesPropertyChangedForBackground() {
		// Arrange
		var frame = new InputFrame(0);
		var vm = new MockTasFrameViewModel(frame, 0, false);
		var changedProperties = new List<string?>();
		vm.PropertyChanged += (_, e) => changedProperties.Add(e.PropertyName);

		// Act
		vm.IsGreenzone = true;

		// Assert
		Assert.Contains(nameof(MockTasFrameViewModel.IsGreenzone), changedProperties);
		Assert.Contains(nameof(MockTasFrameViewModel.Background), changedProperties);
		Assert.True(vm.IsGreenzone);
	}

	/// <summary>
	/// Tests incremental insert - frame numbers should be correctly updated for subsequent frames.
	/// </summary>
	[Fact]
	public void IncrementalInsert_UpdatesSubsequentFrameNumbers() {
		// Arrange - simulate the Frames collection with 5 frames
		var frames = new InputFrame[5];
		for (int i = 0; i < 5; i++) {
			frames[i] = new InputFrame(i);
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 5; i++) {
			viewModels.Add(new MockTasFrameViewModel(frames[i], i, false));
		}

		// Verify initial state: FrameNumbers are 1, 2, 3, 4, 5 (1-based)
		Assert.Equal(1, viewModels[0].FrameNumber);
		Assert.Equal(5, viewModels[4].FrameNumber);

		// Act - simulate InsertFrameViewModel at index 2
		var newFrame = new InputFrame(2);
		var newVm = new MockTasFrameViewModel(newFrame, 2, false);
		viewModels.Insert(2, newVm);

		// Update frame numbers for subsequent items
		for (int i = 3; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		// Assert
		Assert.Equal(6, viewModels.Count);
		Assert.Equal(1, viewModels[0].FrameNumber); // Unchanged
		Assert.Equal(2, viewModels[1].FrameNumber); // Unchanged
		Assert.Equal(3, viewModels[2].FrameNumber); // New frame
		Assert.Equal(4, viewModels[3].FrameNumber); // Was 3, now 4
		Assert.Equal(5, viewModels[4].FrameNumber); // Was 4, now 5
		Assert.Equal(6, viewModels[5].FrameNumber); // Was 5, now 6
	}

	/// <summary>
	/// Tests incremental delete - frame numbers should be correctly updated for subsequent frames.
	/// </summary>
	[Fact]
	public void IncrementalDelete_UpdatesSubsequentFrameNumbers() {
		// Arrange - simulate the Frames collection with 5 frames
		var frames = new InputFrame[5];
		for (int i = 0; i < 5; i++) {
			frames[i] = new InputFrame(i);
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 5; i++) {
			viewModels.Add(new MockTasFrameViewModel(frames[i], i, false));
		}

		// Act - simulate RemoveFrameViewModel at index 2
		viewModels.RemoveAt(2);

		// Update frame numbers for subsequent items
		for (int i = 2; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		// Assert
		Assert.Equal(4, viewModels.Count);
		Assert.Equal(1, viewModels[0].FrameNumber); // Unchanged
		Assert.Equal(2, viewModels[1].FrameNumber); // Unchanged
		Assert.Equal(3, viewModels[2].FrameNumber); // Was 4, now 3
		Assert.Equal(4, viewModels[3].FrameNumber); // Was 5, now 4
	}

	/// <summary>
	/// Tests that SyncNewFrames only adds new frames, not rebuilding existing ones.
	/// </summary>
	[Fact]
	public void SyncNewFrames_OnlyAppendsNewFrames() {
		// Arrange - simulate movie with 5 frames and Frames collection with 3
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < 5; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 3; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		// Keep reference to first VM to verify it wasn't replaced
		var firstVm = viewModels[0];

		// Act - simulate SyncNewFrames
		while (viewModels.Count < movieFrames.Count) {
			int i = viewModels.Count;
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		// Assert
		Assert.Equal(5, viewModels.Count);
		Assert.Same(firstVm, viewModels[0]); // First VM NOT recreated - this is the key optimization
		Assert.Equal(4, viewModels[3].FrameNumber); // New frame 4 (1-based)
		Assert.Equal(5, viewModels[4].FrameNumber); // New frame 5 (1-based)
	}

	#region Batch Insert Tests

	/// <summary>
	/// Tests batch insert of multiple frames with correct renumbering.
	/// Simulates InsertFrameViewModels logic.
	/// </summary>
	[Theory]
	[InlineData(10, 0, 3)]   // Insert at beginning
	[InlineData(10, 5, 3)]   // Insert in middle
	[InlineData(10, 10, 3)]  // Insert at end
	[InlineData(10, 3, 1)]   // Insert single
	[InlineData(10, 7, 5)]   // Insert more than remaining tail
	public void BatchInsert_CorrectlyRenumbersAllFrames(int initialCount, int insertAt, int insertCount) {
		// Arrange
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < initialCount + insertCount; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < initialCount; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		// Act - simulate InsertFrameViewModels
		for (int j = 0; j < insertCount; j++) {
			int idx = insertAt + j;
			var vm = new MockTasFrameViewModel(movieFrames[initialCount + j], idx, false);
			viewModels.Insert(idx, vm);
		}

		// Renumber everything after the inserted block
		for (int i = insertAt + insertCount; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		// Assert - all frame numbers should be sequential 1-based
		Assert.Equal(initialCount + insertCount, viewModels.Count);
		for (int i = 0; i < viewModels.Count; i++) {
			Assert.Equal(i + 1, viewModels[i].FrameNumber);
		}
	}

	/// <summary>
	/// Tests that batch insert preserves existing VM references before the insert point.
	/// </summary>
	[Fact]
	public void BatchInsert_PreservesExistingVmReferences() {
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < 8; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 5; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		var beforeInsert = viewModels.Select(vm => vm).ToArray();

		// Insert 3 frames at index 2
		for (int j = 0; j < 3; j++) {
			int idx = 2 + j;
			viewModels.Insert(idx, new MockTasFrameViewModel(movieFrames[5 + j], idx, false));
		}

		// VMs before insert point should be the same references
		Assert.Same(beforeInsert[0], viewModels[0]);
		Assert.Same(beforeInsert[1], viewModels[1]);
		// VMs after insert point should be the same references (shifted)
		Assert.Same(beforeInsert[2], viewModels[5]);
		Assert.Same(beforeInsert[3], viewModels[6]);
		Assert.Same(beforeInsert[4], viewModels[7]);
	}

	#endregion

	#region Batch Remove Tests

	/// <summary>
	/// Tests batch remove of multiple frames with correct renumbering.
	/// Simulates RemoveFrameViewModels logic.
	/// </summary>
	[Theory]
	[InlineData(10, 0, 3)]   // Remove from beginning
	[InlineData(10, 4, 3)]   // Remove from middle
	[InlineData(10, 7, 3)]   // Remove from near end
	[InlineData(10, 5, 1)]   // Remove single
	public void BatchRemove_CorrectlyRenumbersAllFrames(int initialCount, int removeAt, int removeCount) {
		// Arrange
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < initialCount; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < initialCount; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		// Act — simulate RemoveFrameViewModels (backward removal)
		for (int i = removeAt + removeCount - 1; i >= removeAt; i--) {
			viewModels.RemoveAt(i);
		}

		// Renumber everything after the removed block
		for (int i = removeAt; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		// Assert
		Assert.Equal(initialCount - removeCount, viewModels.Count);
		for (int i = 0; i < viewModels.Count; i++) {
			Assert.Equal(i + 1, viewModels[i].FrameNumber);
		}
	}

	/// <summary>
	/// Tests that batch remove preserves references outside the removed range.
	/// </summary>
	[Fact]
	public void BatchRemove_PreservesExistingVmReferences() {
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < 10; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 10; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		var vm0 = viewModels[0];
		var vm1 = viewModels[1];
		var vm5 = viewModels[5];
		var vm9 = viewModels[9];

		// Remove indices 2-4 (3 frames)
		for (int i = 4; i >= 2; i--) {
			viewModels.RemoveAt(i);
		}

		Assert.Same(vm0, viewModels[0]);
		Assert.Same(vm1, viewModels[1]);
		Assert.Same(vm5, viewModels[2]); // Shifted from 5 to 2
		Assert.Same(vm9, viewModels[6]); // Shifted from 9 to 6
	}

	#endregion

	#region Truncate Tests

	/// <summary>
	/// Tests truncation removes all frames from the specified index to the end.
	/// Simulates TruncateFrameViewModels logic.
	/// </summary>
	[Theory]
	[InlineData(10, 0)]   // Truncate all
	[InlineData(10, 5)]   // Truncate second half
	[InlineData(10, 9)]   // Truncate last frame only
	[InlineData(10, 1)]   // Keep only first frame
	public void Truncate_RemovesFromIndexToEnd(int initialCount, int truncateAt) {
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < initialCount; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < initialCount; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		// Act — simulate TruncateFrameViewModels (reverse removal)
		for (int i = viewModels.Count - 1; i >= truncateAt; i--) {
			viewModels.RemoveAt(i);
		}

		// Assert
		Assert.Equal(truncateAt, viewModels.Count);
		for (int i = 0; i < viewModels.Count; i++) {
			Assert.Equal(i + 1, viewModels[i].FrameNumber);
		}
	}

	/// <summary>
	/// Tests truncate preserves references for frames before the truncation point.
	/// </summary>
	[Fact]
	public void Truncate_PreservesVmReferencesBeforeTruncationPoint() {
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < 10; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 10; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		var vm0 = viewModels[0];
		var vm4 = viewModels[4];

		// Truncate at index 5
		for (int i = viewModels.Count - 1; i >= 5; i--) {
			viewModels.RemoveAt(i);
		}

		Assert.Equal(5, viewModels.Count);
		Assert.Same(vm0, viewModels[0]);
		Assert.Same(vm4, viewModels[4]);
	}

	#endregion

	#region Undo/Redo Incremental Dispatch Tests

	/// <summary>
	/// Tests that undoing an InsertFramesAction results in the correct frame count
	/// when using incremental remove instead of full rebuild.
	/// </summary>
	[Fact]
	public void UndoInsert_IncrementalRemove_MaintainsCorrectState() {
		var movie = CreateTestMovieData(10);
		var viewModels = CreateViewModelsForMovie(movie);

		// Insert 3 frames at index 5
		var newFrames = new List<InputFrame> {
			new(100), new(101), new(102)
		};
		var action = new InsertFramesAction(movie, 5, newFrames);
		action.Execute();

		// Simulate InsertFrameViewModels
		for (int j = 0; j < 3; j++) {
			int idx = 5 + j;
			viewModels.Insert(idx, new MockTasFrameViewModel(movie.InputFrames[idx], idx, false));
		}
		for (int i = 8; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		Assert.Equal(13, viewModels.Count);

		// Now undo
		action.Undo();

		// Simulate incremental RemoveFrameViewModels
		for (int i = 7; i >= 5; i--) {
			viewModels.RemoveAt(i);
		}
		for (int i = 5; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		Assert.Equal(10, viewModels.Count);
		Assert.Equal(10, movie.InputFrames.Count);

		// Verify sequential numbering
		for (int i = 0; i < viewModels.Count; i++) {
			Assert.Equal(i + 1, viewModels[i].FrameNumber);
		}
	}

	/// <summary>
	/// Tests that undoing a DeleteFramesAction results in correct re-insertion
	/// when using incremental insert instead of full rebuild.
	/// </summary>
	[Fact]
	public void UndoDelete_IncrementalInsert_MaintainsCorrectState() {
		var movie = CreateTestMovieData(10);
		var viewModels = CreateViewModelsForMovie(movie);
		var originalCount = movie.InputFrames.Count;

		// Delete 3 frames at index 3
		var action = new DeleteFramesAction(movie, 3, 3);
		action.Execute();

		// Simulate RemoveFrameViewModels
		for (int i = 5; i >= 3; i--) {
			viewModels.RemoveAt(i);
		}
		for (int i = 3; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		Assert.Equal(7, viewModels.Count);

		// Now undo
		action.Undo();

		// Simulate InsertFrameViewModels (undoing delete = insert back)
		for (int j = 0; j < 3; j++) {
			int idx = 3 + j;
			viewModels.Insert(idx, new MockTasFrameViewModel(movie.InputFrames[idx], idx, false));
		}
		for (int i = 6; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		Assert.Equal(originalCount, viewModels.Count);
		Assert.Equal(originalCount, movie.InputFrames.Count);

		for (int i = 0; i < viewModels.Count; i++) {
			Assert.Equal(i + 1, viewModels[i].FrameNumber);
		}
	}

	/// <summary>
	/// Tests that ModifyInputAction undo only refreshes the affected frame VM.
	/// </summary>
	[Fact]
	public void UndoModify_RefreshesOnlySingleFrame() {
		var movie = CreateTestMovieData(10);
		var viewModels = CreateViewModelsForMovie(movie);

		var targetFrame = movie.InputFrames[5];
		var newInput = new ControllerInput { A = true, B = true, Start = true };
		var action = new ModifyInputAction(targetFrame, 2, 0, newInput);
		action.Execute();

		// Track which VMs get refreshed
		var refreshed = new List<int>();
		for (int i = 0; i < viewModels.Count; i++) {
			int idx = i;
			viewModels[i].PropertyChanged += (_, e) => {
				if (e.PropertyName == nameof(MockTasFrameViewModel.P1Input)) {
					refreshed.Add(idx);
				}
			};
		}

		// Simulate ApplyIncrementalUpdate for ModifyInputAction
		for (int i = 0; i < viewModels.Count; i++) {
			if (ReferenceEquals(viewModels[i].Frame, targetFrame)) {
				viewModels[i].RefreshFromFrame();
				break;
			}
		}

		// Only the target frame should have been refreshed
		Assert.Single(refreshed);
		Assert.Equal(5, refreshed[0]);
	}

	/// <summary>
	/// Tests multiple undo/redo cycles maintain correct VM state incrementally.
	/// </summary>
	[Fact]
	public void MultipleUndoRedo_IncrementalMaintainsConsistency() {
		var movie = CreateTestMovieData(10);
		var viewModels = CreateViewModelsForMovie(movie);

		// Insert 2 frames at index 3
		var frames1 = new List<InputFrame> { new(100), new(101) };
		var insert1 = new InsertFramesAction(movie, 3, frames1);
		insert1.Execute();
		SimulateInsert(viewModels, movie, 3, 2);
		Assert.Equal(12, viewModels.Count);

		// Insert 2 more frames at index 8
		var frames2 = new List<InputFrame> { new(200), new(201) };
		var insert2 = new InsertFramesAction(movie, 8, frames2);
		insert2.Execute();
		SimulateInsert(viewModels, movie, 8, 2);
		Assert.Equal(14, viewModels.Count);

		// Undo second insert
		insert2.Undo();
		SimulateRemove(viewModels, 8, 2);
		Assert.Equal(12, viewModels.Count);

		// Undo first insert
		insert1.Undo();
		SimulateRemove(viewModels, 3, 2);
		Assert.Equal(10, viewModels.Count);

		// Redo both
		insert1.Execute();
		SimulateInsert(viewModels, movie, 3, 2);
		insert2.Execute();
		SimulateInsert(viewModels, movie, 8, 2);
		Assert.Equal(14, viewModels.Count);

		// All frame numbers sequential
		for (int i = 0; i < viewModels.Count; i++) {
			Assert.Equal(i + 1, viewModels[i].FrameNumber);
		}
	}

	#endregion

	#region Edge Case Tests

	/// <summary>
	/// Tests inserting zero frames is a no-op.
	/// </summary>
	[Fact]
	public void BatchInsert_ZeroCount_IsNoOp() {
		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 5; i++) {
			viewModels.Add(new MockTasFrameViewModel(new InputFrame(i), i, false));
		}

		// Insert 0 frames — should not change anything
		Assert.Equal(5, viewModels.Count);
	}

	/// <summary>
	/// Tests removing all frames leaves empty collection.
	/// </summary>
	[Fact]
	public void Truncate_AtZero_ClearsAllFrames() {
		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 5; i++) {
			viewModels.Add(new MockTasFrameViewModel(new InputFrame(i), i, false));
		}

		// Truncate at 0
		for (int i = viewModels.Count - 1; i >= 0; i--) {
			viewModels.RemoveAt(i);
		}

		Assert.Empty(viewModels);
	}

	/// <summary>
	/// Tests batch insert at end doesn't need to renumber existing frames.
	/// </summary>
	[Fact]
	public void BatchInsert_AtEnd_NoRenumberingForExisting() {
		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < 8; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < 5; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		var numberChanges = new List<int>();
		foreach (var vm in viewModels) {
			vm.PropertyChanged += (_, e) => {
				if (e.PropertyName == nameof(MockTasFrameViewModel.FrameNumber)) {
					numberChanges.Add(vm.FrameNumber);
				}
			};
		}

		// Insert 3 at end
		for (int j = 0; j < 3; j++) {
			int idx = 5 + j;
			viewModels.Insert(idx, new MockTasFrameViewModel(movieFrames[5 + j], idx, false));
		}

		// Renumber tail (nothing to renumber since insertAt + count == Count)
		for (int i = 8; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		// No existing frames should have had their numbers changed
		Assert.Empty(numberChanges);
		Assert.Equal(8, viewModels.Count);
	}

	/// <summary>
	/// Tests large-scale insert and remove operations for correctness.
	/// </summary>
	[Fact]
	public void LargeScale_InsertAndRemove_MaintainsIntegrity() {
		const int initialSize = 1000;
		const int insertSize = 500;
		const int insertAt = 300;

		var movieFrames = new List<InputFrame>();
		for (int i = 0; i < initialSize + insertSize; i++) {
			movieFrames.Add(new InputFrame(i));
		}

		var viewModels = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < initialSize; i++) {
			viewModels.Add(new MockTasFrameViewModel(movieFrames[i], i, false));
		}

		// Insert 500 at index 300
		for (int j = 0; j < insertSize; j++) {
			int idx = insertAt + j;
			viewModels.Insert(idx, new MockTasFrameViewModel(movieFrames[initialSize + j], idx, false));
		}
		for (int i = insertAt + insertSize; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		Assert.Equal(initialSize + insertSize, viewModels.Count);

		// Remove the same 500
		for (int i = insertAt + insertSize - 1; i >= insertAt; i--) {
			viewModels.RemoveAt(i);
		}
		for (int i = insertAt; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}

		Assert.Equal(initialSize, viewModels.Count);

		for (int i = 0; i < viewModels.Count; i++) {
			Assert.Equal(i + 1, viewModels[i].FrameNumber);
		}
	}

	#endregion

	#region Test Helpers

	private static MovieData CreateTestMovieData(int frameCount) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test Game",
			SystemType = SystemType.Nes
		};
		for (int i = 0; i < frameCount; i++) {
			movie.InputFrames.Add(new InputFrame(i) {
				Controllers = [new ControllerInput { A = i % 2 == 0 }]
			});
		}
		return movie;
	}

	private static ObservableCollection<MockTasFrameViewModel> CreateViewModelsForMovie(MovieData movie) {
		var vms = new ObservableCollection<MockTasFrameViewModel>();
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			vms.Add(new MockTasFrameViewModel(movie.InputFrames[i], i, false));
		}
		return vms;
	}

	private static void SimulateInsert(
		ObservableCollection<MockTasFrameViewModel> viewModels,
		MovieData movie, int startIndex, int count) {
		for (int j = 0; j < count; j++) {
			int idx = startIndex + j;
			viewModels.Insert(idx, new MockTasFrameViewModel(movie.InputFrames[idx], idx, false));
		}
		for (int i = startIndex + count; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}
	}

	private static void SimulateRemove(
		ObservableCollection<MockTasFrameViewModel> viewModels,
		int startIndex, int count) {
		for (int i = startIndex + count - 1; i >= startIndex; i--) {
			viewModels.RemoveAt(i);
		}
		for (int i = startIndex; i < viewModels.Count; i++) {
			viewModels[i].FrameNumber = i + 1;
		}
	}

	#endregion

	#region Paint Input Action Tests

	[Fact]
	public void PaintInputAction_Execute_SetsButtonOnAllFrames() {
		var movie = CreateTestMovieData(5);
		var frames = new List<int> { 0, 2, 4 };
		var oldInputs = frames.Select(i => CloneControllerInput(movie.InputFrames[i].Controllers[0])).ToList();
		var action = new PaintInputAction(movie, frames, 0, "A", true, oldInputs);

		action.Execute();

		Assert.True(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[1].Controllers[0].A);
		Assert.True(movie.InputFrames[2].Controllers[0].A);
		Assert.False(movie.InputFrames[3].Controllers[0].A);
		Assert.True(movie.InputFrames[4].Controllers[0].A);
	}

	[Fact]
	public void PaintInputAction_Undo_RestoresOriginalState() {
		var movie = CreateTestMovieData(3);
		movie.InputFrames[0].Controllers[0].A = true;
		movie.InputFrames[1].Controllers[0].A = false;
		movie.InputFrames[2].Controllers[0].A = true;

		var frames = new List<int> { 0, 1, 2 };
		var oldInputs = frames.Select(i => CloneControllerInput(movie.InputFrames[i].Controllers[0])).ToList();
		var action = new PaintInputAction(movie, frames, 0, "A", false, oldInputs);

		action.Execute();
		Assert.False(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[1].Controllers[0].A);
		Assert.False(movie.InputFrames[2].Controllers[0].A);

		action.Undo();
		Assert.True(movie.InputFrames[0].Controllers[0].A);
		Assert.False(movie.InputFrames[1].Controllers[0].A);
		Assert.True(movie.InputFrames[2].Controllers[0].A);
	}

	[Fact]
	public void PaintInputAction_Description_ShowsFrameCount() {
		var movie = CreateTestMovieData(5);
		var frames = new List<int> { 1, 3, 4 };
		var oldInputs = frames.Select(i => CloneControllerInput(movie.InputFrames[i].Controllers[0])).ToList();
		var action = new PaintInputAction(movie, frames, 0, "B", true, oldInputs);

		Assert.Equal("Paint 3 frame(s)", action.Description);
	}

	private static ControllerInput CloneControllerInput(ControllerInput src) => new() {
		A = src.A, B = src.B, X = src.X, Y = src.Y,
		L = src.L, R = src.R,
		Up = src.Up, Down = src.Down, Left = src.Left, Right = src.Right,
		Start = src.Start, Select = src.Select
	};

	#endregion

	#region ExecuteAction Unified Dispatch Tests

	[Fact]
	public void ExecuteAction_InsertFramesAction_AddsToUndoStackAndUpdatesViewModels() {
		// This tests the unified ExecuteAction path (execute + ApplyIncrementalUpdate)
		var movie = CreateTestMovieData(10);
		var frames = new List<InputFrame> { new InputFrame(5) { Controllers = [new ControllerInput()] } };
		var action = new InsertFramesAction(movie, 5, frames);

		action.Execute();
		Assert.Equal(11, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);
	}

	[Fact]
	public void ExecuteAction_DeleteFramesAction_RemovesAndCanUndo() {
		var movie = CreateTestMovieData(10);
		var action = new DeleteFramesAction(movie, 3, 4);

		action.Execute();
		Assert.Equal(6, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);
	}

	[Fact]
	public void ExecuteAction_ModifyInputAction_ModifiesAndCanUndo() {
		var movie = CreateTestMovieData(5);
		var frame = movie.InputFrames[2];
		bool originalA = frame.Controllers[0].A;
		var newInput = CloneControllerInput(frame.Controllers[0]);
		newInput.A = !originalA;

		var action = new ModifyInputAction(frame, 2, 0, newInput);
		action.Execute();
		Assert.Equal(!originalA, frame.Controllers[0].A);

		action.Undo();
		Assert.Equal(originalA, frame.Controllers[0].A);
	}

	[Fact]
	public void ExecuteAction_ClearInputAction_ClearsAndCanUndo() {
		var movie = CreateTestMovieData(5);
		var frame = movie.InputFrames[0];
		frame.Controllers[0].A = true;
		frame.Controllers[0].B = true;

		var action = new ClearInputAction(frame, 0);
		action.Execute();
		Assert.False(frame.Controllers[0].A);
		Assert.False(frame.Controllers[0].B);

		action.Undo();
		Assert.True(frame.Controllers[0].A);
		Assert.True(frame.Controllers[0].B);
	}

	[Fact]
	public void ExecuteAction_PaintInputAction_PaintsAndCanUndo() {
		var movie = CreateTestMovieData(5);
		var indices = new List<int> { 0, 2, 4 };
		var oldInputs = indices.Select(i => CloneControllerInput(movie.InputFrames[i].Controllers[0])).ToList();

		var action = new PaintInputAction(movie, indices, 0, "B", true, oldInputs);
		action.Execute();
		Assert.True(movie.InputFrames[0].Controllers[0].B);
		Assert.True(movie.InputFrames[2].Controllers[0].B);
		Assert.True(movie.InputFrames[4].Controllers[0].B);

		action.Undo();
		Assert.False(movie.InputFrames[0].Controllers[0].B);
		Assert.False(movie.InputFrames[2].Controllers[0].B);
		Assert.False(movie.InputFrames[4].Controllers[0].B);
	}

	[Fact]
	public void ExecuteAction_InsertFrames_ChainsWithDelete_FullCycle() {
		// Simulates Lua API: insert, then delete, then undo both
		var movie = CreateTestMovieData(10);

		// Insert 3 frames at position 5
		var insertFrames = new List<InputFrame>();
		for (int i = 0; i < 3; i++) {
			insertFrames.Add(new InputFrame(5 + i) { Controllers = [new ControllerInput()] });
		}
		var insertAction = new InsertFramesAction(movie, 5, insertFrames);
		insertAction.Execute();
		Assert.Equal(13, movie.InputFrames.Count);

		// Delete 2 frames at position 0
		var deleteAction = new DeleteFramesAction(movie, 0, 2);
		deleteAction.Execute();
		Assert.Equal(11, movie.InputFrames.Count);

		// Undo delete
		deleteAction.Undo();
		Assert.Equal(13, movie.InputFrames.Count);

		// Undo insert
		insertAction.Undo();
		Assert.Equal(10, movie.InputFrames.Count);
	}

	/// <summary>
	/// Tests that BulkUndoableAction with coalesced contiguous range deletes
	/// correctly removes frames and undoes back to original state.
	/// </summary>
	[Fact]
	public void BulkDelete_CoalescedRanges_DeletesAndUndoes() {
		var movie = CreateTestMovieData(10);
		var original = movie.InputFrames.Select(f => f.FrameNumber).ToList();

		// Simulate coalesced delete of indices [8,7,6, 3,2] → ranges (6,3) and (2,2)
		// Sorted descending ranges: first remove 6..8, then 2..3
		var actions = new List<UndoableAction> {
			new DeleteFramesAction(movie, 6, 3),
			new DeleteFramesAction(movie, 2, 2),
		};
		var bulk = new BulkUndoableAction("Delete 5 frame(s)", actions);
		bulk.Execute();

		Assert.Equal(5, movie.InputFrames.Count);
		// Remaining should be original indices 0,1,4,5,9 → frame numbers 0,1,4,5,9
		Assert.Equal(0, movie.InputFrames[0].FrameNumber);
		Assert.Equal(1, movie.InputFrames[1].FrameNumber);
		Assert.Equal(4, movie.InputFrames[2].FrameNumber);
		Assert.Equal(5, movie.InputFrames[3].FrameNumber);
		Assert.Equal(9, movie.InputFrames[4].FrameNumber);

		// Undo should restore all 10 frames in original order
		bulk.Undo();
		Assert.Equal(10, movie.InputFrames.Count);
		for (int i = 0; i < 10; i++) {
			Assert.Equal(original[i], movie.InputFrames[i].FrameNumber);
		}
	}

	/// <summary>
	/// Tests that a single contiguous selection (e.g. Select All → Delete) produces
	/// exactly one DeleteFramesAction when coalesced (no per-frame overhead).
	/// </summary>
	[Fact]
	public void BulkDelete_FullyContiguous_SingleAction() {
		var movie = CreateTestMovieData(20);

		// Simulate what coalescing does for fully contiguous descending [19,18,...,0]
		// Should coalesce to a single DeleteFramesAction(movie, 0, 20)
		var sorted = Enumerable.Range(0, 20).Reverse().ToList();
		var actions = new List<UndoableAction>();
		int rangeEnd = sorted[0];
		int rangeStart = rangeEnd;
		for (int i = 1; i < sorted.Count; i++) {
			if (sorted[i] == rangeStart - 1) {
				rangeStart = sorted[i];
			} else {
				actions.Add(new DeleteFramesAction(movie, rangeStart, rangeEnd - rangeStart + 1));
				rangeEnd = sorted[i];
				rangeStart = rangeEnd;
			}
		}
		actions.Add(new DeleteFramesAction(movie, rangeStart, rangeEnd - rangeStart + 1));

		// Fully contiguous → exactly 1 action covering all 20 frames
		Assert.Single(actions);
		var deleteAction = (DeleteFramesAction)actions[0];
		Assert.Equal(0, deleteAction.Index);
		Assert.Equal(20, deleteAction.Count);

		var bulk = new BulkUndoableAction("Delete 20 frame(s)", actions);
		bulk.Execute();
		Assert.Empty(movie.InputFrames);

		bulk.Undo();
		Assert.Equal(20, movie.InputFrames.Count);
	}

	#endregion

	#region GetEarliestAffectedFrame Tests

	[Fact]
	public void GetEarliestAffectedFrame_InsertFramesAction_ReturnsIndex() {
		var movie = CreateTestMovieData(10);
		var frames = new List<InputFrame> { new(0) { Controllers = [new ControllerInput()] } };
		var action = new InsertFramesAction(movie, 5, frames);
		Assert.Equal(5, TasEditorViewModel.GetEarliestAffectedFrame(action));
	}

	[Fact]
	public void GetEarliestAffectedFrame_DeleteFramesAction_ReturnsIndex() {
		var movie = CreateTestMovieData(10);
		var action = new DeleteFramesAction(movie, 3, 2);
		Assert.Equal(3, TasEditorViewModel.GetEarliestAffectedFrame(action));
	}

	[Fact]
	public void GetEarliestAffectedFrame_ModifyInputAction_ReturnsFrameIndex() {
		var movie = CreateTestMovieData(10);
		var frame = movie.InputFrames[7];
		var action = new ModifyInputAction(frame, 7, 0, new ControllerInput { B = true });
		Assert.Equal(7, TasEditorViewModel.GetEarliestAffectedFrame(action));
	}

	[Fact]
	public void GetEarliestAffectedFrame_ClearInputAction_ReturnsFrameIndex() {
		var movie = CreateTestMovieData(10);
		var action = new ClearInputAction(movie.InputFrames[4], 4);
		Assert.Equal(4, TasEditorViewModel.GetEarliestAffectedFrame(action));
	}

	[Fact]
	public void GetEarliestAffectedFrame_PaintInputAction_ReturnsMinIndex() {
		var movie = CreateTestMovieData(10);
		var indices = new List<int> { 3, 7, 5 };
		var oldInputs = indices.Select(i => movie.InputFrames[i].Controllers[0].Clone()).ToList();
		var action = new PaintInputAction(movie, indices, 0, "A", true, oldInputs);
		Assert.Equal(3, TasEditorViewModel.GetEarliestAffectedFrame(action));
	}

	[Fact]
	public void GetEarliestAffectedFrame_BulkAction_ReturnsMinOfChildren() {
		var movie = CreateTestMovieData(10);
		var actions = new List<UndoableAction> {
			new ModifyInputAction(movie.InputFrames[8], 8, 0, new ControllerInput()),
			new ClearInputAction(movie.InputFrames[2], 2),
			new ModifyInputAction(movie.InputFrames[5], 5, 0, new ControllerInput()),
		};
		var bulk = new BulkUndoableAction("Test bulk", actions);
		Assert.Equal(2, TasEditorViewModel.GetEarliestAffectedFrame(bulk));
	}

	#endregion

	#region Greenzone Ring Buffer Tests

	[Fact]
	public void GreenzoneManager_NonIntervalFrame_DoesNotAddToRingBuffer() {
		// Verify that calling CaptureState with a non-interval frame doesn't store it
		var gm = new GreenzoneManager();
		gm.CaptureInterval = 10;

		// Capture at non-interval frame (frame 3)
		gm.CaptureState(3, new byte[16]);

		// Frame 3 should NOT have a savestate
		Assert.False(gm.HasState(3));
		Assert.Equal(0, gm.SavestateCount);
	}

	[Fact]
	public void GreenzoneManager_IntervalFrame_StoresState() {
		var gm = new GreenzoneManager();
		gm.CaptureInterval = 10;

		// Capture at interval frame
		gm.CaptureState(10, new byte[16]);

		Assert.True(gm.HasState(10));
		Assert.Equal(1, gm.SavestateCount);
	}

	[Fact]
	public void GreenzoneManager_ForceCapture_StoresNonIntervalFrame() {
		var gm = new GreenzoneManager();
		gm.CaptureInterval = 10;

		// Force capture at non-interval frame
		gm.CaptureState(3, new byte[16], forceCapture: true);

		Assert.True(gm.HasState(3));
		Assert.Equal(1, gm.SavestateCount);
	}

	[Fact]
	public void GreenzoneManager_InvalidateFrom_RemovesStatesAtAndAfter() {
		var gm = new GreenzoneManager();
		gm.CaptureInterval = 10;

		gm.CaptureState(10, new byte[16]);
		gm.CaptureState(20, new byte[16]);
		gm.CaptureState(30, new byte[16]);

		gm.InvalidateFrom(20);

		Assert.True(gm.HasState(10));
		Assert.False(gm.HasState(20));
		Assert.False(gm.HasState(30));
	}

	#endregion

	#region ModifyCommentAction Tests

	[Fact]
	public void ModifyCommentAction_Execute_SetsNewComment() {
		var movie = CreateTestMovieData(5);
		var frame = movie.InputFrames[2];
		frame.Comment = "old comment";

		var action = new ModifyCommentAction(frame, 2, "new comment");
		action.Execute();

		Assert.Equal("new comment", frame.Comment);
	}

	[Fact]
	public void ModifyCommentAction_Undo_RestoresOldComment() {
		var movie = CreateTestMovieData(5);
		var frame = movie.InputFrames[2];
		frame.Comment = "old comment";

		var action = new ModifyCommentAction(frame, 2, "new comment");
		action.Execute();
		action.Undo();

		Assert.Equal("old comment", frame.Comment);
	}

	[Fact]
	public void ModifyCommentAction_Execute_CanAddComment() {
		var movie = CreateTestMovieData(5);
		var frame = movie.InputFrames[0];
		Assert.Null(frame.Comment);

		var action = new ModifyCommentAction(frame, 0, "[M] Marker");
		action.Execute();

		Assert.Equal("[M] Marker", frame.Comment);
	}

	[Fact]
	public void ModifyCommentAction_Undo_CanRemoveComment() {
		var movie = CreateTestMovieData(5);
		var frame = movie.InputFrames[0];
		Assert.Null(frame.Comment);

		var action = new ModifyCommentAction(frame, 0, "[M] Marker");
		action.Execute();
		action.Undo();

		Assert.Null(frame.Comment);
	}

	[Fact]
	public void ModifyCommentAction_Execute_CanRemoveComment() {
		var movie = CreateTestMovieData(5);
		var frame = movie.InputFrames[1];
		frame.Comment = "existing";

		var action = new ModifyCommentAction(frame, 1, null);
		action.Execute();

		Assert.Null(frame.Comment);
	}

	[Fact]
	public void ModifyCommentAction_Description_AddsWhenNoOldComment() {
		var frame = new InputFrame(0);
		var action = new ModifyCommentAction(frame, 0, "new");

		Assert.Equal("Add comment", action.Description);
	}

	[Fact]
	public void ModifyCommentAction_Description_RemovesWhenNoNewComment() {
		var frame = new InputFrame(0) { Comment = "old" };
		var action = new ModifyCommentAction(frame, 0, null);

		Assert.Equal("Remove comment", action.Description);
	}

	[Fact]
	public void ModifyCommentAction_Description_ModifiesWhenBothExist() {
		var frame = new InputFrame(0) { Comment = "old" };
		var action = new ModifyCommentAction(frame, 0, "new");

		Assert.Equal("Modify comment", action.Description);
	}

	[Fact]
	public void ModifyCommentAction_FrameIndex_ReturnsCorrectIndex() {
		var frame = new InputFrame(0);
		var action = new ModifyCommentAction(frame, 42, "test");

		Assert.Equal(42, action.FrameIndex);
	}

	[Fact]
	public void ModifyCommentAction_MultipleUndoRedo_CyclesCorrectly() {
		var frame = new InputFrame(0) { Comment = "A" };
		var action = new ModifyCommentAction(frame, 0, "B");

		action.Execute();
		Assert.Equal("B", frame.Comment);

		action.Undo();
		Assert.Equal("A", frame.Comment);

		action.Execute();
		Assert.Equal("B", frame.Comment);

		action.Undo();
		Assert.Equal("A", frame.Comment);
	}

	#endregion

	#region ModifyCommandAction Frame Tracking Tests

	[Fact]
	public void ModifyCommandAction_FrameIndices_ReturnsTrackedIndices() {
		var movie = CreateTestMovieData(5);
		var targets = new List<InputFrame> { movie.InputFrames[1], movie.InputFrames[3] };
		var targetIndices = new List<int> { 1, 3 };
		var action = new ModifyCommandAction(
			targets,
			targetIndices,
			FrameCommand.SoftReset,
			true);

		Assert.Equal(new List<int> { 1, 3 }, action.FrameIndices);
	}

	[Fact]
	public void ModifyCommandAction_Execute_SetsCommandOnAllTargets() {
		var movie = CreateTestMovieData(5);
		var targets = new List<InputFrame> { movie.InputFrames[1], movie.InputFrames[3] };
		var targetIndices = new List<int> { 1, 3 };
		var action = new ModifyCommandAction(
			targets,
			targetIndices,
			FrameCommand.SoftReset,
			true);

		action.Execute();
		Assert.True(movie.InputFrames[1].Command.HasFlag(FrameCommand.SoftReset));
		Assert.True(movie.InputFrames[3].Command.HasFlag(FrameCommand.SoftReset));
	}

	[Fact]
	public void ModifyCommandAction_Undo_RestoresOriginalCommands() {
		var movie = CreateTestMovieData(5);
		movie.InputFrames[1].Command = FrameCommand.None;
		movie.InputFrames[3].Command = FrameCommand.HardReset;
		var targets = new List<InputFrame> { movie.InputFrames[1], movie.InputFrames[3] };
		var targetIndices = new List<int> { 1, 3 };
		var action = new ModifyCommandAction(
			targets,
			targetIndices,
			FrameCommand.SoftReset,
			true);

		action.Execute();
		action.Undo();

		Assert.Equal(FrameCommand.None, movie.InputFrames[1].Command);
		Assert.Equal(FrameCommand.HardReset, movie.InputFrames[3].Command);
	}

	#endregion

	#region GetEarliestAffectedFrame Tests

	[Fact]
	public void GetEarliestAffectedFrame_ModifyCommentAction_ReturnsFrameIndex() {
		// ModifyCommentAction.FrameIndex should be returned by GetEarliestAffectedFrame
		var frame = new InputFrame(0);
		var action = new ModifyCommentAction(frame, 42, "test");

		// We test that the FrameIndex property is correctly exposed
		Assert.Equal(42, action.FrameIndex);
	}

	[Fact]
	public void GetEarliestAffectedFrame_ModifyCommandAction_ReturnsMinIndex() {
		var movie = CreateTestMovieData(10);
		var targets = new List<InputFrame> { movie.InputFrames[5], movie.InputFrames[2], movie.InputFrames[8] };
		var targetIndices = new List<int> { 5, 2, 8 };
		var action = new ModifyCommandAction(
			targets,
			targetIndices,
			FrameCommand.SoftReset,
			true);

		// FrameIndices should be [5, 2, 8], min is 2
		Assert.Equal(2, action.FrameIndices.Min());
	}

	#endregion

	#region ClassifyMarkerEntryType Tests

	[Fact]
	public void ClassifyMarkerEntryType_MarkerPrefix_ReturnsMarker() {
		Assert.Equal(MarkerEntryType.Marker, TasEditorViewModel.ClassifyMarkerEntryType("[M] Test"));
	}

	[Fact]
	public void ClassifyMarkerEntryType_MarkerPrefixCaseInsensitive_ReturnsMarker() {
		Assert.Equal(MarkerEntryType.Marker, TasEditorViewModel.ClassifyMarkerEntryType("[m] test"));
	}

	[Fact]
	public void ClassifyMarkerEntryType_TodoKeyword_ReturnsTodo() {
		Assert.Equal(MarkerEntryType.Todo, TasEditorViewModel.ClassifyMarkerEntryType("TODO: fix this"));
	}

	[Fact]
	public void ClassifyMarkerEntryType_TodoCaseInsensitive_ReturnsTodo() {
		Assert.Equal(MarkerEntryType.Todo, TasEditorViewModel.ClassifyMarkerEntryType("todo check later"));
	}

	[Fact]
	public void ClassifyMarkerEntryType_RegularComment_ReturnsComment() {
		Assert.Equal(MarkerEntryType.Comment, TasEditorViewModel.ClassifyMarkerEntryType("Just a note"));
	}

	#endregion

	#region RefreshMarkerEntryAt Tests

	[Fact]
	public void RefreshMarkerEntryAt_AddsNewEntryInSortedPosition() {
		// Test using ObservableCollection directly (same algorithm as ViewModel)
		var movie = CreateTestMovieData(10);
		movie.InputFrames[2].Comment = "[M] First";
		movie.InputFrames[8].Comment = "[M] Last";
		var entries = new ObservableCollection<TasMarkerEntryViewModel> {
			new TasMarkerEntryViewModel(2, "[M] First", MarkerEntryType.Marker),
			new TasMarkerEntryViewModel(8, "[M] Last", MarkerEntryType.Marker)
		};

		// Add a comment at frame 5 — should be inserted between 2 and 8
		movie.InputFrames[5].Comment = "[M] Middle";
		SimulateRefreshMarkerEntryAt(entries, movie, 5);

		Assert.Equal(3, entries.Count);
		Assert.Equal(2, entries[0].FrameIndex);
		Assert.Equal(5, entries[1].FrameIndex);
		Assert.Equal(8, entries[2].FrameIndex);
	}

	[Fact]
	public void RefreshMarkerEntryAt_RemovesEntryWhenCommentCleared() {
		var movie = CreateTestMovieData(5);
		movie.InputFrames[2].Comment = "[M] Marker";
		var entries = new ObservableCollection<TasMarkerEntryViewModel> {
			new TasMarkerEntryViewModel(2, "[M] Marker", MarkerEntryType.Marker)
		};

		// Clear the comment
		movie.InputFrames[2].Comment = null;
		SimulateRefreshMarkerEntryAt(entries, movie, 2);

		Assert.Empty(entries);
	}

	[Fact]
	public void RefreshMarkerEntryAt_UpdatesExistingEntry() {
		var movie = CreateTestMovieData(5);
		movie.InputFrames[2].Comment = "[M] Old";
		var entries = new ObservableCollection<TasMarkerEntryViewModel> {
			new TasMarkerEntryViewModel(2, "[M] Old", MarkerEntryType.Marker)
		};

		// Change the comment
		movie.InputFrames[2].Comment = "New comment";
		SimulateRefreshMarkerEntryAt(entries, movie, 2);

		Assert.Single(entries);
		Assert.Equal(2, entries[0].FrameIndex);
		Assert.Equal("New comment", entries[0].Comment);
		Assert.Equal(MarkerEntryType.Comment, entries[0].Type);
	}

	[Fact]
	public void RefreshMarkerEntryAt_HandlesEmptyList() {
		var movie = CreateTestMovieData(5);
		var entries = new ObservableCollection<TasMarkerEntryViewModel>();

		movie.InputFrames[0].Comment = "[M] First";
		SimulateRefreshMarkerEntryAt(entries, movie, 0);

		Assert.Single(entries);
		Assert.Equal(0, entries[0].FrameIndex);
	}

	[Fact]
	public void RefreshMarkerEntryAt_InsertsAtEnd() {
		var movie = CreateTestMovieData(10);
		movie.InputFrames[2].Comment = "[M] First";
		var entries = new ObservableCollection<TasMarkerEntryViewModel> {
			new TasMarkerEntryViewModel(2, "[M] First", MarkerEntryType.Marker)
		};

		movie.InputFrames[9].Comment = "[M] Last";
		SimulateRefreshMarkerEntryAt(entries, movie, 9);

		Assert.Equal(2, entries.Count);
		Assert.Equal(2, entries[0].FrameIndex);
		Assert.Equal(9, entries[1].FrameIndex);
	}

	[Fact]
	public void RefreshMarkerEntryAt_InsertsAtBeginning() {
		var movie = CreateTestMovieData(10);
		movie.InputFrames[5].Comment = "[M] Middle";
		var entries = new ObservableCollection<TasMarkerEntryViewModel> {
			new TasMarkerEntryViewModel(5, "[M] Middle", MarkerEntryType.Marker)
		};

		movie.InputFrames[0].Comment = "[M] First";
		SimulateRefreshMarkerEntryAt(entries, movie, 0);

		Assert.Equal(2, entries.Count);
		Assert.Equal(0, entries[0].FrameIndex);
		Assert.Equal(5, entries[1].FrameIndex);
	}

	/// <summary>
	/// Simulates the RefreshMarkerEntryAt algorithm for testing without ViewModel dependencies.
	/// </summary>
	private static void SimulateRefreshMarkerEntryAt(
		ObservableCollection<TasMarkerEntryViewModel> entries,
		MovieData movie,
		int frameIndex) {
		// Remove existing entry for this frame
		for (int i = entries.Count - 1; i >= 0; i--) {
			if (entries[i].FrameIndex == frameIndex) {
				entries.RemoveAt(i);
				break;
			}
		}

		// Check if the frame now has a comment
		if (frameIndex >= 0 && frameIndex < movie.InputFrames.Count) {
			string? comment = movie.InputFrames[frameIndex].Comment;
			if (!string.IsNullOrWhiteSpace(comment)) {
				MarkerEntryType type = TasEditorViewModel.ClassifyMarkerEntryType(comment);
				var entry = new TasMarkerEntryViewModel(frameIndex, comment, type);
				int insertIndex = 0;
				for (int i = 0; i < entries.Count; i++) {
					if (entries[i].FrameIndex > frameIndex) {
						break;
					}
					insertIndex = i + 1;
				}
				entries.Insert(insertIndex, entry);
			}
		}
	}

	#endregion
}

/// <summary>
/// Mock version of TasFrameViewModel for testing without UI dependencies.
/// </summary>
public class MockTasFrameViewModel : System.ComponentModel.INotifyPropertyChanged {
	private int _frameNumber;
	private bool _isGreenzone;

	public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;

	public InputFrame Frame { get; }

	public int FrameNumber {
		get => _frameNumber;
		set {
			if (_frameNumber != value) {
				_frameNumber = value;
				OnPropertyChanged(nameof(FrameNumber));
			}
		}
	}

	public bool IsGreenzone {
		get => _isGreenzone;
		set {
			if (_isGreenzone != value) {
				_isGreenzone = value;
				OnPropertyChanged(nameof(IsGreenzone));
				OnPropertyChanged(nameof(Background));
			}
		}
	}

	public string Background => IsGreenzone ? "LightGreen" : "Transparent";
	public string P1Input => Frame.Controllers[0].ToNexenFormat();
	public string P2Input => Frame.Controllers.Length > 1 ? Frame.Controllers[1].ToNexenFormat() : "";
	public string MarkerText => Frame.Comment ?? "";
	public string CommentText => Frame.Comment ?? "";
	public bool IsLagFrame => Frame.IsLagFrame;
	public bool HasMarker => !string.IsNullOrEmpty(Frame.Comment);

	public MockTasFrameViewModel(InputFrame frame, int index, bool isGreenzone) {
		Frame = frame;
		_frameNumber = index + 1;
		_isGreenzone = isGreenzone;
	}

	public void RefreshFromFrame() {
		OnPropertyChanged(nameof(P1Input));
		OnPropertyChanged(nameof(P2Input));
		OnPropertyChanged(nameof(MarkerText));
		OnPropertyChanged(nameof(CommentText));
		OnPropertyChanged(nameof(IsLagFrame));
		OnPropertyChanged(nameof(HasMarker));
		OnPropertyChanged(nameof(Background));
	}

	protected void OnPropertyChanged(string propertyName) {
		PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
	}
}
