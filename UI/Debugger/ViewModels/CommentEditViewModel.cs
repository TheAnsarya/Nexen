using System;
using Nexen.Debugger.Labels;
using Nexen.ViewModels;
using static Nexen.Debugger.ViewModels.LabelEditViewModel;

namespace Nexen.Debugger.ViewModels; 
/// <summary>
/// ViewModel for editing comment-only labels in the debugger.
/// </summary>
/// <remarks>
/// <para>
/// This ViewModel provides a simplified interface for editing labels that contain
/// only comments (no label name). It reuses the <see cref="ReactiveCodeLabel"/> class
/// from <see cref="LabelEditViewModel"/> to provide reactive two-way binding.
/// </para>
/// <para>
/// Used by the comment edit dialog to allow users to add or modify inline comments
/// in the disassembly view.
/// </para>
/// </remarks>
public sealed class CommentEditViewModel : ViewModelBase {
	/// <summary>
	/// Gets or sets the reactive code label being edited.
	/// </summary>
	/// <remarks>
	/// This wraps a <see cref="CodeLabel"/> instance with reactive property change
	/// notifications for data binding in the UI.
	/// </remarks>
	public ReactiveCodeLabel Label { get; set; }

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public CommentEditViewModel() : this(new CodeLabel()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="CommentEditViewModel"/> class.
	/// </summary>
	/// <param name="label">The code label to edit.</param>
	public CommentEditViewModel(CodeLabel label) {
		Label = new ReactiveCodeLabel(label);
	}
}
