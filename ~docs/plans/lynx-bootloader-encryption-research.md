# Atari Lynx Bootloader Encryption Research

> Technical specification of the Lynx boot ROM encryption/decryption system.
> Research compiled from libretro-handy, Mednafen, AtariAge forums, and Epyx documentation.

---

## Executive Summary

The Atari Lynx uses **RSA encryption** (not XOR or LFSR) to protect the initial bootloader code on cartridges. The boot ROM contains a decryption routine that uses Montgomery multiplication to perform modular exponentiation efficiently on the 65C02 CPU.

### Key Facts

| Property | Value |
|----------|-------|
| **Algorithm** | RSA (modular exponentiation) |
| **Formula** | `PLAINTEXT = ENCRYPTED^3 mod PUBLIC_KEY` |
| **Public Exponent** | 3 (fixed, for fast decryption) |
| **Block Size** | 51 bytes |
| **Key Size** | 408 bits (51 bytes × 8) |
| **Encrypted Region** | First 256–520 bytes of cartridge (depends on loader) |
| **Purpose** | Anti-piracy, cartridge authentication |

---

## 1. Boot Process Overview

### Sequence

1. **Power On**: CPU executes from reset vector in boot ROM ($FE00–$FFFF)
2. **Hardware Init**: Boot ROM initializes timers, display, MAPCTL
3. **Read Encrypted Data**: Reads encrypted blocks from cartridge address 0
4. **Decrypt**: RSA decryption using Montgomery multiplication
5. **Copy to RAM**: Decrypted loader placed at $0200
6. **Execute Loader**: Jump to $0200, which loads the rest of the game

### Boot ROM Entry Points (HLE Reference)

From `libretro-handy/lynx/system.cpp`:

| Address | Function | Description |
|---------|----------|-------------|
| `$FE00` | `HLE_BIOS_FE00` | Block select (sets cart shifter) |
| `$FE19` | `HLE_BIOS_FE19` | Main decryption entry point |
| `$FE4A` | `HLE_BIOS_FE4A` | Load encrypted blocks from cart |
| `$FF80` | `HLE_BIOS_FF80` | Initial jump from reset vector |

---

## 2. RSA Encryption Algorithm

### Mathematical Foundation

Standard RSA:

- Encryption: `C = M^e mod n` (with private key `e`)
- Decryption: `M = C^d mod n` (with public key `d`)

Lynx uses:

- **Public exponent (d)**: 3 (hardcoded, not stored)
- **Public modulus (n)**: 51-byte value stored in boot ROM

The formula is:

```text
PLAINTEXT = ENCRYPTED³ mod PUBLIC_KEY
```

### Why Exponent 3?

An exponent of 3 was chosen for performance:

- Requires only 2 Montgomery multiplications: `B² = B × B`, then `B³ = B × B²`
- Minimizes boot time on the 4 MHz 65C02
- Sufficient security for a game console (not banking-grade)

---

## 3. Public Key (Modulus)

The 51-byte RSA public modulus stored in the Lynx boot ROM:

```cpp
const unsigned char lynx_public_mod[51] = {
    0x35, 0xB5, 0xA3, 0x94, 0x28, 0x06, 0xD8, 0xA2,
    0x26, 0x95, 0xD7, 0x71, 0xB2, 0x3C, 0xFD, 0x56,
    0x1C, 0x4A, 0x19, 0xB6, 0xA3, 0xB0, 0x26, 0x00,
    0x36, 0x5A, 0x30, 0x6E, 0x3C, 0x4D, 0x63, 0x38,
    0x1B, 0xD4, 0x1C, 0x13, 0x64, 0x89, 0x36, 0x4C,
    0xF2, 0xBA, 0x2A, 0x58, 0xF4, 0xFE, 0xE1, 0xFD,
    0xAC, 0x7E, 0x79
};
```

As a 408-bit integer (big-endian):

```text
0x35B5A39428​06D8A22695​D771B23CFD​561C4A19B6​A3B0260036​5A306E3C4D​
63381BD41C​13648936​4CF2BA2A58​F4FEE1FDAC​7E79
```

This key is embedded in the Mikey chip's internal ROM and is the same for all Lynx units.

---

## 4. Encrypted Data Format

### Block Structure

The encrypted data at the start of a Lynx cartridge:

```text
Offset 0:    Block count byte (N = 0x100 - value gives number of 51-byte blocks)
Offset 1+:   N × 51 bytes of encrypted data
```

For standard 410-byte loader:

- Block count byte: `0xF8` → N = `0x100 - 0xF8` = 8 blocks
- Encrypted data: 8 × 51 = 408 bytes (plus 1 block count byte = 409 bytes)
- Decrypted output: 8 × 50 = 400 bytes (one byte per block is overhead)

### Decryption Process

From `lynxdec.cpp`:

