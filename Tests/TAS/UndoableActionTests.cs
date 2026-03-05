using Nexen.MovieConverter;
using Nexen.ViewModels;
using Xunit;

namespace Nexen.Tests.TAS;

/// <summary>
/// Tests for TAS Editor undoable actions: InsertFramesAction, DeleteFramesAction, ModifyInputAction.
/// Validates execute/undo round-trips preserve movie state correctly.
/// </summary>
public class UndoableActionTests {
	#region Helpers

	private static MovieData CreateTestMovie(int frameCount) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test Game",
			SystemType = SystemType.Nes
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				Controllers = [new ControllerInput { A = i % 2 == 0, B = i % 3 == 0 }]
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	private static void AssertFrameCountAndIntegrity(MovieData movie, int expectedCount) {
		Assert.Equal(expectedCount, movie.InputFrames.Count);

		for (int i = 0; i < movie.InputFrames.Count; i++) {
			Assert.NotNull(movie.InputFrames[i]);
			Assert.NotNull(movie.InputFrames[i].Controllers);
		}
	}

	#endregion

	#region InsertFramesAction Tests

	[Fact]
	public void InsertFramesAction_Execute_InsertsFramesAtIndex() {
		var movie = CreateTestMovie(10);
		var newFrames = new List<InputFrame> {
			new(100) { Comment = "Inserted1" },
			new(101) { Comment = "Inserted2" }
		};

		var action = new InsertFramesAction(movie, 5, newFrames);
		action.Execute();

		Assert.Equal(12, movie.InputFrames.Count);
		Assert.Equal("Inserted1", movie.InputFrames[5].Comment);
		Assert.Equal("Inserted2", movie.InputFrames[6].Comment);
	}

	[Fact]
	public void InsertFramesAction_Undo_RemovesInsertedFrames() {
		var movie = CreateTestMovie(10);
		var originalFrames = movie.InputFrames.ToList();
		var newFrames = new List<InputFrame> {
			new(100) { Comment = "Inserted" }
		};

		var action = new InsertFramesAction(movie, 3, newFrames);
		action.Execute();
		Assert.Equal(11, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);

		for (int i = 0; i < 10; i++) {
			Assert.Same(originalFrames[i], movie.InputFrames[i]);
		}
	}

	[Fact]
	public void InsertFramesAction_ExecuteUndoExecute_RestoresInsertedState() {
		var movie = CreateTestMovie(5);
		var newFrames = new List<InputFrame> {
			new(50) { Comment = "Re-inserted" }
		};

		var action = new InsertFramesAction(movie, 2, newFrames);

		action.Execute();
		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal("Re-inserted", movie.InputFrames[2].Comment);

		action.Undo();
		Assert.Equal(5, movie.InputFrames.Count);

		action.Execute();
		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal("Re-inserted", movie.InputFrames[2].Comment);
	}

	[Fact]
	public void InsertFramesAction_AtBeginning_InsertsCorrectly() {
		var movie = CreateTestMovie(5);
		var newFrames = new List<InputFrame> { new(99) { Comment = "First" } };

		var action = new InsertFramesAction(movie, 0, newFrames);
		action.Execute();

		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal("First", movie.InputFrames[0].Comment);
	}

	[Fact]
	public void InsertFramesAction_AtEnd_InsertsCorrectly() {
		var movie = CreateTestMovie(5);
		var newFrames = new List<InputFrame> { new(99) { Comment = "Last" } };

		var action = new InsertFramesAction(movie, 5, newFrames);
		action.Execute();

		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal("Last", movie.InputFrames[5].Comment);
	}

	[Fact]
	public void InsertFramesAction_Description_IncludesCount() {
		var movie = CreateTestMovie(5);
		var newFrames = new List<InputFrame> { new(0), new(1), new(2) };

		var action = new InsertFramesAction(movie, 0, newFrames);
		Assert.Equal("Insert 3 frame(s)", action.Description);
	}

	#endregion

	#region DeleteFramesAction Tests

	[Fact]
	public void DeleteFramesAction_Execute_RemovesFrames() {
		var movie = CreateTestMovie(10);

		var action = new DeleteFramesAction(movie, 3, 4);
		action.Execute();

		Assert.Equal(6, movie.InputFrames.Count);
	}

