using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
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
#pragma warning disable CS0067 // Event is never used (reserved for future marker click handling)
	public event EventHandler<PianoRollMarkerEventArgs>? MarkerClicked;
#pragma warning restore CS0067

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
	private int _keyboardFrame = -1;
	private int _keyboardButton;
	private readonly HashSet<int> _paintedFrames = new();
	private readonly List<int> _paintFrameBuffer = new();

	#endregion

	#region Cached Rendering Resources

	// Cached brushes — avoids per-frame SolidColorBrush allocations (Render runs at 60fps)
	private static readonly SolidColorBrush HeaderBrush = new(Color.FromRgb(240, 240, 240));
	private static readonly SolidColorBrush GreenzoneBrush = new(Color.FromRgb(200, 255, 200));
	private static readonly SolidColorBrush LagBrush = new(Color.FromRgb(255, 200, 200));
	private static readonly SolidColorBrush PressedBrush = new(Color.FromRgb(100, 150, 255));
	private static readonly SolidColorBrush SelectionFillBrush = new(Color.FromArgb(50, 0, 100, 255));

	// Cached pens — avoids per-frame Pen allocations
	private static readonly Pen GrayBorderPen = new(Brushes.Gray, 1);
	private static readonly Pen GridPen = new(Brushes.LightGray, 0.5);
	private static readonly Pen SelectionBorderPen = new(Brushes.Blue, 2);
	private static readonly Pen PlaybackPen = new(Brushes.Red, 2);

	// Cached typefaces — avoids per-frame Typeface allocations
	private static readonly Typeface BoldTypeface = new("Segoe UI", FontStyle.Normal, FontWeight.Bold);
	private static readonly Typeface NormalTypeface = new("Segoe UI");

	// Keep several zoom buckets to avoid expensive full cache clears on frequent zoom toggles.
	private readonly Dictionary<double, Dictionary<string, FormattedText>> _buttonLabelCaches = new();
	private readonly LinkedList<double> _buttonLabelZoomOrder = new();
	private readonly Dictionary<double, LinkedListNode<double>> _buttonLabelZoomNodes = new();

	private readonly Dictionary<double, FrameNumberTextCache> _frameNumberCaches = new();
	private readonly LinkedList<double> _frameNumberZoomOrder = new();
	private readonly Dictionary<double, LinkedListNode<double>> _frameNumberZoomNodes = new();
	private readonly object _frameNumberCacheLock = new();
	private int _framePrefetchToken;
	private int _lastPrefetchStart = -1;
	private int _lastPrefetchEnd = -1;
	private double _lastPrefetchZoomKey = double.NaN;

	private const int MaxFrameNumberCacheSize = 200;
	private const int MaxZoomCacheSize = 3;
	private const int FramePrefetchBatchSize = 64;

	private sealed class FrameNumberTextCache {
		public Dictionary<int, FormattedText> Items { get; } = new();
		public LinkedList<int> LruOrder { get; } = new();
		public Dictionary<int, LinkedListNode<int>> LruNodes { get; } = new();
	}

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
		var textBrush = Brushes.Black;
		var cache = GetOrCreateButtonLabelCache(ZoomLevel);

		for (int i = 0; i < buttons.Count; i++) {
			var rect = new Rect(0, HeaderHeight + i * cellHeight, LaneHeaderWidth, cellHeight);
			context.FillRectangle(HeaderBrush, rect);
			context.DrawRectangle(GrayBorderPen, rect);

			// Get or create cached FormattedText
			if (!cache.TryGetValue(buttons[i], out var text)) {
				text = new FormattedText(
					buttons[i],
					System.Globalization.CultureInfo.CurrentCulture,
					FlowDirection.LeftToRight,
					BoldTypeface,
					10 * ZoomLevel,
					textBrush
				);
				cache[buttons[i]] = text;
			}

			var textX = rect.X + (rect.Width - text.Width) / 2;
			var textY = rect.Y + (rect.Height - text.Height) / 2;
			context.DrawText(text, new Point(textX, textY));
		}
	}

	private void DrawFrameHeaders(DrawingContext context, Rect bounds, int cellWidth) {
		var textBrush = Brushes.Black;
		var cache = GetOrCreateFrameNumberCache(ZoomLevel);
		var frames = Frames;

		int totalFrames = frames?.Count ?? int.MaxValue;
		var (start, endExclusive) = ComputeVisibleFrameRange(bounds.Width, LaneHeaderWidth, cellWidth, ScrollOffset, totalFrames);
		int visibleFrames = endExclusive - start;

		for (int frame = start; frame < endExclusive; frame++) {
			int i = frame - ScrollOffset;
			var rect = new Rect(LaneHeaderWidth + i * cellWidth, 0, cellWidth, HeaderHeight);

			var bgBrush = frame >= GreenzoneStart ? GreenzoneBrush : HeaderBrush;
			context.FillRectangle(bgBrush, rect);
			context.DrawRectangle(GrayBorderPen, rect);

			// Only show frame numbers at intervals
			if (frame % 10 == 0 || ZoomLevel >= 2.0) {
				FormattedText text;
				lock (_frameNumberCacheLock) {
					if (!cache.Items.TryGetValue(frame, out text!)) {
						text = new FormattedText(
							frame.ToString(),
							System.Globalization.CultureInfo.CurrentCulture,
							FlowDirection.LeftToRight,
							NormalTypeface,
							8 * ZoomLevel,
							textBrush
						);
						AddOrUpdateFrameText(cache, frame, text);
					} else {
						TouchFrameText(cache, frame);
					}
				}

				var textX = rect.X + (rect.Width - text.Width) / 2;
				context.DrawText(text, new Point(textX, (HeaderHeight - text.Height) / 2));
			}
		}

		QueueFrameTextPrefetch(start, visibleFrames, ZoomLevel);
	}

	private void QueueFrameTextPrefetch(int scrollOffset, int visibleFrames, double zoom) {
		var frames = Frames;
		if (frames is null || frames.Count == 0) {
			return;
		}

		int start = Math.Max(0, scrollOffset - visibleFrames);
		int end = Math.Min(frames.Count - 1, scrollOffset + (visibleFrames * 2));
		if (start >= end) {
			return;
		}

		double zoomKey = NormalizeZoom(zoom);
		if (!ShouldQueueFrameTextPrefetch(_lastPrefetchStart, _lastPrefetchEnd, _lastPrefetchZoomKey, start, end, zoomKey)) {
			return;
		}

		_lastPrefetchStart = start;
		_lastPrefetchEnd = end;
		_lastPrefetchZoomKey = zoomKey;

		int token = Interlocked.Increment(ref _framePrefetchToken);
		Task.Run(() => PrefetchFrameText(token, start, end, zoom));
	}

	internal static bool ShouldQueueFrameTextPrefetch(int lastStart, int lastEnd, double lastZoomKey, int start, int end, double zoomKey) {
		return lastStart != start || lastEnd != end || !lastZoomKey.Equals(zoomKey);
	}

	internal static (int Start, int EndExclusive) ComputeVisibleFrameRange(double boundsWidth, int laneHeaderWidth, int cellWidth, int scrollOffset, int totalFrames) {
		if (totalFrames <= 0 || cellWidth <= 0 || boundsWidth <= laneHeaderWidth) {
			return (0, 0);
		}

		int visibleFrames = (int)((boundsWidth - laneHeaderWidth) / cellWidth) + 1;
		int start = Math.Clamp(scrollOffset, 0, totalFrames);
		int endExclusive = Math.Clamp(start + visibleFrames, start, totalFrames);
		return (start, endExclusive);
	}

	private void PrefetchFrameText(int token, int start, int end, double zoom) {
		if (token != Volatile.Read(ref _framePrefetchToken)) {
			return;
		}

		var pending = new List<int>(FramePrefetchBatchSize);
		var cache = GetOrCreateFrameNumberCache(zoom);

		lock (_frameNumberCacheLock) {
			for (int frame = start; frame <= end && pending.Count < FramePrefetchBatchSize; frame++) {
				if (frame % 10 != 0 && zoom < 2.0) {
					continue;
				}

				if (!cache.Items.ContainsKey(frame)) {
					pending.Add(frame);
				}
			}
		}

		if (pending.Count == 0 || token != Volatile.Read(ref _framePrefetchToken)) {
			return;
		}

		var prepared = new List<(int Frame, FormattedText Text)>(pending.Count);
		foreach (int frame in pending) {
			prepared.Add((frame, new FormattedText(
				frame.ToString(),
				System.Globalization.CultureInfo.CurrentCulture,
				FlowDirection.LeftToRight,
				NormalTypeface,
				8 * zoom,
				Brushes.Black
			)));
		}

		if (token != Volatile.Read(ref _framePrefetchToken)) {
			return;
		}

		bool added = false;
		lock (_frameNumberCacheLock) {
			foreach (var (frame, text) in prepared) {
				if (!cache.Items.ContainsKey(frame)) {
					AddOrUpdateFrameText(cache, frame, text);
					added = true;
				}
			}
		}

		if (added) {
			Dispatcher.UIThread.Post(InvalidateVisual, DispatcherPriority.Background);
		}
	}

	private Dictionary<string, FormattedText> GetOrCreateButtonLabelCache(double zoom) {
		double zoomKey = NormalizeZoom(zoom);
		if (!_buttonLabelCaches.TryGetValue(zoomKey, out var cache)) {
			cache = new Dictionary<string, FormattedText>();
			_buttonLabelCaches[zoomKey] = cache;
		}

		TouchZoomEntry(zoomKey, _buttonLabelZoomOrder, _buttonLabelZoomNodes);
		if (_buttonLabelCaches.Count > MaxZoomCacheSize && _buttonLabelZoomOrder.First is not null) {
			double evictZoom = _buttonLabelZoomOrder.First.Value;
			_buttonLabelZoomOrder.RemoveFirst();
			_buttonLabelZoomNodes.Remove(evictZoom);
			_buttonLabelCaches.Remove(evictZoom);
		}

		return cache;
	}

	private FrameNumberTextCache GetOrCreateFrameNumberCache(double zoom) {
		double zoomKey = NormalizeZoom(zoom);
		lock (_frameNumberCacheLock) {
			if (!_frameNumberCaches.TryGetValue(zoomKey, out var cache)) {
				cache = new FrameNumberTextCache();
				_frameNumberCaches[zoomKey] = cache;
			}

			TouchZoomEntry(zoomKey, _frameNumberZoomOrder, _frameNumberZoomNodes);
			if (_frameNumberCaches.Count > MaxZoomCacheSize && _frameNumberZoomOrder.First is not null) {
				double evictZoom = _frameNumberZoomOrder.First.Value;
				_frameNumberZoomOrder.RemoveFirst();
				_frameNumberZoomNodes.Remove(evictZoom);
				_frameNumberCaches.Remove(evictZoom);
			}

			return cache;
		}
	}

	private static void TouchZoomEntry(double zoomKey, LinkedList<double> order, Dictionary<double, LinkedListNode<double>> nodes) {
		if (nodes.TryGetValue(zoomKey, out var existing)) {
			order.Remove(existing);
			nodes[zoomKey] = order.AddLast(zoomKey);
		} else {
			nodes[zoomKey] = order.AddLast(zoomKey);
		}
	}

	private static void AddOrUpdateFrameText(FrameNumberTextCache cache, int frame, FormattedText text) {
		cache.Items[frame] = text;
		TouchFrameText(cache, frame);

		while (cache.Items.Count > MaxFrameNumberCacheSize && cache.LruOrder.First is not null) {
			int oldest = cache.LruOrder.First.Value;
			cache.LruOrder.RemoveFirst();
			cache.LruNodes.Remove(oldest);
			cache.Items.Remove(oldest);
		}
	}

	private static void TouchFrameText(FrameNumberTextCache cache, int frame) {
		if (cache.LruNodes.TryGetValue(frame, out var existing)) {
			cache.LruOrder.Remove(existing);
			cache.LruNodes[frame] = cache.LruOrder.AddLast(frame);
		} else {
			cache.LruNodes[frame] = cache.LruOrder.AddLast(frame);
		}
	}

	private static double NormalizeZoom(double zoom) {
		return Math.Round(zoom, 3);
	}

	private void DrawGrid(DrawingContext context, Rect bounds, IReadOnlyList<string> buttons, int cellWidth, int cellHeight) {
		var frames = Frames;

		if (frames is null) {
			return;
		}

		var (start, endExclusive) = ComputeVisibleFrameRange(bounds.Width, LaneHeaderWidth, cellWidth, ScrollOffset, frames.Count);

		for (int frameIndex = start; frameIndex < endExclusive; frameIndex++) {
			int i = frameIndex - ScrollOffset;

			var frame = frames[frameIndex];
			bool isGreenzone = frameIndex >= GreenzoneStart;

			for (int j = 0; j < buttons.Count; j++) {
				var rect = new Rect(LaneHeaderWidth + i * cellWidth, HeaderHeight + j * cellHeight, cellWidth, cellHeight);

				// Background color
				IBrush? bgBrush = null;

				if (frame.IsLagFrame) {
					bgBrush = LagBrush;
				} else if (isGreenzone) {
					bgBrush = GreenzoneBrush;
				}

				if (bgBrush is not null) {
					context.FillRectangle(bgBrush, rect);
				}

				// Check if button is pressed
				if (frame.Controllers.Length > 0) {
					bool isPressed = GetButtonState(frame.Controllers[0], j, buttons);

					if (isPressed) {
						var innerRect = rect.Deflate(2);
						context.FillRectangle(PressedBrush, innerRect);
					}
				}

				// Grid lines
				context.DrawRectangle(GridPen, rect);
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

		var rect = new Rect(startX, HeaderHeight, endX - startX, height);
		context.FillRectangle(SelectionFillBrush, rect);
		context.DrawRectangle(SelectionBorderPen, rect);
	}

	private void DrawPlaybackPosition(DrawingContext context, Rect bounds, int cellWidth) {
		if (PlaybackFrame < 0) {
			return;
		}

		int x = LaneHeaderWidth + (PlaybackFrame - ScrollOffset) * cellWidth + cellWidth / 2;

		if (x < LaneHeaderWidth || x > bounds.Width) {
			return;
		}

		context.DrawLine(PlaybackPen, new Point(x, 0), new Point(x, bounds.Height));
	}

	private void DrawMarkers(DrawingContext context, Rect bounds, int cellWidth) {
		var frames = Frames;

		if (frames is null) {
			return;
		}

		var markerBrush = Brushes.Orange;
		var (start, endExclusive) = ComputeVisibleFrameRange(bounds.Width, LaneHeaderWidth, cellWidth, ScrollOffset, frames.Count);

		for (int frameIndex = start; frameIndex < endExclusive; frameIndex++) {
			int i = frameIndex - ScrollOffset;

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
			_keyboardFrame = frame;
			_keyboardButton = button;
			_paintedFrames.Clear();

			// Determine paint value (toggle current state)
			_paintValue = !GetCurrentButtonState(frame, button);
			_paintedFrames.Add(frame);

			// Raise click event
			CellClicked?.Invoke(this, new PianoRollCellEventArgs(frame, button, _paintValue));

			e.Handled = true;
		} else if (e.GetCurrentPoint(this).Properties.IsRightButtonPressed) {
			// Right-click for selection
			_keyboardFrame = frame;
			_keyboardButton = button;
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
		if (button == _paintButton && !_paintedFrames.Contains(frame)) {
			_paintedFrames.Add(frame);
			CellClicked?.Invoke(this, new PianoRollCellEventArgs(frame, button, _paintValue));
		}
	}

	protected override void OnPointerReleased(PointerReleasedEventArgs e) {
		base.OnPointerReleased(e);

		if (_isPainting && _paintedFrames.Count > 1) {
			// Raise paint event for multiple cells
			_paintFrameBuffer.Clear();
			foreach (int frame in _paintedFrames) {
				_paintFrameBuffer.Add(frame);
			}
			CellsPainted?.Invoke(this, new PianoRollPaintEventArgs(
				_paintFrameBuffer.ToList(),
				_paintButton,
				_paintValue
			));
		}

		_isPainting = false;
		_paintedFrames.Clear();
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

		if (e.KeyModifiers.HasFlag(KeyModifiers.Control) && e.Key == Key.G) {
			e.Handled = false;
			return;
		}

		if (Frames is null || Frames.Count == 0) {
			return;
		}

		EnsureKeyboardCursor();
		bool extendSelection = e.KeyModifiers.HasFlag(KeyModifiers.Shift);
		var buttons = ButtonLabels ?? _defaultButtons;

		switch (e.Key) {
			case Key.Left:
				MoveKeyboardFrame(-1, extendSelection);
				e.Handled = true;
				break;

			case Key.Right:
				MoveKeyboardFrame(1, extendSelection);
				e.Handled = true;
				break;

			case Key.Up:
				_keyboardButton = Math.Max(0, _keyboardButton - 1);
				e.Handled = true;
				break;

			case Key.Down:
				_keyboardButton = Math.Min(buttons.Count - 1, _keyboardButton + 1);
				e.Handled = true;
				break;

			case Key.Home:
				MoveKeyboardFrame(-int.MaxValue, extendSelection);
				e.Handled = true;
				break;

			case Key.End:
				MoveKeyboardFrame(int.MaxValue, extendSelection);
				e.Handled = true;
				break;

			case Key.Tab:
				_keyboardButton = ComputeNextButtonLane(_keyboardButton, buttons.Count, e.KeyModifiers.HasFlag(KeyModifiers.Shift));
				e.Handled = true;
				break;

			case Key.Enter:
				if (_keyboardFrame >= 0 && _keyboardFrame < Frames.Count && _keyboardButton >= 0 && _keyboardButton < buttons.Count) {
					bool newState = !GetCurrentButtonState(_keyboardFrame, _keyboardButton);
					CellClicked?.Invoke(this, new PianoRollCellEventArgs(_keyboardFrame, _keyboardButton, newState));
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

	private void EnsureKeyboardCursor() {
		if (_keyboardFrame >= 0) {
			return;
		}

		if (SelectedFrame >= 0) {
			_keyboardFrame = SelectedFrame;
		} else if (SelectionStart >= 0) {
			_keyboardFrame = SelectionStart;
		} else {
			_keyboardFrame = ScrollOffset;
		}

		_keyboardButton = Math.Max(0, _keyboardButton);
	}

	private void MoveKeyboardFrame(int delta, bool extendSelection) {
		if (Frames is null || Frames.Count == 0) {
			return;
		}

		var result = ComputeFrameSelectionAfterMove(_keyboardFrame, SelectionStart, SelectionEnd, Frames.Count, delta, extendSelection);
		SelectionStart = result.Start;
		SelectionEnd = result.End;
		_keyboardFrame = result.Cursor;
		SelectedFrame = result.Cursor;
		SelectionChanged?.Invoke(this, new PianoRollSelectionEventArgs(SelectionStart, SelectionEnd));
		ScrollToFrame(result.Cursor);
	}

	internal static int ComputeNextButtonLane(int currentLane, int laneCount, bool reverse) {
		if (laneCount <= 0) {
			return 0;
		}

		int step = reverse ? -1 : 1;
		int next = (currentLane + step) % laneCount;
		if (next < 0) {
			next += laneCount;
		}
		return next;
	}

	internal static (int Start, int End, int Cursor) ComputeFrameSelectionAfterMove(int currentFrame, int selectionStart, int selectionEnd, int frameCount, int delta, bool extendSelection) {
		if (frameCount <= 0) {
			return (-1, -1, -1);
		}

		int maxFrame = frameCount - 1;
		int cursor = currentFrame < 0 ? 0 : currentFrame;
		int next = delta switch {
			<= -1000000 => 0,
			>= 1000000 => maxFrame,
			_ => Math.Clamp(cursor + delta, 0, maxFrame)
		};

		if (extendSelection && selectionStart >= 0) {
			return (selectionStart, next, next);
		}

		return (next, next, next);
	}

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
