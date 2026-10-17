using System;
using System.Text.Json.Serialization;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger;

/// <summary>
/// Represents a debugger bookmark for quick navigation to a specific address.
/// </summary>
public sealed partial class Bookmark : ReactiveObject {
	[Reactive] public partial CpuType CpuType { get; set; }
	[Reactive] public partial MemoryType MemoryType { get; set; }
	[Reactive] public partial uint Address { get; set; }
	[Reactive] public partial string Name { get; set; } = "";
	[Reactive] public partial byte Color { get; set; }

	[JsonIgnore]
	public string DisplayAddress => $"${Address:x4}";

	public Bookmark() { }

	public Bookmark(CpuType cpuType, MemoryType memoryType, uint address, string name, byte color = 0) {
		CpuType = cpuType;
		MemoryType = memoryType;
		Address = address;
		Name = name;
		Color = color;
	}
}
