# 🌼 Pansy Export - Developer Guide

This guide is for developers extending or maintaining the Pansy export functionality.

## Architecture Overview

```text
┌─────────────────────────────────────────────────────────────────┐
│                     Pansy Export System                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌─────────────────┐   ┌─────────────────┐                    │
│  │  PansyExporter  │────▶│  Pansy Binary   │                  │
│  │                 │   │   Format   │                 │
│  └────────┬────────┘   └─────────────────┘                    │
│          │                       ▲                             │
│          ▼                       │                             │
│  ┌─────────────────┐   ┌───────┴─────────┐                    │
│  │  LabelManager   │   │  PansyImporter  │                    │
│  │  CdlManager     │   │               │                  │
│  │  DebugInfo   │  └─────────────────┘                    │
│  └─────────────────┘                                          │
│          │                                                     │
│          ▼                                                     │
│  ┌─────────────────────────────────────────┐                  │
│  │         DebugFolderManager           │                 │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ │                  │
│  │  │ MLB Sync │ │ CDL Sync │ │  Pansy   │ │                  │
│  │  └──────────┘ └──────────┘ └──────────┘ │                  │
│  └─────────────────────────────────────────┘                  │
│          │                                                     │
│          ▼                                                     │
│  ┌─────────────────────────────────────────┐                  │
│  │            SyncManager                │                    │
│  │     (FileSystemWatcher integration)      │                 │
│  └─────────────────────────────────────────┘                  │
│                                                                │
│  ┌─────────────────────────────────────────┐                  │
│  │         DbgToPansyConverter             │                  │
│  │   ca65 | WLA-DX | RGBDS | SDCC | ELF   │                   │
│  └─────────────────────────────────────────┘                  │
│                                                                │
└─────────────────────────────────────────────────────────────────┘
```

## Source Files

### Core Export/Import

| File | Purpose |
| ------ | --------- |
| `UI/Debugger/Integration/PansyExporter.cs` | Main export logic |
| `UI/Debugger/Integration/PansyImporter.cs` | Main import logic |
| `UI/Debugger/Integration/DbgToPansyConverter.cs` | Format conversion |

### Storage & Sync

| File | Purpose |
| ------ | --------- |
| `UI/Debugger/Labels/DebugFolderManager.cs` | Folder-based storage |
| `UI/Debugger/Labels/SyncManager.cs` | File watching |

### UI Components

| File | Purpose |
| ------ | --------- |
| `UI/Debugger/Windows/PansyExportProgressWindow.axaml` | Progress dialog UI |
| `UI/Debugger/Windows/PansyExportProgressWindow.axaml.cs` | Progress logic |

### Configuration

| File | Purpose |
| ------ | --------- |
| `UI/Config/IntegrationConfig.cs` | Settings model |
| `UI/Debugger/Windows/DebuggerConfigWindow.axaml` | Settings UI |

### Tests

| File | Purpose |
| ------ | --------- |
| `Tests/Debugger/Labels/PansyExporterTests.cs` | Export tests |
| `Tests/Debugger/Labels/PansyImporterTests.cs` | Import tests |
| `Tests/Debugger/Labels/DebugFolderManagerTests.cs` | Folder tests |
| `Tests/Debugger/Labels/SyncManagerTests.cs` | Sync tests |
| `Tests/Debugger/Labels/DbgToPansyConverterTests.cs` | Conversion tests |

## Binary Format Implementation

### Header Writing

```csharp
private static void WriteHeader(BinaryWriter writer, PansyHeader header)
{
    // Magic: "PANSY\0\0\0" (8 bytes)
    writer.Write(Encoding.ASCII.GetBytes("PANSY\0\0\0"));
    
    // Version: Major.Minor as uint16
    writer.Write((ushort)((header.VersionMajor << 8) | header.VersionMinor));
    
    // Platform ID (1 byte)
    writer.Write((byte)header.Platform);
    
    // Flags (1 byte)
    byte flags = 0;
    if (header.Compressed) flags |= 0x01;
    if (header.HasRegions) flags |= 0x02;
    if (header.HasXrefs) flags |= 0x04;
    writer.Write(flags);
    
    // ROM size (4 bytes)
    writer.Write(header.RomSize);
    
    // ROM CRC32 (4 bytes)
    writer.Write(header.RomCrc32);
    
    // Timestamp (8 bytes)
    writer.Write(header.Timestamp);
    
    // Reserved (4 bytes)
    writer.Write(0);
}
```

### Section Writing

