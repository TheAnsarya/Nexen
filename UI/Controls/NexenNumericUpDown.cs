using System;
using Avalonia.Controls;

namespace Nexen.Controls;

/// <summary>
/// A numeric up-down control that prevents invalid cast errors when the user
/// clears the text content by defaulting to "0".
/// </summary>
public sealed class NexenNumericUpDown : NumericUpDown {
	protected override Type StyleKeyOverride => typeof(NumericUpDown);

	protected override void OnTextChanged(string? oldValue, string? newValue) {
		if (newValue is null or "") {
			//Prevent displaying invalid cast error/breaking the layout when user clears the content of the control
			Text = "0";
		}

		base.OnTextChanged(oldValue, newValue);
	}
}
