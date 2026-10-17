using Dock.Model.Controls;
using Dock.Model.Core;
using Dock.Model.Mvvm.Core;

/// <summary>
/// Custom proportional dock splitter that inherits from <see cref="DockBase"/>
/// instead of <see cref="Dock.Model.Mvvm.Controls.ProportionalDockSplitter"/>
/// (which inherits from DockableBase and causes style application exceptions).
/// </summary>
public sealed class NexenProportionalDockSplitter : DockBase, IProportionalDockSplitter {
	/// <inheritdoc/>
	public bool CanResize { get; set; } = true;

	/// <inheritdoc/>
	public bool ResizePreview { get; set; } = false;
}
