using System;
using System.Buffers;
using System.Runtime.InteropServices;
using System.Text;

namespace Nexen.Utilities;
public sealed class Utf8Utilities {
	public static string GetStringFromArray(byte[] strArray) {
		for (int i = 0; i < strArray.Length; i++) {
			if (strArray[i] == 0) {
				return Encoding.UTF8.GetString(strArray, 0, i);
			}
		}

		return Encoding.UTF8.GetString(strArray);
	}

	/// <summary>
	/// Converts a null-terminated UTF-8 native string pointer to a managed string.
	/// Uses Marshal.PtrToStringUTF8 to avoid byte[] allocation + Marshal.Copy.
	/// </summary>
	public static string PtrToStringUtf8(IntPtr ptr) {
		if (ptr == IntPtr.Zero) {
			return "";
		}

		return Marshal.PtrToStringUTF8(ptr) ?? "";
	}

	public delegate void StringApiDelegate(IntPtr ptr, Int32 size);

	/// <summary>
	/// Calls a native string API using a pooled buffer instead of allocating a fresh byte[].
	/// Uses ArrayPool to avoid 100KB allocation per call.
	/// </summary>
	public unsafe static string CallStringApi(StringApiDelegate callback, int maxLength = 100000) {
		byte[] outBuffer = ArrayPool<byte>.Shared.Rent(maxLength);
		try {
			fixed (byte* ptr = outBuffer) {
				callback((IntPtr)ptr, maxLength);
				return Utf8Utilities.PtrToStringUtf8((IntPtr)ptr);
			}
		} finally {
			ArrayPool<byte>.Shared.Return(outBuffer);
		}
	}
}