```csharp
private static void WriteSymbolSection(BinaryWriter writer, List<CodeLabel> labels)
{
    writer.Write((byte)SectionType.Symbols);
    writer.Write(labels.Count);
    
    foreach (var label in labels)
    {
        // Address as 4-byte value (memory type in high byte)
        uint address = ((uint)label.MemoryType << 24) | (uint)label.Address;
        writer.Write(address);
        
        // Name as length-prefixed UTF-8
        byte[] nameBytes = Encoding.UTF8.GetBytes(label.Label);
        writer.Write((byte)nameBytes.Length);
        writer.Write(nameBytes);
    }
}
```

### Compression

```csharp
private static byte[] CompressData(byte[] data)
{
    using var output = new MemoryStream();
    using (var deflate = new DeflateStream(output, CompressionLevel.Optimal))
    {
        deflate.Write(data, 0, data.Length);
    }
    return output.ToArray();
}

private static byte[] DecompressData(byte[] compressed, int uncompressedSize)
{
    using var input = new MemoryStream(compressed);
    using var deflate = new DeflateStream(input, CompressionMode.Decompress);
    byte[] output = new byte[uncompressedSize];
    deflate.ReadExactly(output, 0, uncompressedSize);
    return output;
}
```

## Nexen Integration Points

### Getting Labels

```csharp
// Get all labels from LabelManager
var labels = LabelManager.GetLabels();

// Filter by memory type
var prgLabels = labels.Where(l => 
    l.MemoryType == MemoryType.NesPrgRom ||
    l.MemoryType == MemoryType.SnesPrgRom);
```

### Getting CDL Data

```csharp
// Get Code/Data Log for analysis
byte[] cdlData = DebugApi.GetCdlData(
    0,  // Start offset
    DebugApi.GetMemorySize(MemoryType.NesPrgRom),
    MemoryType.NesPrgRom);

// Interpret CDL flags
const byte CdlCode = 0x01;
const byte CdlData = 0x02;
const byte CdlJumpTarget = 0x04;
const byte CdlSubEntryPoint = 0x08;
```

### Getting ROM Info

```csharp
// Get current ROM information
var romInfo = EmuApi.GetRomInfo();

// Key fields:
// - romInfo.RomPath
// - romInfo.Format (NES, SNES, GB, etc.)
// - romInfo.Crc32
// - romInfo.RomSize
```

### Setting Labels

```csharp
// Add labels to LabelManager
var labels = new List<CodeLabel> { /* ... */ };
LabelManager.SetLabels(labels, raiseEvents: true);

// Single label
LabelManager.SetLabel(
    address: 0x8000,
    memoryType: MemoryType.NesPrgRom,
    label: "Reset",
    comment: "Entry point"
);
```

## Adding New Features

### Adding a New Section Type

1. **Define Section ID** in constants:

```csharp
private const byte SectionTypeNewFeature = 0x09;
```

1. **Add Export Method**:

```csharp
private static void WriteNewFeatureSection(BinaryWriter writer, ...)
{
    writer.Write(SectionTypeNewFeature);
    writer.Write(count);
    foreach (var item in items)
    {
        // Write item data
    }
}
```

1. **Add Import Method**:

```csharp
private static void ReadNewFeatureSection(BinaryReader reader, ImportResult result)
{
    int count = reader.ReadInt32();
    for (int i = 0; i < count; i++)
    {
        // Read item data
        // Apply to Nexen
    }
}
```

1. **Add to Export Options**:

```csharp
public class PansyExportOptions
{
    public bool IncludeNewFeature { get; set; } = true;
}
```

1. **Update Header Flags** (if needed):

```csharp
if (options.IncludeNewFeature) flags |= 0x08;
```

### Adding a New Debug Format Converter

1. **Add Format Enum**:

```csharp
public enum DebugFormat
{
    // ... existing ...
    NewFormat
}
```

1. **Add Detection Logic**:

```csharp
public static DebugFormat DetectFormat(string path)
{
    var content = File.ReadAllText(path);
    if (content.StartsWith("NEWFORMAT"))
        return DebugFormat.NewFormat;
    // ... existing ...
}
```

1. **Add Import Method**:

```csharp
private static void ImportNewFormat(string path, List<CodeLabel> labels)
{
    // Parse format
    // Convert to CodeLabel objects
}
```

1. **Wire Up in ConvertAndExport**:

```csharp
case DebugFormat.NewFormat:
    ImportNewFormat(inputPath, labels);
    break;
```

## Testing Guidelines

### Unit Test Structure

```csharp
public class PansyExporterTests
{
    [Fact]
    public void Export_WithLabels_CreatesValidFile()
    {
        // Arrange
        var labels = new List<CodeLabel> { /* test data */ };
        var outputPath = Path.GetTempFileName();
        
        try
        {
            // Act
            bool result = PansyExporter.Export(outputPath);
            
            // Assert
            Assert.True(result);
            Assert.True(File.Exists(outputPath));
            // Verify file contents
        }
        finally
        {
            File.Delete(outputPath);
        }
    }
}
```