```cpp
// Calculate number of encrypted blocks
int blocks = 256 - encrypted[0];

// For each block:
for (int i = 0; i < blocks; i++) {
    // Read 51 bytes of encrypted data
    unsigned char block[51];
    memcpy(block, &encrypted[1 + i * 51], 51);
    
    // Compute B³ mod N using Montgomery multiplication
    unsigned char squared[51], cubed[51];
    lynx_mont(squared, block, block, public_mod, 51);    // B² = B × B
    lynx_mont(cubed, block, squared, public_mod, 51);    // B³ = B × B²
    
    // Accumulate and output 50 bytes (obfuscation step)
    for (int j = 50; j > 0; j--) {
        accumulator += cubed[j];
        accumulator &= 0xFF;
        *output++ = accumulator;
    }
}
```

---

## 5. Montgomery Multiplication

The boot ROM uses Montgomery multiplication for efficient modular arithmetic without division. This is crucial for the 65C02, which lacks hardware division.

### Algorithm Overview

Montgomery multiplication computes `L = M × N mod modulus` using only additions, subtractions, and shifts:

```cpp
void lynx_mont(uint8_t* L,            // result
               const uint8_t* M,       // multiplicand
               const uint8_t* N,       // multiplier
               const uint8_t* modulus, // public key
               int length)             // 51
{
    memset(L, 0, length);
    
    for (int i = 0; i < length; i++) {
        uint8_t n_byte = N[i];
        
        for (int j = 0; j < 8; j++) {
            // L = L × 2
            double_value(L, length);
            
            bool bit = (n_byte & 0x80) != 0;
            n_byte <<= 1;
            
            if (bit) {
                // L += M
                plus_equals_value(L, M, length);
                // L -= modulus (with carry handling)
                minus_equals_value(L, modulus, length);
                // If still >= modulus, subtract again
                if (carry)
                    minus_equals_value(L, modulus, length);
            } else {
                // L -= modulus (normalize)
                minus_equals_value(L, modulus, length);
            }
        }
    }
}
```

### Helper Functions

```cpp
// Double a big integer (shift left by 1)
void double_value(uint8_t* result, int length) {
    int carry = 0;
    for (int i = length - 1; i >= 0; i--) {
        int tmp = 2 * result[i] + carry;
        result[i] = tmp & 0xFF;
        carry = tmp >> 8;
    }
}

// result -= value, returns 1 if no borrow, 0 if borrow
int minus_equals_value(uint8_t* result, const uint8_t* value, int length) {
    int borrow = 0;
    for (int i = length - 1; i >= 0; i--) {
        int tmp = result[i] - value[i] - borrow;
        if (tmp < 0) {
            result[i] = tmp + 256;
            borrow = 1;
        } else {
            result[i] = tmp;
            borrow = 0;
        }
    }
    return borrow ? 0 : 1;  // Return carry
}

// result += value
void plus_equals_value(uint8_t* result, const uint8_t* value, int length) {
    int carry = 0;
    for (int i = length - 1; i >= 0; i--) {
        int tmp = result[i] + value[i] + carry;
        carry = tmp >= 256 ? 1 : 0;
        result[i] = tmp & 0xFF;
    }
}
```

---

## 6. Post-Decryption Obfuscation

After RSA decryption, an additional obfuscation step is applied:

```cpp
int accumulator = 0;
for (int i = 50; i > 0; i--) {  // Skip byte 0 of each block
    accumulator += decrypted_block[i];
    accumulator &= 0xFF;
    output[output_idx++] = accumulator;
}
```

Each block produces 50 output bytes (not 51). The accumulator carries across blocks. The final accumulator value must be 0 for valid data (checksum).

---

## 7. Private Key Status

### Historical Context

The original Lynx development kit used an Amiga-based encryption tool. The private key was protected using a complex multi-floppy system:

1. The private exponent was split across **three keyfile floppies** using XOR
2. All three floppies were required to reconstruct the key in Amiga RAM
3. The key was never stored in plaintext on any single disk

From AtariAge (karri, 2009):
> "The private key used for signing the binary is signed by another key that is lost forever. At the signing process we have only the public key of the 'lost forever' key. With this public key you can retrieve the private key. But the private key will only be retrieved in Amiga RAM."

### Private Key Recovery

Despite the original security measures, the private exponent **was recovered**. The three keyfile blocks were obtained from the Lynx community (likely from Atari insiders) and when XOR'd together, they produce the working private exponent.

