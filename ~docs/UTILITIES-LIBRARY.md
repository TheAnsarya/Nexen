# Shared/Utilities Library Documentation

The Utilities library provides foundational infrastructure for Nexen, including serialization, file I/O, string handling, compression, and platform abstraction.

## Overview

```text
Utilities/
├── Core Utilities
│   ├── Serializer.h/cpp	  - State serialization (save states, rewind)
│   ├── BitUtilities.h		- Bit manipulation (templated, zero-cost)
│   ├── HexUtilities.h/cpp	- Hex string conversion
│   ├── StringUtilities.h	 - String manipulation helpers
│   └── FastString.h		  - Stack-allocated string builder
│
├── File & Archive I/O
│   ├── VirtualFile.h/cpp	 - Unified file abstraction
│   ├── FolderUtilities.h/cpp - Directory operations
│   ├── ArchiveReader.h/cpp   - Archive extraction base
│   ├── ZipReader.h/cpp	   - ZIP file reading
│   ├── ZipWriter.h/cpp	   - ZIP file writing
│   └── SZReader.h/cpp		- 7-Zip archive reading
│
├── Compression
│   ├── CompressionHelper.h   - Compression API wrapper
│   └── miniz.h/cpp		   - DEFLATE implementation
│
├── Hashing & Validation
│   ├── CRC32.h/cpp		   - CRC32 checksum
│   ├── md5.h/cpp			 - MD5 hash
│   └── sha1.h/cpp			- SHA-1 hash
│
├── Platform Abstraction
│   ├── PlatformUtilities.h/cpp - OS-specific helpers
│   ├── UTF8Util.h/cpp		- UTF-8 string encoding
│   ├── PathUtil.h			- Path manipulation
│   └── Socket.h/cpp		  - Network socket wrapper
│
├── Threading & Synchronization
│   ├── SimpleLock.h/cpp	  - Mutex wrapper
│   └── AutoResetEvent.h/cpp  - Event signaling
│
├── Graphics Filters
│   ├── HQX/				   - HQnX filter
│   ├── KreedSaiEagle/		- Kreed/SAI/Eagle filters
│   ├── Scale2x/			  - Scale2x filter
│   ├── xBRZ/				 - xBRZ filter
│   ├── NTSC/				 - NTSC composite simulation
│   └── Video/				- Video output helpers
│
├── Audio
│   └── Audio/				- Audio processing
│
├── Images
│   ├── PNGHelper.h/cpp	   - PNG loading/saving
│   └── spng.h/c			  - Simple PNG library
│
├── Patching
│   └── Patches/			  - IPS/BPS patch support
│
└── Third-Party
	├── magic_enum.hpp		- Enum reflection
	├── kissfft.h			 - FFT library (audio)
	└── safe_ptr.h			- Thread-safe smart pointers
```

## Core Utilities

### Serializer (Serializer.h)

The Serializer class provides a key-value based serialization system used for save states and rewind functionality.

**Key Features:**

- Bidirectional: Same code for saving and loading
- Multiple formats: Binary (compact), Text (debug), Map (Lua API)
- Versioning support for backwards compatibility
- Nested object support via ISerializable interface

**Usage Example:**

```cpp
class MyState : public ISerializable {
	uint32_t _value;
	uint8_t _data[256];
	
	void Serialize(Serializer& s) override {
		SV(_value);		   // Stream variable with name
		SVArray(_data, 256);  // Stream array with count
	}
};

// Saving
Serializer saver(1, true, SerializeFormat::Binary);
myState.Serialize(saver);
saver.SaveTo(outputStream);

// Loading
Serializer loader(1, false, SerializeFormat::Binary);
loader.LoadFrom(inputStream);
myState.Serialize(loader);
```

**Macros:**

- `SV(var)` - Stream variable with automatic name
- `SVArray(arr, count)` - Stream array with count
- `SVI(var)` - Stream indexed variable
- `SVVector(var)` - Stream std::vector

### BitUtilities (BitUtilities.h)

Template-based bit manipulation utilities with compile-time optimization.

**Key Methods:**

