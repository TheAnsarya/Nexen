# ROM Hash Verification Plan

**Created:** 2026-01-26
**Completed:** 2026-01-26 09:00 UTC
**Status:** ✅ IMPLEMENTED
**Commit:** `38dfccf7`
**Priority:** High

## Overview

Currently, Pansy files are named based on the ROM filename. This creates a problem when playing hacked or translated versions of a ROM - the Pansy file could be corrupted or have data from a different ROM version mixed in.

**Goal:** Verify ROM CRC32/MD5/SHA1 hash before updating a Pansy file. Only update if the hash matches, or create a new Pansy file for the different ROM version.

## Current Behavior

1. Load "Game.nes" (original)
2. Play and document → saves "Game.pansy"
3. Load "Game.nes" (but it's actually a translation patch!)
4. Play and document → OVERWRITES "Game.pansy" with wrong data!

## Desired Behavior

1. Load "Game.nes" (original, CRC32: AAAA1111)
2. Play and document → saves "Game-AAAA1111.pansy" (or stores hash in header)
3. Load "Game.nes" (translation, CRC32: BBBB2222)
4. Play and document → saves "Game-BBBB2222.pansy" (separate file)
5. Existing "Game-AAAA1111.pansy" is preserved!

## Pansy File Format Changes

### Option A: Hash in Filename

```json
{RomName}-{CRC32}.pansy
Example: Dragon Warrior IV-A1B2C3D4.pansy
```

**Pros:**

- Easy to identify at a glance
- No format changes needed
- Multiple versions visible in folder

**Cons:**

- Long filenames
- Need to migrate existing files

### Option B: Hash in Pansy Header (Current Format)

The Pansy format already has a `RomCrc32` field in the header! Currently it's a placeholder.

```text
Header Layout (16 bytes):
- Magic: 8 bytes "PANSY\0\0\0"
- Version: 2 bytes (0x0100)
- Platform: 1 byte
- Flags: 1 byte
- RomSize: 4 bytes (currently used)
- RomCrc32: 4 bytes (CURRENTLY PLACEHOLDER - FIX THIS!)
```

**Implementation:**

1. Calculate actual ROM CRC32 on load
2. Store in Pansy header
3. Before updating existing Pansy, check if CRC matches
4. If mismatch: create new file or warn user

**Pros:**

- No filename changes
- Format already supports it
- Clean user experience

**Cons:**

- Need to read existing Pansy to check hash
- User may not realize they have wrong hash

### Recommended: Option B with Fallback

1. Store CRC32 in Pansy header (fix existing placeholder)
2. Before updating:
   - If Pansy doesn't exist: create new with current ROM's CRC
   - If Pansy exists: read header, compare CRC
   - If CRC matches: update normally
   - If CRC mismatch: create `{RomName}-{CRC32}.pansy`

## Implementation Steps

### Step 1: Calculate ROM CRC32 Properly

Location: `UI/Debugger/Labels/PansyExporter.cs`

```csharp
private static uint CalculateRomCrc32(RomInfo romInfo)
{
	// Get PRG ROM data
	var memType = romInfo.ConsoleType.GetMainCpuType().GetPrgRomMemoryType();
	int size = DebugApi.GetMemorySize(memType);
	
	if (size <= 0) return 0;
	
	byte[] romData = DebugApi.GetMemoryState(memType);
	
	// Calculate CRC32 using System.IO.Hashing
	return System.IO.Hashing.Crc32.HashToUInt32(romData);
}
```

### Step 2: Store CRC32 in Header

Location: `UI/Debugger/Labels/PansyExporter.cs` - `Export()` method

```csharp
// Calculate actual ROM CRC32 (replace placeholder)
uint romCrc32 = CalculateRomCrc32(romInfo);

// Write header
writer.Write(Encoding.ASCII.GetBytes(MAGIC));
writer.Write(VERSION);
writer.Write(GetPlatformId(romInfo.Format));
writer.Write((byte)0); // flags
writer.Write(romSize);
writer.Write(romCrc32);  // Was placeholder, now real value!
```

### Step 3: Read Existing Pansy Header

Location: New method in `PansyExporter.cs`

```csharp
/// <summary>
/// Read just the header from an existing Pansy file to check CRC.
/// </summary>
public static PansyHeader? ReadHeader(string path)
{
	if (!File.Exists(path)) return null;
	
	try
	{
		using var stream = File.OpenRead(path);
		using var reader = new BinaryReader(stream);
		
		byte[] magic = reader.ReadBytes(8);
		if (Encoding.ASCII.GetString(magic).TrimEnd('\0') != "PANSY")
			return null;
		
		return new PansyHeader
		{
			Version = reader.ReadUInt16(),
			Platform = reader.ReadByte(),
			Flags = reader.ReadByte(),
			RomSize = reader.ReadUInt32(),
			RomCrc32 = reader.ReadUInt32()
		};
	}
	catch
	{
		return null;
	}
}

public record PansyHeader(ushort Version, byte Platform, byte Flags, uint RomSize, uint RomCrc32);
```

### Step 4: Check CRC Before Update

Location: `PansyExporter.AutoExport()` method

```csharp
public static void AutoExport(RomInfo romInfo, MemoryType memoryType)
{
	if (!ConfigManager.Config.Debug.Integration.AutoExportPansy)
		return;
	
	// Calculate current ROM's CRC32
	uint currentCrc = CalculateRomCrc32(romInfo);
	
	// Get default path
	string defaultPath = GetPansyFilePath(romInfo.GetRomName());
	
	// Check if existing file has matching CRC
	string exportPath = defaultPath;
	var existingHeader = ReadHeader(defaultPath);
	
	if (existingHeader != null && existingHeader.RomCrc32 != 0)
	{
		if (existingHeader.RomCrc32 != currentCrc)
		{
			// CRC mismatch! Create separate file for this ROM version
			string baseName = Path.GetFileNameWithoutExtension(romInfo.GetRomName());
			exportPath = Path.Combine(
				ConfigManager.DebuggerFolder,
				$"{baseName}-{currentCrc:X8}.pansy"
			);
			
			System.Diagnostics.Debug.WriteLine(
				$"[Pansy] CRC mismatch: existing={existingHeader.RomCrc32:X8}, " +
				$"current={currentCrc:X8}. Using: {exportPath}");
		}
	}
	
	Export(exportPath, romInfo, memoryType, currentCrc);
}
```

### Step 5: Update Export Method Signature

```csharp
public static bool Export(string path, RomInfo romInfo, MemoryType memoryType, uint? crc32Override = null)
{
	// Use provided CRC or calculate
	uint romCrc32 = crc32Override ?? CalculateRomCrc32(romInfo);
	
	// ... rest of export logic
}
```

### Step 6: Configuration Option

Location: `UI/Config/IntegrationConfig.cs`

```csharp
// How to handle CRC mismatches
[Reactive] public CrcMismatchBehavior PansyCrcMismatchBehavior { get; set; } = CrcMismatchBehavior.CreateSeparateFile;

public enum CrcMismatchBehavior
{
	CreateSeparateFile,  // Create {name}-{crc}.pansy
	Overwrite,		   // Overwrite anyway (dangerous!)
	Skip				 // Don't save anything
}
```

### Step 7: UI Configuration

Location: `UI/Debugger/Windows/DebuggerConfigWindow.axaml`

```xml
<c:OptionSection Header="{l:Translate lblPansyFileIntegrity}">
	<TextBlock Text="{l:Translate lblCrcMismatchBehavior}" Margin="0 5 0 0" />
	<ComboBox SelectedItem="{Binding Integration.PansyCrcMismatchBehavior}" 
			  ItemsSource="{Binding CrcMismatchBehaviors}"
			  Margin="20 5 0 0" Width="200" />
</c:OptionSection>
```

## Testing Plan

### Test Case 1: Clean Start

1. Delete all .pansy files
2. Load original ROM
3. Play and close
4. Verify .pansy created with correct CRC in header

### Test Case 2: Same ROM Reload

1. Load same ROM again
2. Play more
3. Verify same .pansy file updated (CRC matches)

### Test Case 3: Hacked ROM

1. Load hacked/translated version (different CRC)
2. Play and close
3. Verify NEW .pansy file created with `-{CRC}.pansy` suffix
4. Verify ORIGINAL .pansy file unchanged

### Test Case 4: Legacy Files

1. Use old .pansy file with CRC32 = 0 (placeholder)
2. Load any ROM
3. Verify file gets CRC populated (migration)

## Migration Plan

For existing .pansy files with CRC32 = 0:

1. When loading, treat CRC32=0 as "unknown/unverified"
2. First update will populate the correct CRC
3. Log warning if CRC would change from non-zero

## Risks

1. **CRC Calculation Performance:** May be slow for large ROMs - cache result
2. **Headered ROMs:** NES ROMs have iNES header - need to hash PRG only
3. **User Confusion:** User may not understand why separate files created

## Timeline

- Day 1: Implement CRC calculation and header reading
- Day 2: Implement CRC check and file path logic
- Day 3: UI configuration and testing
- Day 4: Migration logic and edge cases

## Related Issues

- Nexen #2: ROM Hash Verification
- Pansy format already has RomCrc32 field (just unused)
