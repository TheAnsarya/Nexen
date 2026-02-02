# 🌼 Pansy Export - API Documentation

This document describes the public API for the Pansy export/import functionality in Nexen.

## Core Classes

### PansyExporter

**Namespace:** `Nexen.Debugger.Integration`

Exports Nexen debug metadata to the Pansy binary format.

#### Static Methods

```csharp
/// <summary>
/// Export a Pansy file for the currently loaded ROM.
/// </summary>
/// <param name="outputPath">Destination file path</param>
/// <param name="options">Export configuration options</param>
/// <param name="progress">Optional progress callback</param>
/// <param name="cancellationToken">Optional cancellation token</param>
/// <returns>True if export succeeded</returns>
public static bool Export(
	string outputPath,
	PansyExportOptions? options = null,
	IProgress<ExportProgress>? progress = null,
	CancellationToken cancellationToken = default)
```

#### PansyExportOptions

```csharp
public class PansyExportOptions
{
	/// <summary>Include named memory regions in export.</summary>
	public bool IncludeMemoryRegions { get; set; } = true;
	
	/// <summary>Include cross-reference data in export.</summary>
	public bool IncludeCrossReferences { get; set; } = true;
	
	/// <summary>Include data type annotations.</summary>
	public bool IncludeDataBlocks { get; set; } = true;
	
	/// <summary>Use DEFLATE compression for smaller files.</summary>
	public bool UseCompression { get; set; } = true;
	
	/// <summary>Include watch expressions.</summary>
	public bool IncludeWatches { get; set; } = false;
	
	/// <summary>Include breakpoint definitions.</summary>
	public bool IncludeBreakpoints { get; set; } = false;
}
```

#### ExportProgress

```csharp
public class ExportProgress
{
	/// <summary>Current phase name (e.g., "Exporting symbols")</summary>
	public string Phase { get; set; }
	
	/// <summary>Overall progress percentage (0-100)</summary>
	public int OverallPercent { get; set; }
	
	/// <summary>Current phase progress percentage (0-100)</summary>
	public int PhasePercent { get; set; }
	
	/// <summary>Detailed status message</summary>
	public string Detail { get; set; }
}
```

---

### PansyImporter

**Namespace:** `Nexen.Debugger.Integration`

Imports Pansy format files into the Nexen debugger.

#### Static Methods

```csharp
/// <summary>
/// Import a Pansy file and merge with current debug state.
/// </summary>
/// <param name="inputPath">Pansy file to import</param>
/// <param name="showResult">Show success/failure message box</param>
/// <returns>Import result with statistics</returns>
public static ImportResult Import(string inputPath, bool showResult = true)

/// <summary>
/// Import only labels/symbols from a Pansy file.
/// </summary>
/// <param name="inputPath">Pansy file to import</param>
/// <returns>Number of labels imported</returns>
public static int ImportLabelsOnly(string inputPath)

/// <summary>
/// Validate a Pansy file without importing.
/// </summary>
/// <param name="inputPath">Pansy file to validate</param>
/// <returns>Validation result with any errors/warnings</returns>
public static ValidationResult Validate(string inputPath)
```

#### ImportResult

```csharp
public class ImportResult
{
	public bool Success { get; set; }
	public int LabelsImported { get; set; }
	public int CommentsImported { get; set; }
	public int CodeOffsetsImported { get; set; }
	public int DataOffsetsImported { get; set; }
	public int MemoryRegionsImported { get; set; }
	public int CrossReferencesImported { get; set; }
	public List<string> Warnings { get; set; }
	public string? ErrorMessage { get; set; }
}
```

---

### DebugFolderManager

**Namespace:** `Nexen.Debugger.Labels`

Manages folder-based debug storage with MLB/CDL/Pansy sync.

#### Static Methods

```csharp
/// <summary>
/// Get the debug folder path for a ROM.
/// </summary>
/// <param name="romPath">Path to the ROM file</param>
/// <returns>Path to debug folder (e.g., "RomName_debug/")</returns>
public static string GetDebugFolderPath(string romPath)

/// <summary>
/// Initialize folder structure for a ROM.
/// </summary>
/// <param name="romPath">Path to the ROM file</param>
/// <returns>True if folder was created or already exists</returns>
public static bool InitializeFolder(string romPath)

/// <summary>
/// Sync all files in the debug folder.
/// </summary>
/// <param name="romPath">Path to the ROM file</param>
public static void SyncAll(string romPath)

/// <summary>
/// Export current state to debug folder.
/// </summary>
/// <param name="romPath">Path to the ROM file</param>
/// <param name="exportMlb">Export MLB label file</param>
/// <param name="exportCdl">Export CDL file</param>
/// <param name="exportPansy">Export Pansy file</param>
public static void ExportToFolder(
	string romPath,
	bool exportMlb = true,
	bool exportCdl = true,
	bool exportPansy = true)

/// <summary>
/// Import state from debug folder.
/// </summary>
/// <param name="romPath">Path to the ROM file</param>
/// <param name="importMlb">Import MLB label file</param>
/// <param name="importCdl">Import CDL file</param>
/// <param name="importPansy">Import Pansy file</param>
public static void ImportFromFolder(
	string romPath,
	bool importMlb = true,
	bool importCdl = false,
	bool importPansy = false)
```

