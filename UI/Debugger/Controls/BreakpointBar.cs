using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Interop;
using Nexen.Utilities;

namespace Nexen.Debugger.Controls;
public sealed class BreakpointBar : Control {
	protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e) {
		BreakpointManager.BreakpointsChanged += BreakpointManager_BreakpointsChanged;
		if (DataContext is DisassemblyViewModel disModel) {
			disModel.SetRefreshScrollBar(() => InvalidateVisual());
		} else if (DataContext is SourceViewViewModel srcModel) {
			srcModel.SetRefreshScrollBar(() => InvalidateVisual());
		}

		base.OnAttachedToVisualTree(e);
	}

	protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e) {
		BreakpointManager.BreakpointsChanged -= BreakpointManager_BreakpointsChanged;
		if (DataContext is DisassemblyViewModel disModel) {
			disModel.SetRefreshScrollBar(null);
		} else if (DataContext is SourceViewViewModel srcModel) {
			srcModel.SetRefreshScrollBar(null);
		}

		base.OnDetachedFromVisualTree(e);
	}

	private void BreakpointManager_BreakpointsChanged(object? sender, EventArgs e) {
		InvalidateVisual();
	}

	public override void Render(DrawingContext context) {
		base.Render(context);

		double height = Bounds.Height;
		double width = Bounds.Width - 1.0;
		if (DataContext is DisassemblyViewModel disModel) {
			int maxAddress = DebugApi.GetMemorySize(disModel.CpuType.ToMemoryType()) - 1;

			int? activeAddress = disModel.ActiveAddress;
			if (activeAddress >= 0) {
				int position = (int)((double)activeAddress.Value / maxAddress * height) - 2;
				// Use cached brush/pen to avoid per-frame allocation
				SolidColorBrush brush = ColorHelper.GetBrush(Color.FromUInt32(ConfigManager.Config.Debug.Debugger.CodeActiveStatementColor));
				Pen pen = ColorHelper.GetPen(Colors.Black);
				context.FillRectangle(brush, new Rect(0.5, position - 0.5, width, 3));
				context.DrawRectangle(pen, new Rect(0.5, position - 0.5, width, 3));
			}

			foreach (Breakpoint bp in BreakpointManager.GetBreakpoints(disModel.CpuType)) {
				int address = bp.GetRelativeAddress();
				if (address >= 0 && (bp.IsSingleAddress || bp.BreakOnExec)) {
					int position = (int)((double)address / maxAddress * height) - 2;
					if (bp.Enabled) {
						SolidColorBrush brush = ColorHelper.GetBrush(bp.GetColor());
						context.FillRectangle(brush, new Rect(0, position, 4, 4));
					} else {
						Pen pen = ColorHelper.GetPen(bp.GetColor());
						SolidColorBrush brush = ColorHelper.GetBrush(Colors.White);
						context.FillRectangle(brush, new Rect(0, position, 4, 4));
						context.DrawRectangle(pen, new Rect(0.5, position + 0.5, 3, 3));
					}
				}
			}
		} else if (DataContext is SourceViewViewModel srcModel && srcModel.SelectedFile is not null) {
			int? activeLine = srcModel.GetActiveLineIndex();
			if (activeLine >= 0) {
				int position = (int)((double)activeLine.Value / srcModel.SelectedFile.Data.Length * height) - 2;
				SolidColorBrush brush = ColorHelper.GetBrush(Color.FromUInt32(ConfigManager.Config.Debug.Debugger.CodeActiveStatementColor));
				Pen pen = ColorHelper.GetPen(Colors.Black);
				context.FillRectangle(brush, new Rect(0.5, position - 0.5, width, 3));
				context.DrawRectangle(pen, new Rect(0.5, position - 0.5, width, 3));
			}

			for (int i = 0, len = srcModel.SelectedFile.Data.Length; i < len; i++) {
				Breakpoint? bp = srcModel.GetBreakpoint(i);
				if (bp is not null) {
					int position = (int)((double)i / len * height) - 2;
					if (bp.Enabled) {
						SolidColorBrush brush = ColorHelper.GetBrush(bp.GetColor());
						context.FillRectangle(brush, new Rect(0, position, 4, 4));
					} else {
						Pen pen = ColorHelper.GetPen(bp.GetColor());
						context.DrawRectangle(pen, new Rect(0.5, position + 0.5, 3, 3));
					}
				}
			}
		}
	}
}
