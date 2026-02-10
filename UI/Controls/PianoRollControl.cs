using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Media;
using Avalonia.Threading;
using Nexen.MovieConverter;

namespace Nexen.Controls;

/// <summary>
/// Piano roll style control for visualizing and editing TAS input.
/// Displays a timeline with button lanes, allowing visual input editing.
/// </summary>
public sealed class PianoRollControl : Control {
	#region Dependency Properties

	/// <summary>Identifies the Frames property.</summary>
	public static readonly StyledProperty<IReadOnlyList<InputFrame>?> FramesProperty =
		AvaloniaProperty.Register<PianoRollControl, IReadOnlyList<InputFrame>?>(nameof(Frames));

	/// <summary>Identifies the SelectedFrame property.</summary>
	public static readonly StyledProperty<int> SelectedFrameProperty =
		AvaloniaProperty.Register<PianoRollControl, int>(nameof(SelectedFrame), -1);

	/// <summary>Identifies the SelectionStart property.</summary>
	public static readonly StyledProperty<int> SelectionStartProperty =
		AvaloniaProperty.Register<PianoRollControl, int>(nameof(SelectionStart), -1);

	/// <summary>Identifies the SelectionEnd property.</summary>
	public static readonly StyledProperty<int> SelectionEndProperty =
		AvaloniaProperty.Register<PianoRollControl, int>(nameof(SelectionEnd), -1);

	/// <summary>Identifies the ScrollOffset property.</summary>
	public static readonly StyledProperty<int> ScrollOffsetProperty =
		AvaloniaProperty.Register<PianoRollControl, int>(nameof(ScrollOffset), 0);

	/// <summary>Identifies the ZoomLevel property.</summary>
	public static readonly StyledProperty<double> ZoomLevelProperty =
		AvaloniaProperty.Register<PianoRollControl, double>(nameof(ZoomLevel), 1.0);

	/// <summary>Identifies the PlaybackFrame property.</summary>
	public static readonly StyledProperty<int> PlaybackFrameProperty =
		AvaloniaProperty.Register<PianoRollControl, int>(nameof(PlaybackFrame), -1);

	/// <summary>Identifies the GreenzoneStart property.</summary>
	public static readonly StyledProperty<int> GreenzoneStartProperty =
		AvaloniaProperty.Register<PianoRollControl, int>(nameof(GreenzoneStart), 0);

	/// <summary>Identifies the ButtonLabels property.</summary>
	public static readonly StyledProperty<IReadOnlyList<string>?> ButtonLabelsProperty =
		AvaloniaProperty.Register<PianoRollControl, IReadOnlyList<string>?>(nameof(ButtonLabels));

	#endregion

	#region Properties

	/// <summary>Gets or sets the input frames to display.</summary>
	public IReadOnlyList<InputFrame>? Frames {
		get => GetValue(FramesProperty);
		set => SetValue(FramesProperty, value);
	}

	/// <summary>Gets or sets the currently selected frame.</summary>
	public int SelectedFrame {
		get => GetValue(SelectedFrameProperty);
		set => SetValue(SelectedFrameProperty, value);
	}

	/// <summary>Gets or sets the selection range start.</summary>
	public int SelectionStart {
		get => GetValue(SelectionStartProperty);
		set => SetValue(SelectionStartProperty, value);
	}

	/// <summary>Gets or sets the selection range end.</summary>
	public int SelectionEnd {
		get => GetValue(SelectionEndProperty);
		set => SetValue(SelectionEndProperty, value);
	}

	/// <summary>Gets or sets the scroll offset (first visible frame).</summary>
	public int ScrollOffset {
		get => GetValue(ScrollOffsetProperty);
		set => SetValue(ScrollOffsetProperty, value);
	}

	/// <summary>Gets or sets the zoom level (1.0 = default).</summary>
	public double ZoomLevel {
		get => GetValue(ZoomLevelProperty);
		set => SetValue(ZoomLevelProperty, Math.Clamp(value, 0.25, 4.0));
	}

	/// <summary>Gets or sets the current playback frame.</summary>
	public int PlaybackFrame {
		get => GetValue(PlaybackFrameProperty);
		set => SetValue(PlaybackFrameProperty, value);
	}

	/// <summary>Gets or sets the greenzone start frame.</summary>
	public int GreenzoneStart {
		get => GetValue(GreenzoneStartProperty);
		set => SetValue(GreenzoneStartProperty, value);
	}

