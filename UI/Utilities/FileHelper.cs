using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Threading;

namespace Nexen.Utilities; 
/// <summary>
/// Provides resilient file I/O operations with automatic retry and error handling.
/// </summary>
/// <remarks>
/// <para>
/// All methods in this class automatically retry operations up to 3 times with
/// a 50ms delay between attempts. This helps handle transient file access issues
/// such as antivirus locks or pending write operations.
/// </para>
/// <para>
/// Errors are displayed to the user via <see cref="NexenMsgBox"/> after all
/// retries are exhausted.
/// </para>
/// </remarks>
internal sealed class FileHelper {
	/// <summary>
	/// Reads all text from a file with retry logic.
	/// </summary>
	/// <param name="filepath">The path to the file to read.</param>
	/// <returns>
	/// The file contents as a string, or <c>null</c> if the operation failed.
	/// </returns>
	public static string? ReadAllText(string filepath) {
		return AttemptOperation(() => File.ReadAllText(filepath));
	}

	/// <summary>
	/// Writes text to a file using UTF-8 encoding with retry logic.
	/// </summary>
	/// <param name="filepath">The path to the file to write.</param>
	/// <param name="content">The text content to write.</param>
	/// <returns><c>true</c> if successful; otherwise, <c>false</c>.</returns>
	public static bool WriteAllText(string filepath, string content) {
		return WriteAllText(filepath, content, Encoding.UTF8);
	}

	/// <summary>
	/// Writes text to a file using the specified encoding with retry logic.
	/// </summary>
	/// <param name="filepath">The path to the file to write.</param>
	/// <param name="content">The text content to write.</param>
	/// <param name="encoding">The text encoding to use.</param>
	/// <returns><c>true</c> if successful; otherwise, <c>false</c>.</returns>
	public static bool WriteAllText(string filepath, string content, Encoding encoding) {
		return AttemptOperation(() => {
			File.WriteAllText(filepath, content, encoding);
			return true;
		});
	}

	/// <summary>
	/// Reads all bytes from a file with retry logic.
	/// </summary>
	/// <param name="filepath">The path to the file to read.</param>
	/// <returns>
	/// The file contents as a byte array, or <c>null</c> if the operation failed.
	/// </returns>
	public static byte[]? ReadAllBytes(string filepath) {
		return AttemptOperation(() => File.ReadAllBytes(filepath));
	}

	/// <summary>
	/// Writes bytes to a file with retry logic.
	/// </summary>
	/// <param name="filepath">The path to the file to write.</param>
	/// <param name="data">The byte data to write.</param>
	/// <returns><c>true</c> if successful; otherwise, <c>false</c>.</returns>
	public static bool WriteAllBytes(string filepath, byte[] data) {
		return AttemptOperation(() => {
			File.WriteAllBytes(filepath, data);
			return true;
		});
	}

	/// <summary>
	/// Opens a file for reading with shared read/write access.
	/// </summary>
	/// <param name="filepath">The path to the file to open.</param>
	/// <returns>
	/// A <see cref="FileStream"/> for reading, or <c>null</c> if the operation failed.
	/// </returns>
	/// <remarks>
	/// Uses <see cref="FileShare.ReadWrite"/> to allow other processes to access the file.
	/// </remarks>
	public static FileStream? OpenRead(string filepath) {
		return AttemptOperation(() => File.Open(filepath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite));
	}

	/// <summary>
	/// Executes a file operation with automatic retry on failure.
	/// </summary>
	/// <typeparam name="T">The return type of the operation.</typeparam>
	/// <param name="action">The file operation to attempt.</param>
	/// <returns>
	/// The result of the operation, or <c>default</c> if all retries failed.
	/// </returns>
	/// <remarks>
	/// Retries up to 3 times with 50ms delays. Shows an error dialog on the UI thread
	/// if all attempts fail.
	/// </remarks>
	private static T? AttemptOperation<T>(Func<T> action) {
		int retry = 3;
		while (retry > 0) {
			try {
				return action();
			} catch (Exception ex) {
				retry--;
				if (retry == 0) {
					// Show error on UI thread
					if (Dispatcher.UIThread.CheckAccess()) {
						NexenMsgBox.ShowException(ex);
					} else {
						Dispatcher.UIThread.Post(() => NexenMsgBox.ShowException(ex));
					}

					return default;
				} else {
					// Wait before retrying
					System.Threading.Thread.Sleep(50);
				}
			}
		}

		return default;
	}
}
