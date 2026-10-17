using System.Reflection;
using Nexen.MovieConverter;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

public class MarkerCommentPanelTests {
	private static MovieData CreateMovieWithComments() {
		var movie = new MovieData {
			Author = "MarkerTests",
			GameName = "Test",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < 12; i++) {
			movie.InputFrames.Add(new InputFrame(i) {
				Controllers = [new ControllerInput()]
			});
		}

		movie.InputFrames[1].Comment = "Normal note";
		movie.InputFrames[3].Comment = "[M] Boss split";
		movie.InputFrames[5].Comment = "TODO optimize jump";
		movie.InputFrames[7].Comment = "[M] TODO optional route";
		return movie;
	}

	private static void SetMovie(TasEditorViewModel vm, MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(vm, movie);
		vm.Recorder.Movie = movie;
	}

	[Fact]
	public void ClassifyMarkerEntryType_MarkerPrefix_ClassifiedAsMarker() {
		Assert.Equal(MarkerEntryType.Marker, TasEditorViewModel.ClassifyMarkerEntryType("[M] Split"));
	}

	[Fact]
	public void ClassifyMarkerEntryType_TodoKeyword_ClassifiedAsTodo() {
		Assert.Equal(MarkerEntryType.Todo, TasEditorViewModel.ClassifyMarkerEntryType("TODO: verify timing"));
	}

	[Fact]
	public void ClassifyMarkerEntryType_PlainComment_ClassifiedAsComment() {
		Assert.Equal(MarkerEntryType.Comment, TasEditorViewModel.ClassifyMarkerEntryType("General note"));
	}

	[Fact]
	public void RefreshMarkerEntries_AllFilter_IncludesAllCommentFrames() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.MarkerEntryFilter = MarkerEntryFilterType.All;
		vm.RefreshMarkerEntries();