### Integration Test Pattern

```csharp
[Fact]
public void ExportImport_RoundTrip_PreservesLabels()
{
    // Export
    var exportPath = Path.GetTempFileName();
    PansyExporter.Export(exportPath);
    
    // Clear labels
    LabelManager.ClearLabels();
    
    // Import
    var result = PansyImporter.Import(exportPath);
    
    // Verify labels restored
    var labels = LabelManager.GetLabels();
    Assert.Equal(expectedCount, labels.Count());
}
```

### Mock Patterns

```csharp
// For testing without full Nexen context
public interface ILabelProvider
{
    IEnumerable<CodeLabel> GetLabels();
    void SetLabels(IEnumerable<CodeLabel> labels);
}

// Mock implementation
public class MockLabelProvider : ILabelProvider
{
    public List<CodeLabel> Labels { get; } = new();
    public IEnumerable<CodeLabel> GetLabels() => Labels;
    public void SetLabels(IEnumerable<CodeLabel> labels) => Labels.AddRange(labels);
}
```

## Error Handling

### Export Errors

```csharp
public static bool Export(string path, ...)
{
    try
    {
        // Export logic
        return true;
    }
    catch (IOException ex)
    {
        Log.Error($"Failed to write Pansy file: {ex.Message}");
        return false;
    }
    catch (Exception ex)
    {
        Log.Error($"Unexpected export error: {ex}");
        return false;
    }
}
```

### Import Errors

```csharp
public static ImportResult Import(string path, ...)
{
    var result = new ImportResult();
    
    try
    {
        // Validate header
        if (!ValidateHeader(reader))
        {
            result.ErrorMessage = "Invalid Pansy file format";
            return result;
        }
        
        // Check ROM compatibility
        if (header.RomCrc32 != currentRomCrc)
        {
            result.Warnings.Add($"ROM CRC mismatch: expected {header.RomCrc32:X8}");
        }
        
        // Import sections
        // ...
        
        result.Success = true;
    }
    catch (Exception ex)
    {
        result.ErrorMessage = ex.Message;
    }
    
    return result;
}
```

## Performance Considerations

### Large File Handling

```csharp
// Stream-based processing for large files
using var fileStream = new FileStream(path, FileMode.Open, FileAccess.Read, 
    FileShare.Read, bufferSize: 65536);
using var bufferedStream = new BufferedStream(fileStream);
using var reader = new BinaryReader(bufferedStream);
```

### Parallel Processing

```csharp
// Parallel cross-reference analysis
var chunks = labels.Chunk(1000);
var results = new ConcurrentBag<CrossReference>();

Parallel.ForEach(chunks, chunk =>
{
    foreach (var label in chunk)
    {
        var xrefs = AnalyzeCrossReferences(label);
        foreach (var xref in xrefs)
            results.Add(xref);
    }
});
```

### Memory Management

```csharp
// Pool buffers for repeated operations
private static readonly ArrayPool<byte> BufferPool = ArrayPool<byte>.Shared;

public static void ProcessData(...)
{
    byte[] buffer = BufferPool.Rent(4096);
    try
    {
        // Use buffer
    }
    finally
    {
        BufferPool.Return(buffer);
    }
}
```

## Localization

### Adding New Strings

1. **Add to resources.en.xml**:

```xml
<Form ID="MyNewWindow">
    <Entry ID="Title">My New Window</Entry>
    <Entry ID="Description">This is a description</Entry>
</Form>
```

1. **Use in Code**:

```csharp
var title = ResourceHelper.GetMessage("MyNewWindow_Title");
```

1. **Use in AXAML**:

```xml
<TextBlock Text="{l:Translate MyNewWindow_Description}" />
```

## Debugging Tips

### Enable Verbose Logging

```csharp
#if DEBUG
    Log.Debug($"Exporting {labels.Count} labels");
    Log.Debug($"CDL data: {cdlData.Length} bytes");
#endif
```

### Inspect Binary Files

```csharp
// Hex dump helper
public static void DumpHex(byte[] data, int length = 64)
{
    for (int i = 0; i < Math.Min(data.Length, length); i += 16)
    {
        var hex = BitConverter.ToString(data, i, Math.Min(16, data.Length - i));
        Debug.WriteLine($"{i:X4}: {hex}");
    }
}
```

### Test with Known Files

Keep test files in `~manual-testing/`:

- `test_minimal.pansy` - Smallest valid file
- `test_all_sections.pansy` - All section types
- `test_compressed.pansy` - With compression
- `test_large.pansy` - Stress test

## Version History

| Version | Changes |
| --------- | --------- |
| 1.0 | Initial release - symbols, comments, code/data |
| 1.1 | Added memory regions, cross-references, compression |
| 1.2 | (Future) Watches, breakpoints, trace data |
