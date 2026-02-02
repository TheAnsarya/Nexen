using System;
using Dock.Model.Mvvm.Controls;
using Nexen.Debugger.Utilities;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels.DebuggerDock;
/// <summary>
/// Base class for tool container ViewModels in the debugger dock.
/// Provides help tooltip support and selection notification.
/// </summary>
public class BaseToolContainerViewModel : Tool {
	/// <summary>
	/// Gets the help content to display when hovering over the tool.
	/// Override in derived classes to provide contextual help.
	/// </summary>
	public virtual object? HelpContent { get; } = null;

	/// <summary>
	/// Occurs when the tool is selected in the dock.
	/// </summary>
	public event EventHandler? Selected;

	/// <summary>
	/// Called when the tool is selected. Raises the <see cref="Selected"/> event.
	/// </summary>
	public override void OnSelected() {
		Selected?.Invoke(this, EventArgs.Empty);
	}
}

/// <summary>
/// Generic tool container ViewModel that wraps a model of type <typeparamref name="T"/>.
/// Used to host tool panels in the debugger dock system.
/// </summary>
/// <typeparam name="T">The type of the model contained in this tool.</typeparam>
public class ToolContainerViewModel<T> : BaseToolContainerViewModel {
	/// <summary>
	/// Gets or sets the model instance contained in this tool.
	/// </summary>
	public T? Model {
		get;
		set {
			field = value;
			OnPropertyChanged(nameof(Model));
		}
	}

	/// <summary>
	/// Gets the help content from the model if it implements <see cref="IToolHelpTooltip"/>.
	/// </summary>
	public override object? HelpContent {
		get {
			if (Model is IToolHelpTooltip help) {
				return help.HelpTooltip;
			}

			return null;
		}
	}

	/// <summary>
	/// Initializes a new instance of the <see cref="ToolContainerViewModel{T}"/> class.
	/// </summary>
	/// <param name="name">The name and title of the tool container.</param>
	public ToolContainerViewModel(string name) {
		Id = name;
		Title = name;
		CanPin = false;
		CanFloat = false;
	}
}
