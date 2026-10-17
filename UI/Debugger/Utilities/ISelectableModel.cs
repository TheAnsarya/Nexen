using Nexen.Interop;

namespace Nexen.Debugger.Utilities; 
public interface ISelectableModel {
	void ResizeSelectionTo(int rowNumber);
	void MoveCursor(int rowOffset, bool extendSelection);
	void ScrollToTop(bool extendSelection);
	void ScrollToBottom(bool extendSelection);
	void SetSelectedRow(int rowNumber);
	bool IsSelected(int rowNumber);
	void Scroll(int offset);
	AddressInfo? GetSelectedRowAddress();

	int VisibleRowCount { get; }
}
