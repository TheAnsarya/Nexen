using Xunit;
using System;
using System.Text;
using Nexen.Utilities;

namespace Nexen.Tests.Utilities;

/// <summary>
/// Unit tests for <see cref="Utf8Utilities"/> string conversion from byte arrays
/// and native memory pointers.
/// </summary>
public class Utf8UtilitiesTests
{
	#region GetStringFromArray Tests

	[Fact]
	public void GetStringFromArray_SimpleAscii_ReturnsCorrectString()
	{
		byte[] data = Encoding.UTF8.GetBytes("Hello");
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("Hello", result);
	}

	[Fact]
	public void GetStringFromArray_NullTerminated_TruncatesAtNull()
	{
		byte[] data = [0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x00, 0x57, 0x6F, 0x72, 0x6C, 0x64]; // "Hello\0World"
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("Hello", result);
	}

	[Fact]
	public void GetStringFromArray_NoNullTerminator_ReturnsFullString()
	{
		byte[] data = Encoding.UTF8.GetBytes("NoNull");
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("NoNull", result);
	}

	[Fact]
	public void GetStringFromArray_LeadingNull_ReturnsEmptyString()
	{
		byte[] data = [0x00, 0x48, 0x65, 0x6C, 0x6C, 0x6F];
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("", result);
	}

	[Fact]
	public void GetStringFromArray_AllZeros_ReturnsEmptyString()
	{
		byte[] data = new byte[10];
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("", result);
	}

	[Fact]
	public void GetStringFromArray_Utf8Multibyte_ReturnsCorrectString()
	{
		// "café" in UTF-8
		byte[] data = [0x63, 0x61, 0x66, 0xC3, 0xA9];
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("café", result);
	}

	[Fact]
	public void GetStringFromArray_EmptyArray_ReturnsEmptyString()
	{
		byte[] data = [];
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("", result);
	}

	[Fact]
	public void GetStringFromArray_NullTerminatedInMiddle_StopsAtFirstNull()
	{
		byte[] data = [0x41, 0x42, 0x00, 0x43, 0x00, 0x44]; // "AB\0C\0D"
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("AB", result);
	}

	[Fact]
	public void GetStringFromArray_SingleChar_ReturnsSingleChar()
	{
		byte[] data = [0x58]; // "X"
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("X", result);
	}

	[Fact]
	public void GetStringFromArray_SingleCharNullTerminated_ReturnsSingleChar()
	{
		byte[] data = [0x58, 0x00]; // "X\0"
		string result = Utf8Utilities.GetStringFromArray(data);
		Assert.Equal("X", result);
	}

	#endregion

	#region PtrToStringUtf8 Tests

	[Fact]
	public void PtrToStringUtf8_NullPointer_ReturnsEmptyString()
	{
		string result = Utf8Utilities.PtrToStringUtf8(IntPtr.Zero);
		Assert.Equal("", result);
	}

	[Fact]
	public unsafe void PtrToStringUtf8_SimpleAscii_ReturnsCorrectString()
	{
		byte[] data = [0x54, 0x65, 0x73, 0x74, 0x00]; // "Test\0"
		fixed (byte* ptr = data) {
			string result = Utf8Utilities.PtrToStringUtf8((IntPtr)ptr);
			Assert.Equal("Test", result);
		}
	}

	[Fact]
	public unsafe void PtrToStringUtf8_EmptyString_ReturnsEmpty()
	{
		byte[] data = [0x00]; // "\0"
		fixed (byte* ptr = data) {
			string result = Utf8Utilities.PtrToStringUtf8((IntPtr)ptr);
			Assert.Equal("", result);
		}
	}

	[Fact]
	public unsafe void PtrToStringUtf8_Utf8Multibyte_ReturnsCorrectString()
	{
		// "ñ" in UTF-8 = 0xC3 0xB1
		byte[] data = [0xC3, 0xB1, 0x00];
		fixed (byte* ptr = data) {
			string result = Utf8Utilities.PtrToStringUtf8((IntPtr)ptr);
			Assert.Equal("ñ", result);
		}
	}

	#endregion
}