	/// <summary>Gets or sets the button labels for the lanes.</summary>
	public IReadOnlyList<string>? ButtonLabels {
		get => GetValue(ButtonLabelsProperty);
		set => SetValue(ButtonLabelsProperty, value);
	}

	#endregion

	#region Events

	/// <summary>Occurs when a cell is clicked.</summary>
	public event EventHandler<PianoRollCellEventArgs>? CellClicked;

	/// <summary>Occurs when cells are painted (dragged).</summary>
	public event EventHandler<PianoRollPaintEventArgs>? CellsPainted;

	/// <summary>Occurs when the selection changes.</summary>
	public event EventHandler<PianoRollSelectionEventArgs>? SelectionChanged;

	/// <summary>Occurs when a marker is clicked.</summary>
	public event EventHandler<PianoRollMarkerEventArgs>? MarkerClicked;

	#endregion

	#region Constants

	private const int HeaderHeight = 24;
	private const int LaneHeaderWidth = 50;
	private const int DefaultCellWidth = 16;
	private const int DefaultCellHeight = 20;

	#endregion

	#region Private Fields

	private readonly List<string> _defaultButtons = new() { "A", "B", "X", "Y", "L", "R", "↑", "↓", "←", "→", "ST", "SE" };
	private bool _isPainting;
	private bool _paintValue;
	private int _paintButton;
	private int _paintStartFrame;
	private readonly HashSet<(int frame, int button)> _paintedCells = new();

	#endregion

	static PianoRollControl() {
		AffectsRender<PianoRollControl>(
			FramesProperty,
			SelectedFrameProperty,
			SelectionStartProperty,
			SelectionEndProperty,
			ScrollOffsetProperty,
			ZoomLevelProperty,
			PlaybackFrameProperty,
			GreenzoneStartProperty,
			ButtonLabelsProperty
		);
	}

	public PianoRollControl() {
		ClipToBounds = true;
		Focusable = true;
	}

	#region Rendering

	public override void Render(DrawingContext context) {
		base.Render(context);

		var bounds = Bounds;
		var buttons = ButtonLabels ?? _defaultButtons;
		int cellWidth = (int)(DefaultCellWidth * ZoomLevel);
		int cellHeight = (int)(DefaultCellHeight * ZoomLevel);

		// Background
		context.FillRectangle(Brushes.White, bounds);

		// Draw lane headers (button labels)
		DrawLaneHeaders(context, buttons, cellHeight);

		// Draw frame headers
		DrawFrameHeaders(context, bounds, cellWidth);

		// Draw grid and cells
		DrawGrid(context, bounds, buttons, cellWidth, cellHeight);

		// Draw selection
		DrawSelection(context, bounds, cellWidth, cellHeight, buttons.Count);

		// Draw playback position
		DrawPlaybackPosition(context, bounds, cellWidth);

		// Draw markers
		DrawMarkers(context, bounds, cellWidth);
	}

	private void DrawLaneHeaders(DrawingContext context, IReadOnlyList<string> buttons, int cellHeight) {
		var headerBrush = new SolidColorBrush(Color.FromRgb(240, 240, 240));
		var textBrush = Brushes.Black;
		var typeface = new Typeface("Segoe UI", FontStyle.Normal, FontWeight.Bold);

		for (int i = 0; i < buttons.Count; i++) {
			var rect = new Rect(0, HeaderHeight + i * cellHeight, LaneHeaderWidth, cellHeight);
			context.FillRectangle(headerBrush, rect);
			context.DrawRectangle(new Pen(Brushes.Gray, 1), rect);

			var text = new FormattedText(
				buttons[i],
				System.Globalization.CultureInfo.CurrentCulture,
				FlowDirection.LeftToRight,
				typeface,
				10 * ZoomLevel,
				textBrush
			);

			var textX = rect.X + (rect.Width - text.Width) / 2;
			var textY = rect.Y + (rect.Height - text.Height) / 2;
			context.DrawText(text, new Point(textX, textY));
		}
	}

