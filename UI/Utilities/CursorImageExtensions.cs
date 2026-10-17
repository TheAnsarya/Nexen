using System;
using Avalonia.Input;
using Nexen.Interop;

namespace Nexen.Utilities; 
public static class CursorImageExtensions {
	public static StandardCursorType ToStandardCursorType(this CursorImage cursor) {
		return cursor switch {
			CursorImage.Hidden => StandardCursorType.None,
			CursorImage.Cross => StandardCursorType.Cross,
			CursorImage.Arrow => StandardCursorType.Arrow,
			_ => throw new Exception("Unsupported icon type")
		};
	}
}
