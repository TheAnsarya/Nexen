using System.Collections.Generic;
using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config; 
public sealed partial class MemorySearchConfig : BaseWindowConfig<MemorySearchConfig> {
	[Reactive] public partial List<int> ColumnWidths { get; set; } = new();
}
