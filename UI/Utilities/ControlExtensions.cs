using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.VisualTree;

namespace Nexen.Utilities; 
static class ControlExtensions {
	public static bool IsParentWindowFocused(this Control ctrl) {
		return TopLevel.GetTopLevel(ctrl) is WindowBase { IsKeyboardFocusWithin: true };
	}
}
