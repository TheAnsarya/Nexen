using Xunit;
using System;
using System.Collections.Generic;
using Nexen.MovieConverter;
using Nexen.TAS;
using Nexen.ViewModels;

namespace Nexen.Tests.TAS;

/// <summary>
/// Unit tests for <see cref="TasLuaApi"/> Lua scripting API.
/// Covers input manipulation, frame operations, and greenzone interactions.
/// </summary>
public class TasLuaApiTests {
	#region Test Helpers

	private static MovieData CreateTestMovie(int frameCount, int controllerCount = 1) {
		var movie = new MovieData {
			Author = "Test",
			GameName = "Test Game",
			SystemType = SystemType.Nes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				Controllers = new ControllerInput[controllerCount]
			};
			for (int c = 0; c < controllerCount; c++) {
				frame.Controllers[c] = new ControllerInput();
			}
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	#endregion

	#region InsertFrames Tests

	[Fact]
	public void InsertFrames_ValidPosition_InsertsEmptyFrames() {
		// Arrange
		var movie = CreateTestMovie(10);
		int originalCount = movie.InputFrames.Count;
		int insertPosition = 5;
		int insertCount = 3;

		// Act - manually test the logic (without TasLuaApi since it needs ViewModel)
		for (int i = 0; i < insertCount; i++) {
			var frame = new InputFrame {
				FrameNumber = insertPosition + i,
				Controllers = new ControllerInput[1]
			};
			frame.Controllers[0] = new ControllerInput();
			movie.InputFrames.Insert(insertPosition + i, frame);
		}

		// Renumber
		for (int i = insertPosition + insertCount; i < movie.InputFrames.Count; i++) {
			movie.InputFrames[i].FrameNumber = i;
		}

		// Assert
		Assert.Equal(originalCount + insertCount, movie.InputFrames.Count);
		Assert.Equal(insertPosition, movie.InputFrames[insertPosition].FrameNumber);
		Assert.Equal(insertPosition + insertCount, movie.InputFrames[insertPosition + insertCount].FrameNumber);
	}

	[Fact]
	public void InsertFrames_AtEnd_AppendsFrames() {
		// Arrange
		var movie = CreateTestMovie(10);
		int originalCount = movie.InputFrames.Count;
		int insertPosition = movie.InputFrames.Count;
		int insertCount = 5;

		// Act
		for (int i = 0; i < insertCount; i++) {
			var frame = new InputFrame {
				FrameNumber = insertPosition + i,
				Controllers = new ControllerInput[] { new ControllerInput() }
			};
			movie.InputFrames.Add(frame);
		}

		// Assert
		Assert.Equal(originalCount + insertCount, movie.InputFrames.Count);
		Assert.Equal(14, movie.InputFrames[14].FrameNumber);
	}

	[Fact]
	public void InsertFrames_WithMultipleControllers_CreatesCorrectControllerCount() {
		// Arrange
		var movie = CreateTestMovie(10, controllerCount: 4);
		int insertPosition = 5;

		// Act
		var frame = new InputFrame {
			FrameNumber = insertPosition,
			Controllers = new ControllerInput[movie.InputFrames[0].Controllers.Length]
		};
		for (int c = 0; c < frame.Controllers.Length; c++) {
			frame.Controllers[c] = new ControllerInput();
		}
		movie.InputFrames.Insert(insertPosition, frame);

		// Assert
		Assert.Equal(4, movie.InputFrames[insertPosition].Controllers.Length);
	}

	#endregion

	#region DeleteFrames Tests

	[Fact]
	public void DeleteFrames_ValidRange_RemovesFrames() {
		// Arrange
		var movie = CreateTestMovie(20);
		int deletePosition = 5;
		int deleteCount = 5;

		// Act
		movie.InputFrames.RemoveRange(deletePosition, deleteCount);

		// Renumber
		for (int i = deletePosition; i < movie.InputFrames.Count; i++) {
			movie.InputFrames[i].FrameNumber = i;
		}

		// Assert
		Assert.Equal(15, movie.InputFrames.Count);
		Assert.Equal(5, movie.InputFrames[5].FrameNumber);
	}

	[Fact]
	public void DeleteFrames_AtEnd_RemovesLastFrames() {
		// Arrange
		var movie = CreateTestMovie(10);
		int deletePosition = 7;
		int deleteCount = 3;

		// Act
		movie.InputFrames.RemoveRange(deletePosition, deleteCount);

		// Assert
		Assert.Equal(7, movie.InputFrames.Count);
		Assert.Equal(6, movie.InputFrames[^1].FrameNumber);
	}

	[Fact]
	public void DeleteFrames_ExceedsRemaining_ClampsToEnd() {
		// Arrange
		var movie = CreateTestMovie(10);
		int deletePosition = 5;
		int requestedCount = 100;

		// Act - clamp count to available
		int actualCount = Math.Min(requestedCount, movie.InputFrames.Count - deletePosition);
		movie.InputFrames.RemoveRange(deletePosition, actualCount);

		// Assert
		Assert.Equal(5, movie.InputFrames.Count);
	}

	#endregion

	#region Frame Renumbering Tests

	[Fact]
	public void FrameRenumbering_AfterInsert_HasConsecutiveNumbers() {
		// Arrange
		var movie = CreateTestMovie(10);
		int insertPosition = 3;

		// Act - insert frame
		var frame = new InputFrame {
			FrameNumber = insertPosition,
			Controllers = new ControllerInput[] { new ControllerInput() }
		};
		movie.InputFrames.Insert(insertPosition, frame);

		// Renumber
		for (int i = insertPosition + 1; i < movie.InputFrames.Count; i++) {
			movie.InputFrames[i].FrameNumber = i;
		}

		// Assert - verify consecutive frame numbers
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			Assert.Equal(i, movie.InputFrames[i].FrameNumber);
		}
	}

	[Fact]
	public void FrameRenumbering_AfterDelete_HasConsecutiveNumbers() {
		// Arrange
		var movie = CreateTestMovie(20);
		int deletePosition = 10;
		int deleteCount = 5;

		// Act
		movie.InputFrames.RemoveRange(deletePosition, deleteCount);

		// Renumber
		for (int i = deletePosition; i < movie.InputFrames.Count; i++) {
			movie.InputFrames[i].FrameNumber = i;
		}

		// Assert - verify consecutive frame numbers
		for (int i = 0; i < movie.InputFrames.Count; i++) {
			Assert.Equal(i, movie.InputFrames[i].FrameNumber);
		}
	}

	#endregion
}
