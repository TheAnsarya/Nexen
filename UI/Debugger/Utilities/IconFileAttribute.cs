using System;

namespace Nexen.Debugger.Utilities; 
public sealed class IconFileAttribute : Attribute {
	public string Icon { get; }
	public IconFileAttribute(string icon) {
		Icon = "Assets/" + icon + ".png";
	}
}
