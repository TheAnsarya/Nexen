# 🌼 Pansy Export - Tutorials

Step-by-step tutorials for common Pansy export workflows.

## Tutorial 1: First Export

**Goal:** Export your first Pansy file with labels and comments.

### Prerequisites

- Nexen installed
- A ROM file loaded
- Some labels/comments added

### Steps

1. **Load a ROM**
   - File → Open → Select your ROM

2. **Open the Debugger**
   - Debug → Debugger (or press F12)

3. **Add Some Labels**
   - Navigate to a known address (e.g., Reset vector)
   - Right-click → Add Label
   - Enter a name like "Reset_Handler"
   - Add a comment: "Entry point after power-on"

4. **Add More Labels**
   - Find NMI handler, add label "NMI_Handler"
   - Find main loop, add label "MainLoop"

5. **Export to Pansy**
   - Debug → Integration → Export Pansy File...
   - Choose save location
   - Click Save

6. **Verify Export**
   - Open the `.pansy` file location
   - File should be ~1-10KB depending on content

### What's in the File?

Your Pansy file now contains:

- ✅ All your labels with addresses
- ✅ All your comments
- ✅ Code/data markings from CDL (if any)
- ✅ ROM identification (CRC, size, platform)

---

## Tutorial 2: Setting Up Folder Storage

**Goal:** Configure automatic syncing with external tools.

### Why Folder Storage?

- **Human-readable files** - MLB files are plain text
- **Version control friendly** - Works with Git
- **External editor support** - Edit in VS Code, etc.
- **Automatic backups** - Version history

### Steps

1. **Enable Folder Storage**
   - Options → Debugger Options
   - Go to "Integration" tab
   - Check "Use folder-based storage"

2. **Configure Sync Options**
   - Check "Auto-sync MLB files" (labels)
   - Check "Auto-sync CDL files" (code/data)
   - Optionally enable "Keep version history"

3. **Load a ROM**
   - A folder appears next to your ROM:

	 ```text
	 MyGame.nes
	 MyGame_debug/
	 ├── manifest.json
	 ├── deadbeef.mlb
	 └── deadbeef.cdl
	 ```

4. **Edit Externally**
   - Open the `.mlb` file in a text editor
   - Add a label: `PRG:$8000:MyLabel:`
   - Save the file

5. **Watch It Sync**
   - If "Auto-reload" is enabled, labels appear in Nexen
   - Otherwise, Debug → Integration → Reload from Folder

### MLB File Format

```text
; Comments start with semicolon
PRG:$8000:Reset:
PRG:$8003:MainLoop:
PRG:$FFFA:NMI_Vector:
SRAM:$6000:SaveData:
WRAM:$0000:ZeroPage:
```

---

## Tutorial 3: Importing Shared Labels

**Goal:** Import a Pansy file shared by the community.

### Scenario

Someone has shared their analysis of a game as a `.pansy` file. You want to use their labels while preserving your own work.

### Steps

1. **Back Up Your Work First**
   - Debug → Integration → Export Pansy File...
   - Save as "MyWork_backup.pansy"

2. **Import the Shared File**
   - Debug → Integration → Import Pansy File...
   - Select the community file

3. **Review Import Results**
   - Dialog shows: "Imported X labels, Y comments"
   - Check for any warnings

4. **Handle Conflicts**
   - If addresses overlap, your labels are preserved
   - New addresses from the file are added
   - Comments are merged (both kept if different)

5. **Verify in Label List**
   - Debug → Label List
   - Sort by address to see all labels
   - Look for any duplicates to clean up

### Merge Strategies

| Strategy | Behavior |
| ---------- | ---------- |
| Keep Mine | Your labels take priority |
| Keep Theirs | Imported labels take priority |
| Merge All | Keep both (may have duplicates) |
| Interactive | Ask for each conflict |

---

## Tutorial 4: Converting ca65 Debug Files

**Goal:** Import labels from a cc65/ca65 project.

### Prerequisites

- A compiled ca65 project with debug info
- The `.dbg` file from compilation
- Matching ROM file

### Steps

1. **Locate Your Debug File**
   - After `cl65 -g game.c -o game.nes`
   - You'll have `game.dbg`

2. **Open Matching ROM**
   - Load `game.nes` in Nexen

3. **Convert Debug File**
   - Debug → Integration → Convert to Pansy...
   - Select `game.dbg`
   - Format auto-detected as "ca65 Debug"

4. **Review Conversion**

   ```text
   Converting game.dbg...
   Found: 1,247 symbols
   Found: 89 segments
   Found: 342 source locations
   
   Output: game.pansy
   ```

5. **Import the Converted File**
   - Import dialog appears automatically
   - Or manually import the new `.pansy` file

### What Gets Converted?

| ca65 Feature | Pansy Equivalent |
| -------------- | ------------------ |
| Symbols (.global) | Labels |
| Local labels | Labels with scope prefix |
| Segments | Memory regions |
| Line info | Source file comments |
| Scopes | Label prefixes |

---

## Tutorial 5: Real-Time Sync Workflow

**Goal:** Edit labels in VS Code while Nexen updates live.

### Setup

1. **Enable File Watching**
   - Options → Debugger Options → Integration
   - Check "Watch files for external changes"
   - Check "Auto-reload on external change"

2. **Enable Folder Storage**
   - Check "Use folder-based storage"
   - Check "Auto-sync MLB files"

