using System;
using System.Collections.Generic;
using System.IO;
using Nexen.Interop;

namespace Nexen.Debugger.Integration; 
/// <summary>
/// Interface for providers that supply source-level debugging information.
/// </summary>
/// <remarks>
/// <para>
/// Symbol providers enable source-level debugging by mapping between:
/// <list type="bullet">
///   <item><description>Memory addresses ↔ Source file locations</description></item>
///   <item><description>Symbol names ↔ Memory addresses</description></item>
///   <item><description>Scope information for local variables</description></item>
/// </list>
/// </para>
/// <para>
/// Implementations exist for various debug formats:
/// <list type="bullet">
///   <item><description><see cref="DbgImporter"/> - ca65/ld65 DBG files</description></item>
///   <item><description><see cref="SdccSymbolImporter"/> - SDCC CDB files</description></item>
///   <item><description><see cref="WlaDxImporter"/> - WLA-DX symbol files</description></item>
///   <item><description><see cref="ElfImporter"/> - ELF/DWARF debug info</description></item>
///   <item><description><see cref="PansyImporter"/> - Pansy metadata files</description></item>
/// </list>
/// </para>
/// </remarks>
public interface ISymbolProvider {
	/// <summary>
	/// Gets the file system timestamp of when the symbol file was last modified.
	/// </summary>
	/// <remarks>
	/// Used to detect when symbol files have been updated and should be reloaded.
	/// </remarks>
	DateTime SymbolFileStamp { get; }

	/// <summary>
	/// Gets the file path of the loaded symbol file.
	/// </summary>
	string SymbolPath { get; }

	/// <summary>
	/// Gets the list of source files referenced by this symbol provider.
	/// </summary>
	List<SourceFileInfo> SourceFiles { get; }

	/// <summary>
	/// Gets the start address of a source code line.
	/// </summary>
	/// <param name="file">The source file containing the line.</param>
	/// <param name="lineIndex">The zero-based line index in the file.</param>
	/// <returns>
	/// The address info for the start of the line, or <c>null</c> if no mapping exists.
	/// </returns>
	AddressInfo? GetLineAddress(SourceFileInfo file, int lineIndex);

	/// <summary>
	/// Gets the end address of a source code line (exclusive).
	/// </summary>
	/// <param name="file">The source file containing the line.</param>
	/// <param name="lineIndex">The zero-based line index in the file.</param>
	/// <returns>
	/// The address info for the end of the line, or <c>null</c> if no mapping exists.
	/// </returns>
	AddressInfo? GetLineEndAddress(SourceFileInfo file, int lineIndex);

	/// <summary>
	/// Gets the source code line corresponding to a PRG ROM address.
	/// </summary>
	/// <param name="prgRomAddress">The absolute PRG ROM address.</param>
	/// <returns>The source code line text, or <c>null</c> if no mapping exists.</returns>
	string? GetSourceCodeLine(int prgRomAddress);

	/// <summary>
	/// Gets detailed source location information for an address.
	/// </summary>
	/// <param name="address">The address to look up.</param>
	/// <returns>
	/// The source file and line number, or <c>null</c> if no mapping exists.
	/// </returns>
	SourceCodeLocation? GetSourceCodeLineInfo(AddressInfo address);

	/// <summary>
	/// Gets the address information for a source symbol.
	/// </summary>
	/// <param name="symbol">The symbol to look up.</param>
	/// <returns>The address info, or <c>null</c> if the symbol has no address.</returns>
	AddressInfo? GetSymbolAddressInfo(SourceSymbol symbol);

	/// <summary>
	/// Gets the source location where a symbol is defined.
	/// </summary>
	/// <param name="symbol">The symbol to look up.</param>
	/// <returns>
	/// The definition location, or <c>null</c> if not available.
	/// </returns>
	SourceCodeLocation? GetSymbolDefinition(SourceSymbol symbol);

	/// <summary>
	/// Finds a symbol by name within a scope range.
	/// </summary>
	/// <param name="word">The symbol name to search for.</param>
	/// <param name="scopeStart">The start address of the scope to search.</param>
	/// <param name="scopeEnd">The end address of the scope to search.</param>
	/// <returns>The matching symbol, or <c>null</c> if not found.</returns>
	/// <remarks>
	/// Scope ranges are used to resolve local variables and labels that may
	/// have the same name in different scopes.
	/// </remarks>
	SourceSymbol? GetSymbol(string word, int scopeStart, int scopeEnd);

	/// <summary>
	/// Gets all symbols from this provider.
	/// </summary>
	/// <returns>A list of all symbols.</returns>
	List<SourceSymbol> GetSymbols();

