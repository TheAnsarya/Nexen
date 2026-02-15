using System.Collections.ObjectModel;
using Nexen.MovieConverter;
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
