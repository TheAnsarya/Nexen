# Code Formatting Guidelines

This project uses **clang-format** for consistent C++ code formatting.

## Configuration

The formatting rules are defined in [.clang-format](.clang-format).

### Key Settings

| Setting | Value | Description |
| --------- | ------- | ------------- |
| **IndentWidth** | 4 | Use 4-space equivalent indentation |
| **UseTab** | ForIndentation | Tabs for indentation, spaces for alignment |
| **TabWidth** | 4 | Tab width is 4 spaces |
| **ColumnLimit** | 0 | No line length limit (don't reflow) |
| **BreakBeforeBraces** | Custom | K&R style - opening brace on same line |
| **PointerAlignment** | Left | Pointers attach to type: `int* ptr` |
| **SpaceAfterCStyleCast** | false | No space after casts: `(int)value` |

### Brace Style (K&R)

```cpp
// Functions - brace on next line
void Function()
{
	// code
}

// Control statements - brace on same line
if (condition) {
	// code
} else {
	// code
}

for (int i = 0; i < n; i++) {
	// code
}

// Classes/structs - brace on same line
class MyClass {
	// members
};
```

### Indentation

- **Tabs for indentation**, spaces for alignment
- Access modifiers (`public:`, `private:`) are not indented
- Case labels in switches are not indented from switch

```cpp
class Example {
public:
	void Method();
private:
	int _member;
};

switch (value) {
case 1:
	DoSomething();
	break;
default:
	break;
}
```

## Applying Formatting

### Format All Files

```powershell
# From repository root
Get-ChildItem -Path Core -Recurse -Include *.cpp,*.h | ForEach-Object {
    & "clang-format" -i $_.FullName
}
```

### Format Single File

```powershell
clang-format -i path/to/file.cpp
```

### Check Without Modifying

```powershell
clang-format --dry-run --Werror path/to/file.cpp
```

## Excluded Files

Third-party libraries are **not** formatted to maintain upstream compatibility:

- `Utilities/magic_enum.hpp`
- `Utilities/miniz.*`
- `Utilities/stb_vorbis.*`
- `Utilities/emu2413.*`
- `Utilities/ymfm/*`
- `Utilities/blip_buf.*`
- `Utilities/NTSC/*_ntsc*`

## IDE Integration

### Visual Studio

1. Tools → Options → Text Editor → C/C++ → Formatting
2. Enable "Use clang-format"
3. The `.clang-format` file will be auto-detected

### VS Code

1. Install "C/C++" extension
2. Settings: Set `C_Cpp.clang_format_style` to `file`
3. Format on save or use Shift+Alt+F

## Notes

- All warnings from third-party libraries are expected and ignored
- The formatting is applied only to project-owned code
- Run formatting before committing to maintain consistency