#### Folder Structure

```json
{RomName}_debug/
├── manifest.json		   # Metadata about ROM and settings
├── {crc32}.mlb			 # Nexen Label file
├── {crc32}.cdl			 # Code/Data Log
├── {crc32}.pansy		   # Full Pansy export
└── .history/			   # Version history (if enabled)
	└── {timestamp}.mlb
```

---

### SyncManager

**Namespace:** `Nexen.Debugger.Labels`

Monitors debug folder for external changes and triggers reloads.

#### Instance Members

```csharp
/// <summary>
/// Start watching the debug folder for changes.
/// </summary>
/// <param name="folderPath">Debug folder to monitor</param>
public void StartWatching(string folderPath)

/// <summary>
/// Stop watching for changes.
/// </summary>
public void StopWatching()

/// <summary>
/// Force an immediate sync check.
/// </summary>
public void ForceSync()

/// <summary>
/// Current sync state.
/// </summary>
public SyncState State { get; }
```

#### Events

```csharp
/// <summary>Fired when a file change is detected.</summary>
public event EventHandler<FileChangeEventArgs>? ChangeDetected;

/// <summary>Fired when sync state changes.</summary>
public event EventHandler<SyncState>? SyncStatusChanged;

/// <summary>Fired when a conflict is detected (external + local changes).</summary>
public event EventHandler<ConflictEventArgs>? ConflictDetected;
```

#### SyncState Enum

```csharp
public enum SyncState
{
	Idle,		   // No activity
	Watching,	   // Monitoring for changes
	Syncing,		// Currently syncing
	Error,		  // Error occurred
	Conflict		// Conflicting changes detected
}
```

---

### DbgToPansyConverter

**Namespace:** `Nexen.Debugger.Integration`

Converts various debug formats to Pansy.

#### Static Methods

```csharp
/// <summary>
/// Convert a debug file to Pansy format.
/// </summary>
/// <param name="inputPath">Source debug file</param>
/// <param name="outputPath">Destination Pansy file</param>
/// <param name="romInfo">ROM information for context</param>
/// <returns>Conversion result</returns>
public static ConversionResult ConvertAndExport(
	string inputPath,
	string outputPath,
	RomInfo romInfo)

/// <summary>
/// Detect the format of a debug file.
/// </summary>
/// <param name="filePath">Path to debug file</param>
/// <returns>Detected format type</returns>
public static DebugFormat DetectFormat(string filePath)

/// <summary>
/// Batch convert multiple files.
/// </summary>
/// <param name="inputPaths">Source files</param>
/// <param name="outputFolder">Destination folder</param>
/// <param name="romInfo">ROM information</param>
/// <param name="progress">Progress callback</param>
/// <returns>List of conversion results</returns>
public static List<ConversionResult> BatchConvert(
	IEnumerable<string> inputPaths,
	string outputFolder,
	RomInfo romInfo,
	IProgress<int>? progress = null)
```

#### DebugFormat Enum

```csharp
public enum DebugFormat
{
	Unknown,
	NexenMlb,  	   // Nexen Label file
	Ca65Dbg,		// cc65/ca65 debug info
	WlaDxSym,	   // WLA-DX symbol file
	RgbdsSym,	   // RGBDS symbol file
	SdccSym,		// SDCC symbol file
	ElfDwarf		// ELF with DWARF info
}
```

---

## Configuration

### IntegrationConfig

Located in `UI/Config/IntegrationConfig.cs`:

