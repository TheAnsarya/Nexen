using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using Avalonia.Controls;

namespace Nexen.Utilities; 
public static class TooltipHelper {
	public static void ShowTooltip(Control target, object? tooltipContent, int horizontalOffset) {
		try {
			ToolTip.SetShowDelay(target, 0);
			ToolTip.SetTip(target, tooltipContent);

			//Force tooltip to update its position
			ToolTip.SetHorizontalOffset(target, horizontalOffset - 1);
			ToolTip.SetHorizontalOffset(target, horizontalOffset);

			ToolTip.SetIsOpen(target, true);
		} catch (Exception ex) {
			Debug.WriteLine($"TooltipHelper.ShowTooltip failed: {ex.Message}");
			HideTooltip(target);
		}
	}

	public static void HideTooltip(Control target) {
		try {
			ToolTip.SetTip(target, null);
			ToolTip.SetIsOpen(target, false);
		} catch (Exception ex) {
			Debug.WriteLine($"TooltipHelper.HideTooltip failed: {ex.Message}");
		}
	}
}