3. **Load Your ROM**
   - Debug folder is created
   - MLB file appears

### VS Code Workflow

1. **Open Debug Folder**
   - File → Open Folder → `MyGame_debug/`

2. **Install MLB Extension (optional)**
   - Search "Nexen Labels" in Extensions
   - Provides syntax highlighting

3. **Edit Labels**

   ```text
   ; Open deadbeef.mlb
   PRG:$8000:Reset:
   PRG:$8010:InitializeHardware:
   PRG:$8050:LoadTitleScreen:   ; Added!
   ```

4. **Save and Watch**
   - Press Ctrl+S
   - Nexen debugger updates within 1 second
   - New label appears in Label List

### Bidirectional Sync

Changes in Nexen also update the file:

1. Add a label in Nexen debugger
2. MLB file updates automatically
3. VS Code shows the change

---

## Tutorial 6: Batch Analysis Project

**Goal:** Set up a full game analysis project with version control.

### Project Structure

```text
MyGameAnalysis/
├── roms/
│   └── MyGame.nes		   # Your ROM (gitignore!)
├── debug/
│   └── MyGame_debug/		# Generated debug folder
│	   ├── manifest.json
│	   ├── *.mlb
│	   ├── *.cdl
│	   └── *.pansy
├── docs/
│   ├── rom-map.md
│   └── notes.md
├── .gitignore
└── README.md
```

### Setup Steps

1. **Create Project Folder**

   ```powershell
   mkdir MyGameAnalysis
   cd MyGameAnalysis
   mkdir roms debug docs
   ```

2. **Configure .gitignore**

   ```gitignore
   # ROMs (copyrighted)
   roms/*.nes
   roms/*.sfc
   
   # Keep debug folder but ignore CDL (large)
   debug/**/*.cdl
   
   # Keep Pansy and MLB files
   !debug/**/*.pansy
   !debug/**/*.mlb
   !debug/**/manifest.json
   ```

3. **Configure Nexen**
   - Set debug folder location to `debug/`
   - Enable folder storage
   - Enable version history

4. **Start Analysis**
   - Load ROM from `roms/`
   - Labels auto-save to `debug/`
   - Commit regularly!

### Git Workflow

```bash
# Initial commit
git add debug/ docs/
git commit -m "Initial analysis setup"

# After analysis session
git add debug/*.mlb debug/*.pansy
git commit -m "Add reset handler labels"

# Share with team
git push origin main
```

### Collaboration

Team members can:

1. Clone the repo
2. Add their own ROM file
3. Nexen loads labels from `debug/`
4. Make changes and push

---

## Tutorial 7: Recovering from Backup

**Goal:** Restore labels from a corrupted or lost state.

### If You Have Version History

1. **Open Debug Folder**
   - Navigate to `MyGame_debug/.history/`

2. **Find Backup**
   - Files named: `20240115_143022.mlb`
   - Sorted by timestamp

3. **Preview Contents**
   - Open in text editor to verify

4. **Restore**
   - Copy backup to main file
   - Or: Debug → Integration → Import from History...

### If You Have a Pansy Backup

1. **Locate Backup**
   - Find your `MyGame_backup.pansy`

2. **Clear Current State (optional)**
   - Debug → Labels → Clear All
   - Only if you want full replacement

3. **Import Backup**
   - Debug → Integration → Import Pansy File...
   - Select backup file

### If You Only Have MLB

1. **Load MLB Directly**
   - Debug → Labels → Import Labels
   - Select the `.mlb` file

### Prevention Tips

- Enable version history (keeps last 10+ versions)
- Use folder storage with Git
- Export Pansy before major changes
- Enable auto-save

---

## Tutorial 8: Multi-ROM Project

**Goal:** Share labels across multiple versions of a game (US, EU, JP).

### Approach: Relative Addresses

For games where code is at same offsets:

1. **Create Master Pansy**
   - Analyze primary version (e.g., US)
   - Export as `MyGame_Master.pansy`

2. **Import to Other Versions**
   - Load EU ROM
   - Import master file
   - Nexen adjusts for header differences

### Approach: Separate + Shared

For games with code differences:

```text
MyGame_Project/
├── shared/
│   └── common_labels.mlb	# Shared labels
├── us/
│   └── us_specific.mlb
├── eu/
│   └── eu_specific.mlb
└── jp/
	└── jp_specific.mlb
```

Configure Nexen to load:

1. Shared labels first
2. Region-specific labels second

### Approach: Conversion Script

```python
# offset_converter.py
def convert_offsets(input_mlb, output_mlb, offset_delta):
	"""Convert labels from one version to another."""
	with open(input_mlb) as f:
		for line in f:
			if line.startswith('PRG:$'):
				# Parse and adjust address
				addr = int(line.split(':')[1][1:], 16)
				new_addr = addr + offset_delta
				# Write adjusted line
```

---

## Quick Reference

### Export Checklist

- [ ] Labels complete for session
- [ ] Comments added for complex code
- [ ] CDL data current (run game first)
- [ ] Compression enabled for sharing
- [ ] Cross-refs included if needed

### Import Checklist

- [ ] ROM CRC matches
- [ ] Backup current work
- [ ] Review import filters
- [ ] Check for conflicts
- [ ] Verify label count

### Folder Storage Checklist

- [ ] Folder storage enabled
- [ ] MLB sync enabled
- [ ] Version history enabled
- [ ] File watching enabled (if needed)
- [ ] .gitignore configured