	private void DrawFrameHeaders(DrawingContext context, Rect bounds, int cellWidth) {
		var headerBrush = new SolidColorBrush(Color.FromRgb(240, 240, 240));
		var textBrush = Brushes.Black;
		var greenzoneBrush = new SolidColorBrush(Color.FromRgb(200, 255, 200));
		var typeface = new Typeface("Segoe UI");

		int visibleFrames = (int)((bounds.Width - LaneHeaderWidth) / cellWidth) + 1;

		for (int i = 0; i < visibleFrames; i++) {
			int frame = ScrollOffset + i;
			var rect = new Rect(LaneHeaderWidth + i * cellWidth, 0, cellWidth, HeaderHeight);

			var bgBrush = frame >= GreenzoneStart ? greenzoneBrush : headerBrush;
			context.FillRectangle(bgBrush, rect);
			context.DrawRectangle(new Pen(Brushes.Gray, 1), rect);

			// Only show frame numbers at intervals
			if (frame % 10 == 0 || ZoomLevel >= 2.0) {
				var text = new FormattedText(
					frame.ToString(),
					System.Globalization.CultureInfo.CurrentCulture,
					FlowDirection.LeftToRight,
					typeface,
					8 * ZoomLevel,
					textBrush
				);

				var textX = rect.X + (rect.Width - text.Width) / 2;
				context.DrawText(text, new Point(textX, (HeaderHeight - text.Height) / 2));
			}
		}
	}

	private void DrawGrid(DrawingContext context, Rect bounds, IReadOnlyList<string> buttons, int cellWidth, int cellHeight) {
		var frames = Frames;

		if (frames is null) {
			return;
		}

		int visibleFrames = (int)((bounds.Width - LaneHeaderWidth) / cellWidth) + 1;
		var greenzoneBrush = new SolidColorBrush(Color.FromRgb(200, 255, 200));
		var lagBrush = new SolidColorBrush(Color.FromRgb(255, 200, 200));
		var pressedBrush = new SolidColorBrush(Color.FromRgb(100, 150, 255));
		var gridPen = new Pen(Brushes.LightGray, 0.5);

		for (int i = 0; i < visibleFrames; i++) {
			int frameIndex = ScrollOffset + i;

			if (frameIndex >= frames.Count) {
				break;
			}

			var frame = frames[frameIndex];
			bool isGreenzone = frameIndex >= GreenzoneStart;

			for (int j = 0; j < buttons.Count; j++) {
				var rect = new Rect(LaneHeaderWidth + i * cellWidth, HeaderHeight + j * cellHeight, cellWidth, cellHeight);

				// Background color
				IBrush? bgBrush = null;

				if (frame.IsLagFrame) {
					bgBrush = lagBrush;
				} else if (isGreenzone) {
					bgBrush = greenzoneBrush;
				}

				if (bgBrush is not null) {
					context.FillRectangle(bgBrush, rect);
				}

				// Check if button is pressed
				if (frame.Controllers.Length > 0) {
					bool isPressed = GetButtonState(frame.Controllers[0], j, buttons);

					if (isPressed) {
						var innerRect = rect.Deflate(2);
						context.FillRectangle(pressedBrush, innerRect);
					}
				}

				// Grid lines
				context.DrawRectangle(gridPen, rect);
			}
		}
	}

	private void DrawSelection(DrawingContext context, Rect bounds, int cellWidth, int cellHeight, int buttonCount) {
		if (SelectionStart < 0 || SelectionEnd < 0) {
			return;
		}

		int start = Math.Min(SelectionStart, SelectionEnd);
		int end = Math.Max(SelectionStart, SelectionEnd);

		int startX = LaneHeaderWidth + (start - ScrollOffset) * cellWidth;
		int endX = LaneHeaderWidth + (end - ScrollOffset + 1) * cellWidth;
		int height = buttonCount * cellHeight;

		var selectionBrush = new SolidColorBrush(Color.FromArgb(50, 0, 100, 255));
		var selectionPen = new Pen(Brushes.Blue, 2);

		var rect = new Rect(startX, HeaderHeight, endX - startX, height);
		context.FillRectangle(selectionBrush, rect);
		context.DrawRectangle(selectionPen, rect);
	}

	private void DrawPlaybackPosition(DrawingContext context, Rect bounds, int cellWidth) {
		if (PlaybackFrame < 0) {
			return;
		}

		int x = LaneHeaderWidth + (PlaybackFrame - ScrollOffset) * cellWidth + cellWidth / 2;

		if (x < LaneHeaderWidth || x > bounds.Width) {
			return;
		}

		var pen = new Pen(Brushes.Red, 2);
		context.DrawLine(pen, new Point(x, 0), new Point(x, bounds.Height));
	}