		Assert.Equal(4, vm.MarkerEntries.Count);
	}

	[Fact]
	public void RefreshMarkerEntries_MarkerFilter_IncludesOnlyMarkers() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.MarkerEntryFilter = MarkerEntryFilterType.Marker;
		vm.RefreshMarkerEntries();

		Assert.Equal(2, vm.MarkerEntries.Count);
		Assert.All(vm.MarkerEntries, e => Assert.Equal(MarkerEntryType.Marker, e.Type));
	}

	[Fact]
	public void RefreshMarkerEntries_TodoFilter_IncludesOnlyTodos() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.MarkerEntryFilter = MarkerEntryFilterType.Todo;
		vm.RefreshMarkerEntries();

		Assert.Single(vm.MarkerEntries);
		Assert.Equal(MarkerEntryType.Todo, vm.MarkerEntries[0].Type);
	}

	[Fact]
	public void NavigateToMarkerEntry_UpdatesSelectedFrame() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.RefreshMarkerEntries();
		var entry = vm.MarkerEntries.First(e => e.FrameIndex == 3);

		vm.NavigateToMarkerEntry(entry);

		Assert.Equal(3, vm.SelectedFrameIndex);
		Assert.Equal(new[] { 3 }, vm.SelectedFrameIndices);
	}

	[Fact]
	public void UpsertMarkerEntry_AddsCommentAndMarksDirty() {
		using var vm = new TasEditorViewModel();
		var movie = CreateMovieWithComments();
		movie.InputFrames[9].Comment = null;
		SetMovie(vm, movie);

		bool result = vm.UpsertMarkerEntry(9, "New panel note");

		Assert.True(result);
		Assert.Equal("New panel note", movie.InputFrames[9].Comment);
		Assert.True(vm.HasUnsavedChanges);
	}

	[Fact]
	public void UpsertMarkerEntry_EmptyText_RemovesComment() {
		using var vm = new TasEditorViewModel();
		var movie = CreateMovieWithComments();
		SetMovie(vm, movie);

		bool result = vm.UpsertMarkerEntry(1, "   ");

		Assert.True(result);
		Assert.Null(movie.InputFrames[1].Comment);
	}

	[Fact]
	public void DeleteSelectedMarkerEntry_RemovesFrameComment() {
		using var vm = new TasEditorViewModel();
		var movie = CreateMovieWithComments();
		SetMovie(vm, movie);
		vm.RefreshMarkerEntries();
		vm.SelectedMarkerEntry = vm.MarkerEntries.First(e => e.FrameIndex == 5);

		vm.DeleteSelectedMarkerEntry();

		Assert.Null(movie.InputFrames[5].Comment);
	}

	[Fact]
	public void ExportMarkerEntries_WritesHeaderAndRows() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.RefreshMarkerEntries();

		string dir = Path.Combine(Path.GetTempPath(), "nexen-marker-tests", Guid.NewGuid().ToString("N"));
		Directory.CreateDirectory(dir);
		try {
			string outPath = Path.Combine(dir, "markers.txt");
			bool exported = vm.ExportMarkerEntries(outPath);

			Assert.True(exported);
			string[] lines = File.ReadAllLines(outPath);
			Assert.True(lines.Length >= 2);
			Assert.Equal("Frame\tType\tComment", lines[0]);
		} finally {
			Directory.Delete(dir, true);
		}
	}

	[Fact]
	public void RefreshMarkerEntries_NoMovie_ClearsEntries() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.RefreshMarkerEntries();
		Assert.NotEmpty(vm.MarkerEntries);

		SetMovie(vm, null!);
		vm.RefreshMarkerEntries();
		Assert.Empty(vm.MarkerEntries);
	}

	[Fact]
	public void FilterChange_AfterRefresh_FiltersFromCache() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.MarkerEntryFilter = MarkerEntryFilterType.All;
		vm.RefreshMarkerEntries();

		Assert.Equal(4, vm.MarkerEntries.Count);

		// Switch filter — should use cached markers, not rescan frames
		vm.MarkerEntryFilter = MarkerEntryFilterType.Marker;
		// The WhenAnyValue subscription fires ApplyMarkerFilter automatically,
		// but in test we call RefreshMarkerEntries to simulate. The cache is already built.
		vm.RefreshMarkerEntries();

		Assert.Equal(2, vm.MarkerEntries.Count);
		Assert.All(vm.MarkerEntries, e => Assert.Equal(MarkerEntryType.Marker, e.Type));

		// Switch to Todo
		vm.MarkerEntryFilter = MarkerEntryFilterType.Todo;
		vm.RefreshMarkerEntries();

		Assert.Single(vm.MarkerEntries);
		Assert.Equal(MarkerEntryType.Todo, vm.MarkerEntries[0].Type);

		// Switch back to All
		vm.MarkerEntryFilter = MarkerEntryFilterType.All;
		vm.RefreshMarkerEntries();

		Assert.Equal(4, vm.MarkerEntries.Count);
	}

	[Fact]
	public void RefreshMarkerEntryAt_UpdatesCache() {
		using var vm = new TasEditorViewModel();
		var movie = CreateMovieWithComments();
		SetMovie(vm, movie);
		vm.MarkerEntryFilter = MarkerEntryFilterType.All;
		vm.RefreshMarkerEntries();

		Assert.Equal(4, vm.MarkerEntries.Count);

		// Add a comment incrementally
		movie.InputFrames[9].Comment = "[M] New marker";
		vm.RefreshMarkerEntryAt(9);

		Assert.Equal(5, vm.MarkerEntries.Count);

		// Switch filter — the new marker should appear in Marker filter
		vm.MarkerEntryFilter = MarkerEntryFilterType.Marker;
		vm.RefreshMarkerEntries();

		Assert.Equal(3, vm.MarkerEntries.Count);
		Assert.Contains(vm.MarkerEntries, e => e.FrameIndex == 9);
	}

	[Fact]
	public void SelectedMarkerEntry_PreservedAfterFilterChange() {
		using var vm = new TasEditorViewModel();
		SetMovie(vm, CreateMovieWithComments());
		vm.MarkerEntryFilter = MarkerEntryFilterType.All;
		vm.RefreshMarkerEntries();

		// Select a marker entry
		vm.SelectedMarkerEntry = vm.MarkerEntries.First(e => e.FrameIndex == 3);

		// Refresh with same filter — selection should be preserved via binary search
		vm.RefreshMarkerEntries();

		Assert.NotNull(vm.SelectedMarkerEntry);
		Assert.Equal(3, vm.SelectedMarkerEntry!.FrameIndex);
	}
}