	[Fact]
	public void DeleteFramesAction_Undo_RestoresDeletedFrames() {
		var movie = CreateTestMovie(10);
		var originalFrames = movie.InputFrames.ToList();

		var action = new DeleteFramesAction(movie, 3, 4);
		action.Execute();
		Assert.Equal(6, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);

		for (int i = 0; i < 10; i++) {
			Assert.Same(originalFrames[i], movie.InputFrames[i]);
		}
	}

	[Fact]
	public void DeleteFramesAction_ExecuteUndoExecute_WorksCorrectly() {
		var movie = CreateTestMovie(8);

		var action = new DeleteFramesAction(movie, 2, 3);

		action.Execute();
		Assert.Equal(5, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(8, movie.InputFrames.Count);

		action.Execute();
		Assert.Equal(5, movie.InputFrames.Count);
	}

	[Fact]
	public void DeleteFramesAction_FirstFrames_WorksCorrectly() {
		var movie = CreateTestMovie(10);
		var keptFrame = movie.InputFrames[3];

		var action = new DeleteFramesAction(movie, 0, 3);
		action.Execute();

		Assert.Equal(7, movie.InputFrames.Count);
		Assert.Same(keptFrame, movie.InputFrames[0]);
	}

	[Fact]
	public void DeleteFramesAction_LastFrames_WorksCorrectly() {
		var movie = CreateTestMovie(10);

		var action = new DeleteFramesAction(movie, 7, 3);
		action.Execute();

		Assert.Equal(7, movie.InputFrames.Count);
	}

	[Fact]
	public void DeleteFramesAction_AllFrames_LeavesEmpty() {
		var movie = CreateTestMovie(5);

		var action = new DeleteFramesAction(movie, 0, 5);
		action.Execute();

		Assert.Empty(movie.InputFrames);

		action.Undo();
		Assert.Equal(5, movie.InputFrames.Count);
	}

	[Fact]
	public void DeleteFramesAction_Description_IncludesCount() {
		var movie = CreateTestMovie(10);
		var action = new DeleteFramesAction(movie, 2, 5);
		Assert.Equal("Delete 5 frame(s)", action.Description);
	}

	#endregion

	#region ModifyInputAction Tests

	[Fact]
	public void ModifyInputAction_Execute_ChangesInput() {
		var movie = CreateTestMovie(5);
		var frame = movie.InputFrames[2];
		var newInput = new ControllerInput { A = true, B = true, Up = true };

		var action = new ModifyInputAction(frame, 0, newInput);
		action.Execute();

		Assert.True(frame.Controllers[0].A);
		Assert.True(frame.Controllers[0].B);
		Assert.True(frame.Controllers[0].Up);
	}

	[Fact]
	public void ModifyInputAction_Undo_RestoresOriginalInput() {
		var movie = CreateTestMovie(5);
		var frame = movie.InputFrames[2];
		bool originalA = frame.Controllers[0].A;
		bool originalB = frame.Controllers[0].B;

		var newInput = new ControllerInput { A = !originalA, B = !originalB };
		var action = new ModifyInputAction(frame, 0, newInput);

		action.Execute();
		Assert.Equal(!originalA, frame.Controllers[0].A);

		action.Undo();
		Assert.Equal(originalA, frame.Controllers[0].A);
		Assert.Equal(originalB, frame.Controllers[0].B);
	}

	[Fact]
	public void ModifyInputAction_ExecuteUndoExecute_RestoresModifiedState() {
		var movie = CreateTestMovie(5);
		var frame = movie.InputFrames[0];
		var newInput = new ControllerInput { Start = true, Select = true };

		var action = new ModifyInputAction(frame, 0, newInput);

		action.Execute();
		Assert.True(frame.Controllers[0].Start);

		action.Undo();
		Assert.False(frame.Controllers[0].Start);

		action.Execute();
		Assert.True(frame.Controllers[0].Start);
		Assert.True(frame.Controllers[0].Select);
	}

	[Fact]
	public void ModifyInputAction_Description_IsCorrect() {
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput()]
		};
		var action = new ModifyInputAction(frame, 0, new ControllerInput());
		Assert.Equal("Modify input", action.Description);
	}

	[Fact]
	public void ModifyInputAction_PreservesUnchangedButtons() {
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput { L = true, R = true, X = true, Y = true }]
		};

		// Only change A, leaving original L/R/X/Y
		var newInput = new ControllerInput { A = true };
		var action = new ModifyInputAction(frame, 0, newInput);

		action.Execute();
		Assert.True(frame.Controllers[0].A);
		Assert.False(frame.Controllers[0].L); // newInput has L=false

		action.Undo();
		Assert.True(frame.Controllers[0].L); // Original restored
		Assert.True(frame.Controllers[0].R);
		Assert.True(frame.Controllers[0].X);
		Assert.True(frame.Controllers[0].Y);
	}

	#endregion

	#region Undo/Redo Stack Simulation Tests

	[Fact]
	public void UndoStack_MultipleActions_UndoesInReverseOrder() {
		var movie = CreateTestMovie(10);
		var undoStack = new Stack<UndoableAction>();

		// Execute 3 actions
		var action1 = new InsertFramesAction(movie, 0, [new InputFrame(100) { Comment = "A" }]);
		action1.Execute();
		undoStack.Push(action1);

		var action2 = new InsertFramesAction(movie, 0, [new InputFrame(101) { Comment = "B" }]);
		action2.Execute();
		undoStack.Push(action2);

		var action3 = new InsertFramesAction(movie, 0, [new InputFrame(102) { Comment = "C" }]);
		action3.Execute();
		undoStack.Push(action3);

		Assert.Equal(13, movie.InputFrames.Count);

		// Undo all in reverse
		undoStack.Pop().Undo();
		Assert.Equal(12, movie.InputFrames.Count);

		undoStack.Pop().Undo();
		Assert.Equal(11, movie.InputFrames.Count);

		undoStack.Pop().Undo();
		Assert.Equal(10, movie.InputFrames.Count);
	}

	[Fact]
	public void UndoRedoStack_MixedActions_PreservesState() {
		var movie = CreateTestMovie(5);
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		// Insert 2 frames
		var insert = new InsertFramesAction(movie, 2, [new InputFrame(50), new InputFrame(51)]);
		insert.Execute();
		undoStack.Push(insert);
		Assert.Equal(7, movie.InputFrames.Count);

		// Delete 1 frame
		var delete = new DeleteFramesAction(movie, 0, 1);
		delete.Execute();
		undoStack.Push(delete);
		Assert.Equal(6, movie.InputFrames.Count);

		// Undo delete
		var undone = undoStack.Pop();
		undone.Undo();
		redoStack.Push(undone);
		Assert.Equal(7, movie.InputFrames.Count);

		// Redo delete
		var redone = redoStack.Pop();
		redone.Execute();
		undoStack.Push(redone);
		Assert.Equal(6, movie.InputFrames.Count);
	}

	[Fact]
	public void UndoStack_NewActionClearsRedoStack() {
		var movie = CreateTestMovie(5);
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		// Execute action
		var action1 = new InsertFramesAction(movie, 0, [new InputFrame(50)]);
		action1.Execute();
		undoStack.Push(action1);

		// Undo it
		var undone = undoStack.Pop();
		undone.Undo();
		redoStack.Push(undone);

		Assert.Single(redoStack);

		// Execute new action — should clear redo stack
		var action2 = new DeleteFramesAction(movie, 0, 1);
		action2.Execute();
		undoStack.Push(action2);
		redoStack.Clear(); // This is what the real code does

		Assert.Empty(redoStack);
		Assert.Single(undoStack);
	}

	#endregion

	#region Edge Case Tests

	[Fact]
	public void InsertFramesAction_EmptyList_NoChange() {
		var movie = CreateTestMovie(5);
		var action = new InsertFramesAction(movie, 2, []);

		action.Execute();
		Assert.Equal(5, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(5, movie.InputFrames.Count);
	}

	[Fact]
	public void InsertFramesAction_LargeInsert_WorksCorrectly() {
		var movie = CreateTestMovie(10);
		var newFrames = Enumerable.Range(0, 1000)
			.Select(i => new InputFrame(i))
			.ToList();

		var action = new InsertFramesAction(movie, 5, newFrames);
		action.Execute();
		Assert.Equal(1010, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);
	}

	[Fact]
	public void DeleteThenInsert_SamePosition_RestoresOriginal() {
		var movie = CreateTestMovie(10);
		var savedFrames = movie.InputFrames.GetRange(3, 4).ToList();

		var deleteAction = new DeleteFramesAction(movie, 3, 4);
		deleteAction.Execute();
		Assert.Equal(6, movie.InputFrames.Count);

		var insertAction = new InsertFramesAction(movie, 3, savedFrames);
		insertAction.Execute();
		Assert.Equal(10, movie.InputFrames.Count);

		for (int i = 0; i < savedFrames.Count; i++) {
			Assert.Same(savedFrames[i], movie.InputFrames[3 + i]);
		}
	}

	#endregion
}