	private void DrawMarkers(DrawingContext context, Rect bounds, int cellWidth) {
		var frames = Frames;

		if (frames is null) {
			return;
		}

		var markerBrush = Brushes.Orange;
		int visibleFrames = (int)((bounds.Width - LaneHeaderWidth) / cellWidth) + 1;

		for (int i = 0; i < visibleFrames; i++) {
			int frameIndex = ScrollOffset + i;

			if (frameIndex >= frames.Count) {
				break;
			}

			var frame = frames[frameIndex];

			if (!string.IsNullOrEmpty(frame.Comment)) {
				int x = LaneHeaderWidth + i * cellWidth + cellWidth / 2;
				context.FillRectangle(markerBrush, new Rect(x - 3, 2, 6, HeaderHeight - 4));
			}
		}
	}

	private static bool GetButtonState(ControllerInput input, int buttonIndex, IReadOnlyList<string> buttons) {
		if (buttonIndex >= buttons.Count) {
			return false;
		}

		string button = buttons[buttonIndex];

		return button switch {
			"A" => input.A,
			"B" => input.B,
			"X" => input.X,
			"Y" => input.Y,
			"L" => input.L,
			"R" => input.R,
			"↑" or "UP" => input.Up,
			"↓" or "DOWN" => input.Down,
			"←" or "LEFT" => input.Left,
			"→" or "RIGHT" => input.Right,
			"ST" or "START" => input.Start,
			"SE" or "SELECT" => input.Select,
			_ => false
		};
	}

	#endregion

	#region Input Handling

	protected override void OnPointerPressed(PointerPressedEventArgs e) {
		base.OnPointerPressed(e);
		Focus();

		var pos = e.GetPosition(this);
		var (frame, button) = GetCellFromPosition(pos);

		if (frame < 0 || button < 0) {
			return;
		}

		if (e.GetCurrentPoint(this).Properties.IsLeftButtonPressed) {
			// Start painting
			_isPainting = true;
			_paintStartFrame = frame;
			_paintButton = button;
			_paintedCells.Clear();

			// Determine paint value (toggle current state)
			_paintValue = !GetCurrentButtonState(frame, button);
			_paintedCells.Add((frame, button));

			// Raise click event
			CellClicked?.Invoke(this, new PianoRollCellEventArgs(frame, button, _paintValue));

			e.Handled = true;
		} else if (e.GetCurrentPoint(this).Properties.IsRightButtonPressed) {
			// Right-click for selection
			SelectionStart = frame;
			SelectionEnd = frame;
			SelectionChanged?.Invoke(this, new PianoRollSelectionEventArgs(frame, frame));
		}
	}

	protected override void OnPointerMoved(PointerEventArgs e) {
		base.OnPointerMoved(e);

		if (!_isPainting) {
			return;
		}

		var pos = e.GetPosition(this);
		var (frame, button) = GetCellFromPosition(pos);

		if (frame < 0) {
			return;
		}

		// Paint along the same button lane
		if (button == _paintButton && !_paintedCells.Contains((frame, button))) {
			_paintedCells.Add((frame, button));
			CellClicked?.Invoke(this, new PianoRollCellEventArgs(frame, button, _paintValue));
		}
	}

	protected override void OnPointerReleased(PointerReleasedEventArgs e) {
		base.OnPointerReleased(e);

		if (_isPainting && _paintedCells.Count > 1) {
			// Raise paint event for multiple cells
			CellsPainted?.Invoke(this, new PianoRollPaintEventArgs(
				_paintedCells.Select(c => c.frame).ToList(),
				_paintButton,
				_paintValue
			));
		}

		_isPainting = false;
		_paintedCells.Clear();
	}

	protected override void OnPointerWheelChanged(PointerWheelEventArgs e) {
		base.OnPointerWheelChanged(e);

		if (e.KeyModifiers.HasFlag(KeyModifiers.Control)) {
			// Zoom with Ctrl+Wheel
			double delta = e.Delta.Y > 0 ? 0.25 : -0.25;
			ZoomLevel = Math.Clamp(ZoomLevel + delta, 0.25, 4.0);
		} else {
			// Scroll horizontally
			int delta = e.Delta.Y > 0 ? -10 : 10;
			ScrollOffset = Math.Max(0, ScrollOffset + delta);
		}

		e.Handled = true;
	}

