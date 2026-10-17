using Nexen.MovieConverter;
using Nexen.TAS;
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

		var action = new ModifyInputAction(frame, 2, 0, newInput);
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
		var action = new ModifyInputAction(frame, 2, 0, newInput);

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

		var action = new ModifyInputAction(frame, 0, 0, newInput);

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
		var action = new ModifyInputAction(frame, 0, 0, new ControllerInput());
		Assert.Equal("Modify input", action.Description);
	}

	[Fact]
	public void ModifyInputAction_PreservesUnchangedButtons() {
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput { L = true, R = true, X = true, Y = true }]
		};

		// Only change A, leaving original L/R/X/Y
		var newInput = new ControllerInput { A = true };
		var action = new ModifyInputAction(frame, 0, 0, newInput);

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

	#region ClearInputAction Tests

	[Fact]
	public void ClearInputAction_Execute_ClearsAllButtons() {
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput { A = true, B = true, Up = true, Start = true }]
		};

		var action = new ClearInputAction(frame, 0);
		action.Execute();

		Assert.False(frame.Controllers[0].A);
		Assert.False(frame.Controllers[0].B);
		Assert.False(frame.Controllers[0].Up);
		Assert.False(frame.Controllers[0].Start);
	}

	[Fact]
	public void ClearInputAction_Undo_RestoresAllOriginalButtons() {
		var frame = new InputFrame(0) {
			Controllers = [new ControllerInput { A = true, B = true, L = true, R = true, X = true, Y = true }]
		};

		var action = new ClearInputAction(frame, 0);
		action.Execute();

		Assert.False(frame.Controllers[0].A);

		action.Undo();

		Assert.True(frame.Controllers[0].A);
		Assert.True(frame.Controllers[0].B);
		Assert.True(frame.Controllers[0].L);
		Assert.True(frame.Controllers[0].R);
		Assert.True(frame.Controllers[0].X);
		Assert.True(frame.Controllers[0].Y);
	}

	[Fact]
	public void ClearInputAction_MultiController_ClearsAll() {
		var frame = new InputFrame(0) {
			Controllers = [
				new ControllerInput { A = true, B = true },
				new ControllerInput { Up = true, Down = true }
			]
		};

		var action = new ClearInputAction(frame, 0);
		action.Execute();

		Assert.False(frame.Controllers[0].A);
		Assert.False(frame.Controllers[0].B);
		Assert.False(frame.Controllers[1].Up);
		Assert.False(frame.Controllers[1].Down);

		action.Undo();

		Assert.True(frame.Controllers[0].A);
		Assert.True(frame.Controllers[1].Up);
	}

	#endregion

	#region PaintInputAction Tests

	[Fact]
	public void PaintInputAction_Execute_SetsButtonAcrossFrames() {
		var movie = CreateTestMovie(10);
		var indices = new List<int> { 0, 2, 4, 6, 8 };
		var oldInputs = indices.Select(i => new ControllerInput {
			A = movie.InputFrames[i].Controllers[0].A,
			B = movie.InputFrames[i].Controllers[0].B
		}).ToList();

		var action = new PaintInputAction(movie, indices, 0, "B", true, oldInputs);
		action.Execute();

		foreach (int i in indices) {
			Assert.True(movie.InputFrames[i].Controllers[0].B);
		}
	}

	[Fact]
	public void PaintInputAction_Undo_RestoresOriginalButtons() {
		var movie = CreateTestMovie(10);
		var indices = new List<int> { 1, 3, 5 };
		var oldInputs = indices.Select(i => new ControllerInput {
			A = movie.InputFrames[i].Controllers[0].A,
			B = movie.InputFrames[i].Controllers[0].B,
			Up = movie.InputFrames[i].Controllers[0].Up
		}).ToList();

		// Save original states
		bool[] originalA = indices.Select(i => movie.InputFrames[i].Controllers[0].A).ToArray();

		var action = new PaintInputAction(movie, indices, 0, "A", true, oldInputs);
		action.Execute();

		foreach (int i in indices) {
			Assert.True(movie.InputFrames[i].Controllers[0].A);
		}

		action.Undo();

		for (int j = 0; j < indices.Count; j++) {
			Assert.Equal(originalA[j], movie.InputFrames[indices[j]].Controllers[0].A);
		}
	}

	[Fact]
	public void PaintInputAction_AllButtons_WorkCorrectly() {
		var movie = CreateTestMovie(5);
		var indices = new List<int> { 0 };
		string[] buttons = ["A", "B", "X", "Y", "L", "R", "Up", "Down", "Left", "Right", "Start", "Select"];

		foreach (var button in buttons) {
			var oldInputs = new List<ControllerInput> { new() };
			var action = new PaintInputAction(movie, indices, 0, button, true, oldInputs);
			action.Execute();
		}

		var ctrl = movie.InputFrames[0].Controllers[0];
		Assert.True(ctrl.A);
		Assert.True(ctrl.B);
		Assert.True(ctrl.X);
		Assert.True(ctrl.Y);
		Assert.True(ctrl.L);
		Assert.True(ctrl.R);
		Assert.True(ctrl.Up);
		Assert.True(ctrl.Down);
		Assert.True(ctrl.Left);
		Assert.True(ctrl.Right);
		Assert.True(ctrl.Start);
		Assert.True(ctrl.Select);
	}

	[Fact]
	public void PaintInputAction_Description_IncludesFrameCount() {
		var movie = CreateTestMovie(5);
		var action = new PaintInputAction(movie, [0, 1, 2], 0, "A", true, [new(), new(), new()]);
		Assert.Equal("Paint 3 frame(s)", action.Description);
	}

	#endregion

	#region Multi-Action Chain Tests

	[Fact]
	public void MultiActionChain_InsertModifyPaintDelete_FullUndoCycle() {
		var movie = CreateTestMovie(10);
		var undoStack = new Stack<UndoableAction>();

		// 1. Insert 2 frames at position 5
		var insertFrames = new List<InputFrame> {
			new(50) { Controllers = [new ControllerInput()] },
			new(51) { Controllers = [new ControllerInput()] }
		};
		var insertAction = new InsertFramesAction(movie, 5, insertFrames);
		insertAction.Execute();
		undoStack.Push(insertAction);
		Assert.Equal(12, movie.InputFrames.Count);

		// 2. Modify frame 5 (one of the inserted frames)
		var newInput = new ControllerInput { A = true, Start = true };
		var modifyAction = new ModifyInputAction(movie.InputFrames[5], 5, 0, newInput);
		modifyAction.Execute();
		undoStack.Push(modifyAction);
		Assert.True(movie.InputFrames[5].Controllers[0].A);

		// 3. Paint B across frames 5-7
		var paintIndices = new List<int> { 5, 6, 7 };
		var paintOldInputs = paintIndices.Select(i => new ControllerInput {
			A = movie.InputFrames[i].Controllers[0].A,
			B = movie.InputFrames[i].Controllers[0].B
		}).ToList();
		var paintAction = new PaintInputAction(movie, paintIndices, 0, "B", true, paintOldInputs);
		paintAction.Execute();
		undoStack.Push(paintAction);
		Assert.True(movie.InputFrames[5].Controllers[0].B);
		Assert.True(movie.InputFrames[6].Controllers[0].B);
		Assert.True(movie.InputFrames[7].Controllers[0].B);

		// 4. Delete frame 0
		var deleteAction = new DeleteFramesAction(movie, 0, 1);
		deleteAction.Execute();
		undoStack.Push(deleteAction);
		Assert.Equal(11, movie.InputFrames.Count);

		// Undo all in reverse order
		undoStack.Pop().Undo(); // Undo delete
		Assert.Equal(12, movie.InputFrames.Count);

		undoStack.Pop().Undo(); // Undo paint
		// Frame 6 B should be restored to original
		Assert.False(movie.InputFrames[6].Controllers[0].B);

		undoStack.Pop().Undo(); // Undo modify
		Assert.False(movie.InputFrames[5].Controllers[0].A);

		undoStack.Pop().Undo(); // Undo insert
		Assert.Equal(10, movie.InputFrames.Count);
	}

	#endregion

	#region Undo/Redo State Machine Tests

	[Fact]
	public void UndoRedoStateMachine_InitialState_NothingAvailable() {
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		Assert.Empty(undoStack);
		Assert.Empty(redoStack);
	}

	[Fact]
	public void UndoRedoStateMachine_AfterExecute_CanUndoButNotRedo() {
		var movie = CreateTestMovie(5);
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		var action = new InsertFramesAction(movie, 0, [new InputFrame(50)]);
		action.Execute();
		undoStack.Push(action);
		redoStack.Clear();

		Assert.NotEmpty(undoStack);
		Assert.Empty(redoStack);
	}

	[Fact]
	public void UndoRedoStateMachine_AfterUndo_CanRedoButNotUndo() {
		var movie = CreateTestMovie(5);
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		var action = new InsertFramesAction(movie, 0, [new InputFrame(50)]);
		action.Execute();
		undoStack.Push(action);

		// Undo
		var undone = undoStack.Pop();
		undone.Undo();
		redoStack.Push(undone);

		Assert.Empty(undoStack);
		Assert.NotEmpty(redoStack);
	}

	[Fact]
	public void UndoRedoStateMachine_NewActionAfterUndo_ClearsRedo() {
		var movie = CreateTestMovie(5);
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		// Execute and undo
		var action1 = new InsertFramesAction(movie, 0, [new InputFrame(50)]);
		action1.Execute();
		undoStack.Push(action1);
		var undone = undoStack.Pop();
		undone.Undo();
		redoStack.Push(undone);

		Assert.Single(redoStack);

		// Execute new action - should clear redo
		var action2 = new InsertFramesAction(movie, 0, [new InputFrame(60)]);
		action2.Execute();
		undoStack.Push(action2);
		redoStack.Clear();

		Assert.Single(undoStack);
		Assert.Empty(redoStack);
	}

	[Fact]
	public void UndoRedoStateMachine_MultipleUndoRedo_MaintainsConsistency() {
		var movie = CreateTestMovie(5);
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		// Execute 3 actions
		for (int i = 0; i < 3; i++) {
			var action = new InsertFramesAction(movie, 0, [new InputFrame(100 + i)]);
			action.Execute();
			undoStack.Push(action);
			redoStack.Clear();
		}
		Assert.Equal(8, movie.InputFrames.Count);
		Assert.Equal(3, undoStack.Count);

		// Undo 2
		for (int i = 0; i < 2; i++) {
			var undone = undoStack.Pop();
			undone.Undo();
			redoStack.Push(undone);
		}
		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Single(undoStack);
		Assert.Equal(2, redoStack.Count);

		// Redo 1
		var redone = redoStack.Pop();
		redone.Execute();
		undoStack.Push(redone);
		Assert.Equal(7, movie.InputFrames.Count);
		Assert.Equal(2, undoStack.Count);
		Assert.Single(redoStack);

		// New action invalidates remaining redo
		var fresh = new DeleteFramesAction(movie, 0, 1);
		fresh.Execute();
		undoStack.Push(fresh);
		redoStack.Clear();
		Assert.Equal(6, movie.InputFrames.Count);
		Assert.Equal(3, undoStack.Count);
		Assert.Empty(redoStack);
	}

	[Fact]
	public void UndoRedoStateMachine_FullCycle_RestoresOriginalState() {
		var movie = CreateTestMovie(10);
		var undoStack = new Stack<UndoableAction>();
		var redoStack = new Stack<UndoableAction>();

		// Snapshot original
		int originalCount = movie.InputFrames.Count;
		bool[] originalA = movie.InputFrames.Select(f => f.Controllers[0].A).ToArray();

		// Execute insert, modify, delete
		var insert = new InsertFramesAction(movie, 3, [new InputFrame(99) { Controllers = [new ControllerInput()] }]);
		insert.Execute();
		undoStack.Push(insert);

		var modify = new ModifyInputAction(movie.InputFrames[0], 0, 0, new ControllerInput { A = true, B = true });
		modify.Execute();
		undoStack.Push(modify);

		var delete = new DeleteFramesAction(movie, 5, 2);
		delete.Execute();
		undoStack.Push(delete);

		// Undo all
		while (undoStack.Count > 0) {
			var action = undoStack.Pop();
			action.Undo();
			redoStack.Push(action);
		}

		// Verify original state
		Assert.Equal(originalCount, movie.InputFrames.Count);
		for (int i = 0; i < originalCount; i++) {
			Assert.Equal(originalA[i], movie.InputFrames[i].Controllers[0].A);
		}

		// Redo all
		while (redoStack.Count > 0) {
			var action = redoStack.Pop();
			action.Execute();
			undoStack.Push(action);
		}

		// Verify modified state
		Assert.Equal(originalCount + 1 - 2, movie.InputFrames.Count);
	}

	#endregion

	#region RecordingUndoAction Tests

	[Fact]
	public void RecordingUndoAction_AppendMode_UndoRemovesAppendedFrames() {
		var movie = CreateTestMovie(10);
		int startIndex = movie.InputFrames.Count;

		// Simulate recording 5 frames in append mode
		for (int i = 0; i < 5; i++) {
			movie.InputFrames.Add(new InputFrame(100 + i) { Comment = $"Recorded{i}" });
		}

		Assert.Equal(15, movie.InputFrames.Count);

		var action = new RecordingUndoAction(movie, RecordingMode.Append, startIndex, 5);
		action.Undo();

		Assert.Equal(10, movie.InputFrames.Count);
	}

	[Fact]
	public void RecordingUndoAction_InsertMode_UndoRemovesInsertedFrames() {
		var movie = CreateTestMovie(10);
		int insertAt = 5;

		// Simulate recording 3 frames in insert mode
		for (int i = 0; i < 3; i++) {
			movie.InputFrames.Insert(insertAt + i, new InputFrame(100 + i) { Comment = $"Inserted{i}" });
		}

		Assert.Equal(13, movie.InputFrames.Count);

		var action = new RecordingUndoAction(movie, RecordingMode.Insert, insertAt, 3);
		action.Undo();

		Assert.Equal(10, movie.InputFrames.Count);
	}

	[Fact]
	public void RecordingUndoAction_OverwriteMode_UndoRestoresOriginalFrames() {
		var movie = CreateTestMovie(10);
		int overwriteAt = 3;

		// Clone original frames before overwrite
		var originals = new List<InputFrame>();
		for (int i = overwriteAt; i < overwriteAt + 3 && i < movie.InputFrames.Count; i++) {
			originals.Add(movie.InputFrames[i].Clone());
		}

		// Simulate overwriting 3 frames
		for (int i = 0; i < 3; i++) {
			movie.InputFrames[overwriteAt + i] = new InputFrame(200 + i) { Comment = $"Overwritten{i}" };
		}

		Assert.Equal("Overwritten0", movie.InputFrames[3].Comment);

		var action = new RecordingUndoAction(movie, RecordingMode.Overwrite, overwriteAt, 3, originals);
		action.Undo();

		Assert.Equal(10, movie.InputFrames.Count);
		Assert.Equal(originals[0].Controllers[0].A, movie.InputFrames[3].Controllers[0].A);
		Assert.Null(movie.InputFrames[3].Comment);
	}

	[Fact]
	public void RecordingUndoAction_OverwriteExtendsPastEnd_UndoTrimsToo() {
		var movie = CreateTestMovie(10);
		int overwriteAt = 8;

		// Clone the 2 frames that exist
		var originals = new List<InputFrame> {
			movie.InputFrames[8].Clone(),
			movie.InputFrames[9].Clone()
		};

		// Overwrite the 2 existing + append 1 extra
		movie.InputFrames[8] = new InputFrame(200) { Comment = "OW1" };
		movie.InputFrames[9] = new InputFrame(201) { Comment = "OW2" };
		movie.InputFrames.Add(new InputFrame(202) { Comment = "OW3" });

		Assert.Equal(11, movie.InputFrames.Count);

		var action = new RecordingUndoAction(movie, RecordingMode.Overwrite, overwriteAt, 3, originals);
		action.Undo();

		Assert.Equal(10, movie.InputFrames.Count);
		Assert.Null(movie.InputFrames[8].Comment);
		Assert.Null(movie.InputFrames[9].Comment);
	}

	[Fact]
	public void RecordingUndoAction_Description_IncludesModeAndCount() {
		var movie = CreateTestMovie(5);
		var action = new RecordingUndoAction(movie, RecordingMode.Append, 5, 10);
		Assert.Equal("Record 10 frame(s) (Append)", action.Description);

		var action2 = new RecordingUndoAction(movie, RecordingMode.Insert, 3, 2);
		Assert.Equal("Record 2 frame(s) (Insert)", action2.Description);
	}

	[Fact]
	public void RecordingUndoAction_ZeroFrames_NoChange() {
		var movie = CreateTestMovie(10);
		var action = new RecordingUndoAction(movie, RecordingMode.Append, 10, 0);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);
	}

	#endregion

	#region RerecordUndoAction Tests

	[Fact]
	public void RerecordUndoAction_Execute_TruncatesMovie() {
		var movie = CreateTestMovie(10);
		int truncateFrom = 5;
		var removedFrames = movie.InputFrames.GetRange(truncateFrom, 5).ToList();

		// Simulate the truncation (already done by RerecordFrom)
		movie.InputFrames.RemoveRange(truncateFrom, 5);
		Assert.Equal(5, movie.InputFrames.Count);

		var action = new RerecordUndoAction(movie, truncateFrom, removedFrames);

		// Undo restores
		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);

		// Redo (Execute) truncates again
		action.Execute();
		Assert.Equal(5, movie.InputFrames.Count);
	}

	[Fact]
	public void RerecordUndoAction_Undo_RestoresRemovedFrames() {
		var movie = CreateTestMovie(10);
		int truncateFrom = 3;
		var removedFrames = movie.InputFrames.GetRange(truncateFrom, 7).ToList();

		movie.InputFrames.RemoveRange(truncateFrom, 7);
		Assert.Equal(3, movie.InputFrames.Count);

		var action = new RerecordUndoAction(movie, truncateFrom, removedFrames);
		action.Undo();

		Assert.Equal(10, movie.InputFrames.Count);
		for (int i = 0; i < removedFrames.Count; i++) {
			Assert.Same(removedFrames[i], movie.InputFrames[truncateFrom + i]);
		}
	}

	[Fact]
	public void RerecordUndoAction_UndoRedo_Roundtrips() {
		var movie = CreateTestMovie(10);
		int truncateFrom = 6;
		var removedFrames = movie.InputFrames.GetRange(truncateFrom, 4).ToList();

		movie.InputFrames.RemoveRange(truncateFrom, 4);
		Assert.Equal(6, movie.InputFrames.Count);

		var action = new RerecordUndoAction(movie, truncateFrom, removedFrames);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);

		action.Execute();
		Assert.Equal(6, movie.InputFrames.Count);

		action.Undo();
		Assert.Equal(10, movie.InputFrames.Count);
	}

	[Fact]
	public void RerecordUndoAction_Description_IncludesDetails() {
		var movie = CreateTestMovie(10);
		var removedFrames = movie.InputFrames.GetRange(5, 5).ToList();
		var action = new RerecordUndoAction(movie, 5, removedFrames);
		Assert.Equal("Rerecord from frame 5 (removed 5 frame(s))", action.Description);
	}

	[Fact]
	public void RerecordUndoAction_TruncateAll_RestoresAll() {
		var movie = CreateTestMovie(10);
		var removedFrames = movie.InputFrames.GetRange(0, 10).ToList();

		movie.InputFrames.RemoveRange(0, 10);
		Assert.Empty(movie.InputFrames);

		var action = new RerecordUndoAction(movie, 0, removedFrames);
		action.Undo();

		Assert.Equal(10, movie.InputFrames.Count);
	}

	#endregion

	#region GetEarliestAffectedFrame Tests for New Actions

	[Fact]
	public void GetEarliestAffectedFrame_RecordingUndoAction_ReturnsStartIndex() {
		var movie = CreateTestMovie(10);
		var action = new RecordingUndoAction(movie, RecordingMode.Append, 7, 3);
		Assert.Equal(7, TasEditorViewModel.GetEarliestAffectedFrame(action));
	}

	[Fact]
	public void GetEarliestAffectedFrame_RerecordUndoAction_ReturnsTruncateFrame() {
		var movie = CreateTestMovie(10);
		var removedFrames = movie.InputFrames.GetRange(4, 6).ToList();
		var action = new RerecordUndoAction(movie, 4, removedFrames);
		Assert.Equal(4, TasEditorViewModel.GetEarliestAffectedFrame(action));
	}

	#endregion

	#region Thread Safety Guard Tests

	private static void SetMovieOnViewModel(TasEditorViewModel vm, MovieData movie) {
		var prop = typeof(TasEditorViewModel).GetProperty(nameof(TasEditorViewModel.Movie));
		prop!.SetValue(vm, movie);
		vm.Recorder.Movie = movie;
	}

	[Fact]
	public void ExecuteAction_WhenRecording_BlocksEditing() {
		var vm = new TasEditorViewModel();
		SetMovieOnViewModel(vm, CreateTestMovie(10));
		vm.IsRecording = true;

		int originalCount = vm.Movie!.InputFrames.Count;
		var insertAction = new InsertFramesAction(vm.Movie, 0, [new InputFrame(99)]);

		vm.ExecuteAction(insertAction);

		// Should be blocked — frame count unchanged
		Assert.Equal(originalCount, vm.Movie.InputFrames.Count);
	}

	[Fact]
	public void ExecuteAction_WhenNotRecording_AllowsEditing() {
		var vm = new TasEditorViewModel();
		SetMovieOnViewModel(vm, CreateTestMovie(10));
		vm.IsRecording = false;

		var insertAction = new InsertFramesAction(vm.Movie!, 0, [new InputFrame(99)]);

		vm.ExecuteAction(insertAction);

		Assert.Equal(11, vm.Movie!.InputFrames.Count);
	}

	#endregion
}
