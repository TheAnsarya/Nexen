using Nexen.Debugger.Integration;
using Nexen.Debugger.Labels;
using Nexen.Interop;

namespace Nexen.Debugger.Disassembly; 
public sealed class LocationInfo {
	public AddressInfo? RelAddress;
	public AddressInfo? AbsAddress;
	public CodeLabel? Label;
	public int? LabelAddressOffset;
	public SourceSymbol? Symbol;
	public SourceCodeLocation? SourceLocation;

	public int? ArrayIndex = null;
}