```cpp
// Set 8 bits at position 16 (bits 16-23)
BitUtilities::SetBits<16>(dst, 0x42);

// Get 8 bits from position 8 (bits 8-15)
uint8_t value = BitUtilities::GetBits<8>(src);
```

**Benefits:**

- Zero runtime overhead (compile-time templates)
- `__forceinline` ensures no function call overhead
- Type-safe with templates

### HexUtilities (HexUtilities.h)

Cached hex string conversion for performance.

**Key Methods:**

```cpp
string ToHex(uint8_t value);	  // "AB"
string ToHex(uint16_t value);	 // "1234"
string ToHex(uint32_t value);	 // Variable length
string ToHex24(int32_t value);	// "00FFFF" (24-bit addresses)
int FromHex(string hex);		  // Parse hex string
```

### FastString (FastString.h)

Stack-allocated string builder for high-performance string concatenation.

**Features:**

- No heap allocation for typical strings (< ~400 chars)
- StringBuilder-like API
- Automatic overflow to heap if needed

### StringUtilities (StringUtilities.h)

Common string operations:

- `Split(string, delimiter)` - Split string
- `Trim(string)` - Remove whitespace
- `ToLower(string)` / `ToUpper(string)`
- `Contains(string, substring)`

## File I/O

### VirtualFile (VirtualFile.h)

Unified file abstraction supporting:

- Regular filesystem files
- Files inside ZIP archives
- Files inside 7-Zip archives
- Memory buffers

**Usage:**

```cpp
VirtualFile file("game.zip/rom.nes");
vector<uint8_t> data;
file.ReadAllBytes(data);
```

### Archive Support

- **ZipReader/ZipWriter** - ZIP file handling (miniz-based)
- **SZReader** - 7-Zip archive extraction
- **ArchiveReader** - Common base class

## Hashing

### CRC32 (CRC32.h)

```cpp
uint32_t crc = CRC32::GetCRC(data, size);
uint32_t crc = CRC32::GetCRC(filename);
```

### MD5/SHA1

```cpp
string hash = MD5::GetMd5Hash(data, size);
string hash = SHA1::GetSha1Hash(data, size);
```

## Threading

### SimpleLock (SimpleLock.h)

Cross-platform mutex wrapper with RAII pattern:

```cpp
SimpleLock lock;

void ThreadSafeMethod() {
	auto guard = lock.AcquireSafe();
	// Protected code
} // Automatically released
```

### AutoResetEvent (AutoResetEvent.h)

Thread signaling for producer/consumer patterns:

```cpp
AutoResetEvent event;
event.Wait();	// Block until signaled
event.Signal();  // Wake waiting thread
```

## Graphics Filters

Supported upscaling filters:

- **HQ2x/HQ3x/HQ4x** - High Quality Magnification
- **Scale2x/Scale3x** - Edge-preserving scaling
- **xBRZ** - xBR-based scaling
- **Kreed SAI/Eagle** - Classic filters
- **NTSC** - Composite video simulation

## Platform Abstraction

### PlatformUtilities (PlatformUtilities.h)

OS-specific functionality:

- File dialogs
- Clipboard access
- System paths
- Display information

### UTF8Util (UTF8Util.h)

UTF-8/UTF-16/UTF-32 conversion for cross-platform text handling.

## Dependencies

**Third-Party Libraries:**

- `miniz` - DEFLATE compression (embedded)
- `spng` - Simple PNG (embedded)
- `magic_enum` - Enum reflection (header-only)
- `kissfft` - FFT (header-only)

## Thread Safety

Most utilities are **not thread-safe** by default. Use `SimpleLock` for synchronization when needed. Key exceptions:

- `CRC32::GetCRC` is thread-safe
- `MD5`/`SHA1` hash functions are thread-safe

## Performance Considerations

- **HexUtilities**: Uses cached strings for byte values
- **BitUtilities**: Templates ensure compile-time optimization
- **FastString**: Stack allocation avoids heap for small strings
- **Serializer**: Binary format is optimized for size and speed

## Related Documentation

- [CODE-DOCUMENTATION-STYLE.md](CODE-DOCUMENTATION-STYLE.md) - Documentation standards
- [CPP-DEVELOPMENT-GUIDE.md](CPP-DEVELOPMENT-GUIDE.md) - C++ coding standards
