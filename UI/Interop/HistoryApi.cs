using System;
using System.Runtime.InteropServices;
using Nexen.Config;

namespace Nexen.Interop;
public sealed class HistoryApi {
	private const string DllPath = EmuApi.DllName;

	[DllImport(DllPath)] public static extern void HistoryViewerInitialize(IntPtr windowHandle, IntPtr viewerHandle);
	[DllImport(DllPath)] public static extern void HistoryViewerRelease();

	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool HistoryViewerEnabled();

	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool HistoryViewerSaveMovie([MarshalAs(UnmanagedType.LPUTF8Str)] string movieFile, UInt32 startPosition, UInt32 endPosition);
	[DllImport(DllPath)][return: MarshalAs(UnmanagedType.I1)] public static extern bool HistoryViewerCreateSaveState([MarshalAs(UnmanagedType.LPUTF8Str)] string outfileFile, UInt32 position);
	[DllImport(DllPath)] public static extern void HistoryViewerSetPosition(UInt32 seekPosition);
	[DllImport(DllPath)] public static extern void HistoryViewerResumeGameplay(UInt32 seekPosition);

	[DllImport(DllPath, EntryPoint = "HistoryViewerGetState")] private static extern void HistoryViewerGetStateWrapper(out HistoryViewerState state);
	public static HistoryViewerState HistoryViewerGetState() {
		HistoryViewerGetStateWrapper(out HistoryViewerState state);
		return state;
	}
	[DllImport(DllPath)] public static extern void HistoryViewerSetOptions(HistoryViewerOptions options);

	[DllImport(DllPath)] public static extern IntPtr HistoryViewerRegisterNotificationCallback(NotificationListener.NotificationCallback callback);
	[DllImport(DllPath)] public static extern void HistoryViewerUnregisterNotificationCallback(IntPtr notificationListener);
}

public struct HistoryViewerState {
	public UInt32 Position;
	public UInt32 Length;
	public UInt32 Volume;
	public double Fps;
	[MarshalAs(UnmanagedType.I1)] public bool IsPaused;

	public UInt32 SegmentCount;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 1000)]
	public UInt32[] Segments;
}

public struct HistoryViewerOptions {
	[MarshalAs(UnmanagedType.I1)] public bool IsPaused;
	public UInt32 Volume;
	public UInt32 Width;
	public UInt32 Height;
}
