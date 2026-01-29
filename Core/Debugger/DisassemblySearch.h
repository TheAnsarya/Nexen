#pragma once
#include "pch.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DebugUtilities.h"

class Disassembler;
class LabelManager;
enum class CpuType : uint8_t;

/// <summary>
/// Disassembly search options.
/// </summary>
struct DisassemblySearchOptions {
	bool MatchCase;        ///< True for case-sensitive search
	bool MatchWholeWord;   ///< True to match whole words only
	bool SearchBackwards;  ///< True to search backwards
	bool SkipFirstLine;    ///< True to skip first line (continue search)
};

/// <summary>
/// Disassembly text search (find instructions, operands, labels, comments).
/// </summary>
/// <remarks>
/// Architecture:
/// - Searches disassembled text (instruction mnemonics, operands, labels, comments)
/// - Uses Disassembler to generate disassembly on-the-fly
/// - Template optimization for case-sensitive vs case-insensitive
/// 
/// Search features:
/// - Case-sensitive/insensitive matching
/// - Whole word matching (word boundary detection)
/// - Forward/backward search
/// - Multi-result search (find all occurrences)
/// 
/// Text matching:
/// - Template <matchCase> for compile-time branch elimination
/// - IsWordSeparator(): Detects word boundaries (space, comma, etc)
/// - TextContains(): Substring matching with options
/// 
/// Use cases:
/// - Find all uses of instruction (e.g., all "STA $2000")
/// - Find label references (e.g., all "JMP MyFunction")
/// - Find specific operands (e.g., all "$ff")
/// - Navigate disassembly (find next/prev occurrence)
/// </remarks>
class DisassemblySearch {
private:
	Disassembler* _disassembler;   ///< Disassembler for generating disassembly
	LabelManager* _labelManager;   ///< Label manager for label resolution

	/// <summary>
	/// Search disassembly for string.
	/// </summary>
	uint32_t SearchDisassembly(CpuType cpuType, const char* searchString, int32_t startAddress, DisassemblySearchOptions options, CodeLineData searchResults[], uint32_t maxResultCount);

	/// <summary>
	/// Check if text contains substring (template for case matching).
	/// </summary>
	template <bool matchCase>
	bool TextContains(string& needle, const char* hay, int size, DisassemblySearchOptions& options);
	
	/// <summary>
	/// Check if text contains substring (runtime case matching).
	/// </summary>
	bool TextContains(string& needle, const char* hay, int size, DisassemblySearchOptions& options);
	
	/// <summary>
	/// Check if character is word separator.
	/// </summary>
	bool IsWordSeparator(char c);

public:
	/// <summary>
	/// Constructor for disassembly search.
	/// </summary>
	DisassemblySearch(Disassembler* disassembler, LabelManager* labelManager);

	/// <summary>
	/// Search disassembly for next occurrence.
	/// </summary>
	/// <param name="cpuType">CPU type</param>
	/// <param name="searchString">Search string</param>
	/// <param name="startAddress">Start address</param>
	/// <param name="options">Search options</param>
	/// <returns>Address of next occurrence, or -1 if not found</returns>
	int32_t SearchDisassembly(CpuType cpuType, const char* searchString, int32_t startAddress, DisassemblySearchOptions options);
	
	/// <summary>
	/// Find all occurrences of search string.
	/// </summary>
	/// <param name="cpuType">CPU type</param>
	/// <param name="searchString">Search string</param>
	/// <param name="options">Search options</param>
	/// <param name="output">Output array for results</param>
	/// <param name="maxResultCount">Maximum results</param>
	/// <returns>Number of results found</returns>
	uint32_t FindOccurrences(CpuType cpuType, const char* searchString, DisassemblySearchOptions options, CodeLineData output[], uint32_t maxResultCount);
};