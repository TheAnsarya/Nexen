using System;
using System.Runtime.InteropServices;
using Mesen.Utilities;

namespace Mesen.Interop;

[StructLayout(LayoutKind.Sequential)]
public struct InteropSaveStateInfo {
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2000)]
	public byte[] FilePath;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
	public byte[] RomName;
	public Int64 Timestamp;
	public UInt32 FileSize;
}

/// <summary>A timestamped save state file belonging to the current ROM.</summary>
public sealed class SaveStateInfo {
	public string FilePath { get; }
	public string RomName { get; }
	public DateTime Timestamp { get; }
	public uint FileSize { get; }
	/// <summary>True for saves created by the periodic auto-save.</summary>
	public bool IsAutoSave { get; }

	internal SaveStateInfo(InteropSaveStateInfo info) {
		FilePath = Utf8Utilities.GetStringFromArray(info.FilePath);
		RomName = Utf8Utilities.GetStringFromArray(info.RomName);
		// time_t is parsed from the local-time filename via mktime, so it is a real epoch value
		Timestamp = DateTimeOffset.FromUnixTimeSeconds(info.Timestamp).LocalDateTime;
		FileSize = info.FileSize;
		IsAutoSave = System.IO.Path.GetFileName(FilePath).StartsWith("auto_", StringComparison.Ordinal);
	}
}