	protected override void OnKeyDown(KeyEventArgs e) {
		base.OnKeyDown(e);

		switch (e.Key) {
			case Key.Left:
				ScrollOffset = Math.Max(0, ScrollOffset - 1);
				e.Handled = true;
				break;

			case Key.Right:
				ScrollOffset++;
				e.Handled = true;
				break;

			case Key.Home:
				ScrollOffset = 0;
				e.Handled = true;
				break;

			case Key.End:
				if (Frames is not null) {
					ScrollOffset = Math.Max(0, Frames.Count - 10);
				}

				e.Handled = true;
				break;

			case Key.OemPlus when e.KeyModifiers.HasFlag(KeyModifiers.Control):
				ZoomLevel = Math.Min(4.0, ZoomLevel + 0.25);
				e.Handled = true;
				break;

			case Key.OemMinus when e.KeyModifiers.HasFlag(KeyModifiers.Control):
				ZoomLevel = Math.Max(0.25, ZoomLevel - 0.25);
				e.Handled = true;
				break;
		}
	}

	#endregion

	#region Helpers

	private (int frame, int button) GetCellFromPosition(Point pos) {
		int cellWidth = (int)(DefaultCellWidth * ZoomLevel);
		int cellHeight = (int)(DefaultCellHeight * ZoomLevel);
		var buttons = ButtonLabels ?? _defaultButtons;

		if (pos.X < LaneHeaderWidth || pos.Y < HeaderHeight) {
			return (-1, -1);
		}

		int frame = ScrollOffset + (int)((pos.X - LaneHeaderWidth) / cellWidth);
		int button = (int)((pos.Y - HeaderHeight) / cellHeight);

		if (button >= buttons.Count) {
			return (-1, -1);
		}

		return (frame, button);
	}

	private bool GetCurrentButtonState(int frame, int button) {
		var frames = Frames;

		if (frames is null || frame < 0 || frame >= frames.Count) {
			return false;
		}

		var inputFrame = frames[frame];

		if (inputFrame.Controllers.Length == 0) {
			return false;
		}

		var buttons = ButtonLabels ?? _defaultButtons;
		return GetButtonState(inputFrame.Controllers[0], button, buttons);
	}

	/// <summary>
	/// Scrolls to make the specified frame visible.
	/// </summary>
	/// <param name="frame">The frame to scroll to.</param>
	public void ScrollToFrame(int frame) {
		int cellWidth = (int)(DefaultCellWidth * ZoomLevel);
		int visibleFrames = (int)((Bounds.Width - LaneHeaderWidth) / cellWidth);

		if (frame < ScrollOffset) {
			ScrollOffset = frame;
		} else if (frame >= ScrollOffset + visibleFrames) {
			ScrollOffset = frame - visibleFrames + 1;
		}

		InvalidateVisual();
	}

	#endregion
}

#region Event Args

/// <summary>
/// Event arguments for cell click events.
/// </summary>
public sealed class PianoRollCellEventArgs : EventArgs {
	/// <summary>Gets the frame number.</summary>
	public int Frame { get; }

	/// <summary>Gets the button index.</summary>
	public int ButtonIndex { get; }

	/// <summary>Gets the new button state.</summary>
	public bool NewState { get; }

	public PianoRollCellEventArgs(int frame, int buttonIndex, bool newState) {
		Frame = frame;
		ButtonIndex = buttonIndex;
		NewState = newState;
	}
}

/// <summary>
/// Event arguments for paint (drag) events.
/// </summary>
public sealed class PianoRollPaintEventArgs : EventArgs {
	/// <summary>Gets the frames that were painted.</summary>
	public IReadOnlyList<int> Frames { get; }

	/// <summary>Gets the button index that was painted.</summary>
	public int ButtonIndex { get; }

	/// <summary>Gets the value that was painted.</summary>
	public bool PaintValue { get; }

	public PianoRollPaintEventArgs(IReadOnlyList<int> frames, int buttonIndex, bool paintValue) {
		Frames = frames;
		ButtonIndex = buttonIndex;
		PaintValue = paintValue;
	}
}

/// <summary>
/// Event arguments for selection change events.
/// </summary>
public sealed class PianoRollSelectionEventArgs : EventArgs {
	/// <summary>Gets the selection start frame.</summary>
	public int SelectionStart { get; }

	/// <summary>Gets the selection end frame.</summary>
	public int SelectionEnd { get; }

	public PianoRollSelectionEventArgs(int start, int end) {
		SelectionStart = start;
		SelectionEnd = end;
	}
}

/// <summary>
/// Event arguments for marker click events.
/// </summary>
public sealed class PianoRollMarkerEventArgs : EventArgs {
	/// <summary>Gets the frame with the marker.</summary>
	public int Frame { get; }

	/// <summary>Gets the marker text.</summary>
	public string? MarkerText { get; }

	public PianoRollMarkerEventArgs(int frame, string? text) {
		Frame = frame;
		MarkerText = text;
	}
}

#endregion
