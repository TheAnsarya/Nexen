using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Rendering;
using Nexen.Localization;
using Nexen.Windows;

namespace Nexen.Utilities; 
public sealed class NexenMsgBox {
	public static Task ShowException(Exception ex) {
		return NexenMsgBox.Show(null, "UnexpectedError", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.Message + Environment.NewLine + ex.StackTrace);
	}

	public static Task<DialogResult> Show(IRenderRoot? parent, string text, MessageBoxButtons buttons, MessageBoxIcon icon, params string[] args) {
		Window? wnd = parent as Window;
		if (parent is not null && wnd is null) {
			throw new Exception("Invalid parent window");
		}

		string resourceText = ResourceHelper.GetMessage(text, args);

		if (resourceText.StartsWith("[[")) {
			if (args is not null && args.Length > 0) {
				return MessageBox.Show(wnd, $"Critical error ({text}) {args}", "Nexen", buttons, icon);
			} else {
				return MessageBox.Show(wnd, $"Critical error ({text})", "Nexen", buttons, icon);
			}
		} else {
			return MessageBox.Show(wnd, ResourceHelper.GetMessage(text, args), "Nexen", buttons, icon);
		}
	}
}
