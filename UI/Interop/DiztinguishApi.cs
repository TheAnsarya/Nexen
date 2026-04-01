using System;
using System.Runtime.InteropServices;

namespace Mesen.Interop
{
	public class DiztinguishApi
	{
		private const string DllPath = EmuApi.DllName;

		[DllImport(DllPath)] public static extern bool DiztinguishApi_StartServer(UInt16 port);
		[DllImport(DllPath)] public static extern void DiztinguishApi_StopServer();
		[DllImport(DllPath)] public static extern bool DiztinguishApi_IsServerRunning();
		[DllImport(DllPath)] public static extern bool DiztinguishApi_IsClientConnected();
		[DllImport(DllPath)] public static extern UInt16 DiztinguishApi_GetServerPort();

		[DllImport(DllPath)] public static extern UInt64 DiztinguishApi_GetMessagesSent();
		[DllImport(DllPath)] public static extern UInt64 DiztinguishApi_GetMessagesReceived();
		[DllImport(DllPath)] public static extern UInt64 DiztinguishApi_GetBytesSent();
		[DllImport(DllPath)] public static extern UInt64 DiztinguishApi_GetBytesReceived();
		[DllImport(DllPath)] public static extern UInt64 DiztinguishApi_GetConnectionDuration();
		[DllImport(DllPath)] public static extern double DiztinguishApi_GetBandwidthKBps();

		// Diagnostic functions for streaming health
		[DllImport(DllPath)] public static extern UInt32 DiztinguishApi_GetCurrentFrame();
		[DllImport(DllPath)] public static extern UInt32 DiztinguishApi_GetTraceBufferSize();
		[DllImport(DllPath)] public static extern bool DiztinguishApi_IsConfigReceived();

		// Convenience wrapper methods
		public static bool StartServer(ushort port = 9998)
		{
			return DiztinguishApi_StartServer(port);
		}

		public static void StopServer()
		{
			DiztinguishApi_StopServer();
		}

		public static bool IsServerRunning()
		{
			return DiztinguishApi_IsServerRunning();
		}

		public static bool IsClientConnected()
		{
			return DiztinguishApi_IsClientConnected();
		}

		public static ushort GetServerPort()
		{
			return DiztinguishApi_GetServerPort();
		}

		public static ulong GetMessagesSent()
		{
			return DiztinguishApi_GetMessagesSent();
		}

		public static ulong GetMessagesReceived()
		{
			return DiztinguishApi_GetMessagesReceived();
		}

		public static ulong GetBytesSent()
		{
			return DiztinguishApi_GetBytesSent();
		}

		public static ulong GetBytesReceived()
		{
			return DiztinguishApi_GetBytesReceived();
		}

		public static TimeSpan GetConnectionDuration()
		{
			return TimeSpan.FromMilliseconds(DiztinguishApi_GetConnectionDuration());
		}

		public static double GetBandwidthKBps()
		{
			return DiztinguishApi_GetBandwidthKBps();
		}

		// Diagnostic functions
		public static uint GetCurrentFrame()
		{
			return DiztinguishApi_GetCurrentFrame();
		}

		public static uint GetTraceBufferSize()
		{
			return DiztinguishApi_GetTraceBufferSize();
		}

		public static bool IsConfigReceived()
		{
			return DiztinguishApi_IsConfigReceived();
		}
	}
}