	/// <summary>
	/// Gets the size in bytes of a symbol's data.
	/// </summary>
	/// <param name="srcSymbol">The symbol to get the size of.</param>
	/// <returns>The size in bytes, or 0 if unknown.</returns>
	int GetSymbolSize(SourceSymbol srcSymbol);
}

/// <summary>
/// Interface for providers that supply file content data.
/// </summary>
/// <remarks>
/// Used to abstract file content access, allowing both embedded and external files.
/// </remarks>
public interface IFileDataProvider {
	/// <summary>
	/// Gets the file content as an array of lines.
	/// </summary>
	string[] Data { get; }
}

/// <summary>
/// Represents a source file referenced by debug symbols.
/// </summary>
public sealed class SourceFileInfo {
	/// <summary>
	/// Gets the file name or path of the source file.
	/// </summary>
	public string Name { get; }

	/// <summary>
	/// Gets the provider for the file's content.
	/// </summary>
	public IFileDataProvider InternalFile { get; }

	/// <summary>
	/// Gets whether this file contains assembly language code.
	/// </summary>
	/// <remarks>
	/// Used to determine syntax highlighting and debugging behavior.
	/// </remarks>
	public bool IsAssembly { get; }

	/// <summary>
	/// Gets the file content as an array of lines.
	/// </summary>
	public string[] Data => InternalFile.Data;

	/// <summary>
	/// Gets the complete file content as a single string.
	/// </summary>
	public string FileData => string.Join(Environment.NewLine, InternalFile.Data);

	/// <summary>
	/// Initializes a new instance of the <see cref="SourceFileInfo"/> class.
	/// </summary>
	/// <param name="name">The file name or path.</param>
	/// <param name="isAssembly">Whether the file contains assembly code.</param>
	/// <param name="internalFile">The provider for file content.</param>
	public SourceFileInfo(string name, bool isAssembly, IFileDataProvider internalFile) {
		Name = name;
		IsAssembly = isAssembly;
		InternalFile = internalFile;
	}

	/// <summary>
	/// Returns a string representation showing the file path.
	/// </summary>
	/// <returns>The folder/filename for display purposes.</returns>
	public override string ToString() {
		string? folderName = Path.GetDirectoryName(Name);
		string fileName = Path.GetFileName(Name);
		if (string.IsNullOrWhiteSpace(folderName)) {
			return fileName;
		} else {
			return $"{folderName}/{fileName}";
		}
	}
}

/// <summary>
/// Represents a symbol from source-level debug information.
/// </summary>
/// <remarks>
/// Symbols include labels, variables, constants, and other named entities
/// from the source code that have been mapped to memory addresses.
/// </remarks>
public readonly struct SourceSymbol {
	/// <summary>
	/// Gets the name of the symbol as it appears in source code.
	/// </summary>
	public string Name { get; }

	/// <summary>
	/// Gets the memory address of the symbol, or <c>null</c> if address-less.
	/// </summary>
	public int? Address { get; }

	/// <summary>
	/// Gets the internal symbol object from the debug format provider.
	/// </summary>
	/// <remarks>
	/// The type of this object depends on the symbol provider implementation.
	/// </remarks>
	public object InternalSymbol { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="SourceSymbol"/> struct.
	/// </summary>
	/// <param name="name">The symbol name.</param>
	/// <param name="address">The optional memory address.</param>
	/// <param name="internalSymbol">The internal symbol object.</param>
	public SourceSymbol(string name, int? address, object internalSymbol) {
		Name = name;
		Address = address;
		InternalSymbol = internalSymbol;
	}
}

/// <summary>
/// Represents a location in source code (file and line number).
/// </summary>
public readonly struct SourceCodeLocation : IEquatable<SourceCodeLocation> {
	/// <summary>
	/// Gets the source file containing this location.
	/// </summary>
	public SourceFileInfo File { get; }

	/// <summary>
	/// Gets the one-based line number in the source file.
	/// </summary>
	public int LineNumber { get; }

	/// <summary>
	/// Gets the internal line object from the debug format provider.
	/// </summary>
	/// <remarks>
	/// May contain additional line metadata from the debug format.
	/// </remarks>
	public object? InternalLine { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="SourceCodeLocation"/> struct.
	/// </summary>
	/// <param name="file">The source file.</param>
	/// <param name="lineNumber">The one-based line number.</param>
	/// <param name="internalLine">Optional internal line data.</param>
	public SourceCodeLocation(SourceFileInfo file, int lineNumber, object? internalLine = null) {
		File = file;
		LineNumber = lineNumber;
		InternalLine = internalLine;
	}

	/// <summary>
	/// Determines whether this location equals another location.
	/// </summary>
	/// <param name="other">The other location to compare.</param>
	/// <returns><c>true</c> if the file and line number match.</returns>
	public bool Equals(SourceCodeLocation other) {
		return File == other.File && LineNumber == other.LineNumber;
	}
}