Source: [42Bastian/lynx-encryption-tools](https://github.com/42Bastian/lynx-encryption-tools/blob/master/keys.h) (David Huseby, zlib license)

The private exponent (51 bytes / 408 bits):

```cpp
const uint8_t lynx_private_exp[51] = {
    0x23, 0xCE, 0x6D, 0x0D, 0x70, 0x04, 0x90, 0x6C,
    0x19, 0xB9, 0x3A, 0x4B, 0xCC, 0x28, 0xA8, 0xE4,
    0x12, 0xDC, 0x11, 0x24, 0x6D, 0x20, 0x19, 0x55,
    0x79, 0x87, 0xAB, 0x5C, 0xA8, 0x18, 0xA3, 0xD3,
    0xC8, 0xE3, 0x27, 0x6D, 0x42, 0x70, 0xCB, 0x80,
    0x21, 0xD6, 0xBD, 0xA4, 0x29, 0x6D, 0x47, 0xB1,
    0xE5, 0xE2, 0xA3
};
```

### Encryption Formula

With the private exponent known, encryption is:

```text
ENCRYPTED = PLAINTEXT^d mod PUBLIC_KEY
```

Where `d` is the 408-bit private exponent above.

### Legacy Workaround (Still Useful)

Harry Dodgson's pre-encrypted bootloaders (410 bytes) remain useful for:

- Quick homebrew development (no encryption step needed)
- Compatibility testing
- Three variants exist for different cart sizes (512B/1KB/2KB sectors)

---

## 8. Existing Implementations

### Decryption (Available)

| Location | Language | License |
|----------|----------|---------|
| `libretro-handy/lynx/lynxdec.cpp` | C | zlib |
| `libretro-handy/lynx/system.cpp` (HLE) | C++ | zlib |
| Mednafen/Beetle-Lynx | C++ | GPL-2.0 |

### Encryption (Now Available!)

| Location | Language | License |
|----------|----------|---------|
| [42Bastian/lynx-encryption-tools](https://github.com/42Bastian/lynx-encryption-tools) | C | zlib |
| **Nexen** `Core/Lynx/LynxDecrypt.h` | C++ | MIT |

---

## 9. Nexen Implementation (COMPLETED)

### Implementation Status: ✅ COMPLETE

The full encryption/decryption API is implemented in `Core/Lynx/LynxDecrypt.h/.cpp`.

### API Summary

```cpp
namespace LynxCrypto {
    // High-Level API
    DecryptResult Decrypt(std::span<const uint8_t> encrypted);
    EncryptResult Encrypt(std::span<const uint8_t> plaintext);
    bool Validate(std::span<const uint8_t> encrypted);
    size_t GetDecryptedSize(std::span<const uint8_t> encrypted);
    size_t GetBlockCount(std::span<const uint8_t> encrypted);
    
    // Low-Level API
    void DecryptBlock(output, block, accumulator);
    void EncryptBlock(output, block, accumulator);
    void MontgomeryMultiply(result, multiplicand, multiplier, modulus);
    void ModularExponentiate(result, base, exponent, modulus);
    
    // Constants
    constexpr uint8_t PublicModulus[51];
    constexpr uint8_t PrivateExponent[51];
    constexpr size_t BlockSize = 51;
    constexpr size_t OutputBytesPerBlock = 50;
}
```

### Files Created

| File | Purpose |
|------|---------|
| `Core/Lynx/LynxDecrypt.h` | Full API header with XML documentation |
| `Core/Lynx/LynxDecrypt.cpp` | Montgomery multiplication, RSA encrypt/decrypt |
| `Core.Tests/Lynx/LynxDecryptTests.cpp` | 45+ unit tests |
| `Core.Benchmarks/Lynx/LynxDecryptBench.cpp` | Performance benchmarks |
| `Core.Benchmarks/Lynx/LynxDecryptBench.cpp` | Performance benchmarks |

---

## 10. Test Vectors

### Known Encrypted/Decrypted Pairs

To validate the implementation, extract encrypted data from a commercial cart:

```cpp
// First 52 bytes from a commercial .lnx file (after 64-byte header)
// Run through decryption and verify output matches expected loader
```

Harry Dodgson's 410-byte encrypted loader (available in new_bll SDK) can serve as a test vector.

---

## 11. References

### Primary Sources

1. **libretro-handy `lynxdec.cpp`** — Complete decryption implementation
   - Source: <https://github.com/libretro/libretro-handy/blob/master/lynx/lynxdec.cpp>
   - License: zlib

2. **AtariAge "Lynx Encryption?" Thread** — Technical discussions with karri
   - URL: <https://forums.atariage.com/topic/129030-lynx-encryption>
   - Key contributors: karri, Wookie, Harry Dodgson

3. **Epyx Hardware Reference** — Boot ROM section
   - URL: <https://www.monlynx.de/lynx/lynx4.html>

### Secondary Sources

1. **new_bll SDK** — Contains pre-encrypted bootloaders
   - URL: <https://github.com/42Bastian/new_bll>

2. **cgexpo.com lnxcrypt.zip** — Original Amiga encryption tools (archived)

---

## Appendix A: Complete Decryption Code

Full reference implementation from libretro-handy (zlib license):

```cpp
#include <stdlib.h>
#include <string.h>

#define CHUNK_LENGTH 51

const unsigned char lynx_public_mod[CHUNK_LENGTH] = {
    0x35, 0xB5, 0xA3, 0x94, 0x28, 0x06, 0xD8, 0xA2,
    0x26, 0x95, 0xD7, 0x71, 0xB2, 0x3C, 0xFD, 0x56,
    0x1C, 0x4A, 0x19, 0xB6, 0xA3, 0xB0, 0x26, 0x00,
    0x36, 0x5A, 0x30, 0x6E, 0x3C, 0x4D, 0x63, 0x38,
    0x1B, 0xD4, 0x1C, 0x13, 0x64, 0x89, 0x36, 0x4C,
    0xF2, 0xBA, 0x2A, 0x58, 0xF4, 0xFE, 0xE1, 0xFD,
    0xAC, 0x7E, 0x79
};

void double_value(unsigned char* result, int length) {
    int x = 0;
    for (int i = length - 1; i >= 0; i--) {
        x += 2 * result[i];
        result[i] = (unsigned char)(x & 0xFF);
        x >>= 8;
    }
}

int minus_equals_value(unsigned char* result, const unsigned char* value, int length) {
    unsigned char* tmp = (unsigned char*)calloc(1, length);
    int x = 0;
    for (int i = length - 1; i >= 0; i--) {
        x += result[i] - value[i];
        tmp[i] = (unsigned char)(x & 0xFF);
        x >>= 8;
    }
    if (x >= 0) {
        memcpy(result, tmp, length);
        free(tmp);
        return 1;  // Carry (result >= value)
    }
    free(tmp);
    return 0;  // No carry (result < value)
}

void plus_equals_value(unsigned char* result, const unsigned char* value, int length) {
    int carry = 0;
    for (int i = length - 1; i >= 0; i--) {
        int tmp = result[i] + value[i] + carry;
        carry = (tmp >= 256) ? 1 : 0;
        result[i] = (unsigned char)tmp;
    }
}

void lynx_mont(unsigned char* L, const unsigned char* M, const unsigned char* N,
               const unsigned char* modulus, int length) {
    memset(L, 0, length);
    
    for (int i = 0; i < length; i++) {
        unsigned char tmp = N[i];
        for (int j = 0; j < 8; j++) {
            double_value(L, length);
            unsigned char increment = (tmp & 0x80) / 0x80;
            tmp <<= 1;
            
            if (increment != 0) {
                plus_equals_value(L, M, length);
                int carry = minus_equals_value(L, modulus, length);
                if (carry != 0)
                    minus_equals_value(L, modulus, length);
            } else {
                minus_equals_value(L, modulus, length);
            }
        }
    }
}

int decrypt_block(int accumulator, unsigned char* result, const unsigned char* encrypted,
                  const unsigned char* public_mod, int length) {
    unsigned char* A = (unsigned char*)calloc(1, length);
    unsigned char* B = (unsigned char*)calloc(1, length);
    unsigned char* TMP = (unsigned char*)calloc(1, length);
    
    // Copy encrypted block in reverse order
    for (int i = length - 1; i >= 0; i--) {
        B[i] = *encrypted++;
    }
    
    // A = B² mod public_mod
    lynx_mont(A, B, B, public_mod, length);
    memcpy(TMP, A, length);
    
    // A = B³ mod public_mod
    lynx_mont(A, B, TMP, public_mod, length);
    
    // Accumulate and output
    for (int i = length - 1; i > 0; i--) {
        accumulator += A[i];
        accumulator &= 0xFF;
        *result++ = (unsigned char)accumulator;
    }
    
    free(A);
    free(B);
    free(TMP);
    return accumulator;
}

int decrypt_frame(unsigned char* result, const unsigned char* encrypted,
                  const unsigned char* public_mod, int length) {
    int accumulator = 0;
    int blocks = 256 - *encrypted++;
    
    for (int i = 0; i < blocks; i++) {
        accumulator = decrypt_block(accumulator, result, encrypted, public_mod, length);
        result += (length - 1);
        encrypted += length;
    }
    
    return blocks;
}

void lynx_decrypt(unsigned char* result, const unsigned char* encrypted, int length) {
    decrypt_frame(result, encrypted, lynx_public_mod, length);
}
```

---

## Appendix B: BS93 Homebrew Format

For homebrew development, the **BS93 format** bypasses encryption entirely:

- Header: 10 bytes (`"BS93"` magic + metadata)
- No encrypted section
- Code loads directly into RAM at $0200
- Only works with emulators in "homebrew mode" or HLE BIOS

Nexen already supports BS93 detection via:

```cpp
if (!strcmp(&clip[6], "BS93"))
    fileType = HANDY_FILETYPE_HOMEBREW;
```
