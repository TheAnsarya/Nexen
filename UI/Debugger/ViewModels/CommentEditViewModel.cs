using System;
using Nexen.Debugger.Labels;
using Nexen.ViewModels;
using static Nexen.Debugger.ViewModels.LabelEditViewModel;

namespace Nexen.Debugger.ViewModels {
	public class CommentEditViewModel : ViewModelBase {
		public ReactiveCodeLabel Label { get; set; }

		[Obsolete("For designer only")]
		public CommentEditViewModel() : this(new CodeLabel()) { }

		public CommentEditViewModel(CodeLabel label) {
			Label = new ReactiveCodeLabel(label);
		}
	}
}