```csharp
public class IntegrationConfig
{
	// Auto-save settings
	public bool AutoSavePansy { get; set; } = false;
	public int AutoSaveIntervalMinutes { get; set; } = 5;
	public bool SaveOnRomUnload { get; set; } = true;
	
	// Export options (default)
	public bool IncludeMemoryRegions { get; set; } = true;
	public bool IncludeCrossReferences { get; set; } = true;
	public bool IncludeDataBlocks { get; set; } = true;
	public bool UseCompression { get; set; } = true;
	
	// Folder storage
	public bool UseFolderStorage { get; set; } = false;
	public bool SyncMlbFiles { get; set; } = true;
	public bool SyncCdlFiles { get; set; } = true;
	public bool KeepVersionHistory { get; set; } = false;
	public int MaxHistoryVersions { get; set; } = 10;
	
	// File watching
	public bool EnableFileWatching { get; set; } = false;
	public bool AutoReloadOnExternalChange { get; set; } = false;
}
```

---

## Binary Format Reference

### Header (32 bytes)

| Offset | Size | Field | Description |
| -------- | ------ | ------- | ------------- |
| 0x00 | 8 | Magic | "PANSY\0\0\0" |
| 0x08 | 2 | Version | Major.Minor (e.g., 0x0101) |
| 0x0A | 1 | Platform | See PlatformId enum |
| 0x0B | 1 | Flags | Bit flags (compression, etc.) |
| 0x0C | 4 | RomSize | Size of source ROM |
| 0x10 | 4 | RomCrc32 | CRC32 of ROM data |
| 0x14 | 8 | Timestamp | Unix timestamp |
| 0x1C | 4 | Reserved | Future use |

### Flags Byte

| Bit | Name | Description |
| ----- | ------ | ------------- |
| 0 | Compressed | Content is DEFLATE compressed |
| 1 | HasRegions | Contains memory region data |
| 2 | HasXrefs | Contains cross-reference data |
| 3-7 | Reserved | Future use |

### Section Format

```text
┌──────────────────┐
│ Section Type (1) │
├──────────────────┤
│ Entry Count (4)  │
├──────────────────┤
│ Entries...	   │
│ (variable)	   │
└──────────────────┘
```

### Section Types

| ID | Type | Entry Format |
| ---- | ------ | -------------- |
| 0x01 | Symbols | Address(4) + Name(string) |
| 0x02 | Comments | Address(4) + Text(string) |
| 0x03 | CodeOffsets | Address(4) |
| 0x04 | DataOffsets | Address(4) + Type(1) |
| 0x05 | JumpTargets | Address(4) |
| 0x06 | SubEntries | Address(4) |
| 0x07 | MemRegions | Start(4) + End(4) + Name(string) + Type(1) |
| 0x08 | CrossRefs | Type(1) + From(4) + To(4) |
| 0xFF | End | No data |

---

## Usage Examples

### Basic Export

```csharp
// Simple export with defaults
PansyExporter.Export("MyGame.pansy");

// Export with options
var options = new PansyExportOptions {
	UseCompression = true,
	IncludeCrossReferences = true
};
PansyExporter.Export("MyGame.pansy", options);
```

### Export with Progress

```csharp
var progress = new Progress<ExportProgress>(p => {
	Console.WriteLine($"{p.Phase}: {p.OverallPercent}%");
});

var cts = new CancellationTokenSource();
bool success = PansyExporter.Export(
	"MyGame.pansy",
	options: null,
	progress: progress,
	cancellationToken: cts.Token);
```

### Import Labels

```csharp
// Import everything
var result = PansyImporter.Import("MyGame.pansy");
if (result.Success) {
	Console.WriteLine($"Imported {result.LabelsImported} labels");
}

// Import labels only (fast)
int count = PansyImporter.ImportLabelsOnly("MyGame.pansy");
```

### Folder-Based Workflow

```csharp
string romPath = "C:/Games/MyGame.nes";

// Initialize debug folder
DebugFolderManager.InitializeFolder(romPath);

// Export to folder
DebugFolderManager.ExportToFolder(romPath,
	exportMlb: true,
	exportCdl: true,
	exportPansy: true);

// Start watching for changes
var sync = new SyncManager();
sync.ChangeDetected += (s, e) => {
	Console.WriteLine($"File changed: {e.FilePath}");
};
sync.StartWatching(DebugFolderManager.GetDebugFolderPath(romPath));
```

### Format Conversion

```csharp
// Convert ca65 debug file
var format = DbgToPansyConverter.DetectFormat("game.dbg");
if (format == DebugFormat.Ca65Dbg) {
	var result = DbgToPansyConverter.ConvertAndExport(
		"game.dbg",
		"game.pansy",
		romInfo);
}

// Batch convert
var files = Directory.GetFiles("debug/", "*.dbg");
var results = DbgToPansyConverter.BatchConvert(
	files,
	"pansy/",
	romInfo);
```
