using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Nexen.Localization;
using Nexen.Windows;

namespace Nexen.Utilities; 
public sealed class NexenMsgBox {
	public static Task ShowException(Exception ex) {
		return NexenMsgBox.Show(null, "UnexpectedError", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.Message + Environment.NewLine + ex.StackTrace);
	}

	public static Task<DialogResult> Show(Visual? parent, string text, MessageBoxButtons buttons, MessageBoxIcon icon, params string[] args) {
		Window? wnd = ApplicationHelper.ResolveParentWindow(parent);

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
