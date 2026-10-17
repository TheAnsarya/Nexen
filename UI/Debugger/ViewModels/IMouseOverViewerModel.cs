using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Nexen.Debugger.Controls;
using Nexen.Utilities;

namespace Nexen.Debugger.ViewModels; 
/// <summary>
/// Interface for ViewModels that provide mouse-over tooltips on picture viewers.
/// </summary>
/// <remarks>
/// <para>
/// Implemented by ViewModels for graphics viewers (tiles, sprites, palettes) to provide
/// contextual information when the mouse hovers over specific pixels or tiles.
/// </para>
/// <para>
/// The <see cref="MouseViewerModelEvents"/> class handles the event wiring between
/// the PictureViewer control and ViewModels implementing this interface.
/// </para>
/// </remarks>
public interface IMouseOverViewerModel {
	/// <summary>
	/// Gets or sets the current tooltip panel being displayed.
	/// </summary>
	DynamicTooltip? ViewerTooltip { get; set; }

	/// <summary>
	/// Gets or sets the current mouse position in grid coordinates (tile/pixel position).
	/// </summary>
	PixelPoint? ViewerMousePos { get; set; }

	/// <summary>
	/// Creates or updates a tooltip panel for the specified grid position.
	/// </summary>
	/// <param name="p">The grid position (tile or pixel coordinates) to get tooltip for.</param>
	/// <param name="tooltipToUpdate">An existing tooltip to update, or null to create a new one.</param>
	/// <returns>A DynamicTooltip with content for the position, or null to hide the tooltip.</returns>
	DynamicTooltip? GetPreviewPanel(PixelPoint p, DynamicTooltip? tooltipToUpdate);
}

/// <summary>
/// Helper class that wires up mouse events from a PictureViewer to an IMouseOverViewerModel.
/// </summary>
/// <remarks>
/// <para>
/// Handles PointerMoved and PointerExited events on the PictureViewer control,
/// converting screen coordinates to grid coordinates and invoking the ViewModel's
/// tooltip generation methods.
/// </para>
/// <para>
/// Tooltips are shown/hidden using the <see cref="TooltipHelper"/> utility class.
/// </para>
/// </remarks>
public sealed class MouseViewerModelEvents {
	private IMouseOverViewerModel _model;
	private Window _wnd;

	/// <summary>
	/// Initializes mouse event handling for a picture viewer.
	/// </summary>
	/// <param name="model">The ViewModel implementing mouse-over tooltip support.</param>
	/// <param name="wnd">The parent window (for tooltip positioning).</param>
	/// <param name="viewer">The PictureViewer control to attach events to.</param>
	public static void InitEvents(IMouseOverViewerModel model, Window wnd, PictureViewer viewer) {
		new MouseViewerModelEvents(model, wnd, viewer);
	}

	/// <summary>
	/// Private constructor - use <see cref="InitEvents"/> to create instances.
	/// </summary>
	private MouseViewerModelEvents(IMouseOverViewerModel model, Window wnd, PictureViewer viewer) {
		_model = model;
		_wnd = wnd;
		viewer.PointerMoved += PicViewer_PointerMoved;
		viewer.PointerExited += PicViewer_PointerExited;
	}

	/// <summary>
	/// Handles mouse movement over the picture viewer to update tooltips.
	/// </summary>
	private void PicViewer_PointerMoved(object? sender, PointerEventArgs e) {
		if (sender is PictureViewer viewer) {
			// Convert mouse position to grid coordinates
			PixelPoint? point = viewer.GetGridPointFromMousePoint(e.GetCurrentPoint(viewer).Position);
			if (point == _model.ViewerMousePos) {
				return;
			}

			_model.ViewerMousePos = point;

			// Get or update tooltip for this position
			_model.ViewerTooltip = point is null ? null : _model.GetPreviewPanel(point.Value, _model.ViewerTooltip);

			if (_model.ViewerTooltip is not null) {
				TooltipHelper.ShowTooltip(viewer, _model.ViewerTooltip, 15);
			} else {
				_model.ViewerTooltip = null;
				TooltipHelper.HideTooltip(viewer);
			}
		}
	}

	/// <summary>
	/// Handles mouse leaving the picture viewer to hide tooltips.
	/// </summary>
	private void PicViewer_PointerExited(object? sender, PointerEventArgs e) {
		if (sender is PictureViewer viewer) {
			TooltipHelper.HideTooltip(viewer);
		}

		_model.ViewerTooltip = null;
		_model.ViewerMousePos = null;
	}
}
