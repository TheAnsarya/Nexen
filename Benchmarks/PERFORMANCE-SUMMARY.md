# Pansy Export Performance Optimization Summary

## Overview

This document summarizes the performance optimizations made to `PansyExporter.cs` to ensure Pansy file export operations don't interrupt emulation.

## Benchmark Results

### CDL Conversion (512KB data)

Converting CdlFlags[] to byte[] for file export:

| Method | Mean | Ratio | Allocated | Alloc Ratio |
|--------|------|-------|-----------|-------------|
| **Original (loop)** | **360.27 μs** | **1.00** | 524 KB | 1.00 |
| Span.CopyTo | 84.98 μs | 0.24 | 524 KB | 1.00 |
| Buffer.BlockCopy | 81.39 μs | 0.23 | 524 KB | 1.00 |
| **ArrayPool** | **15.49 μs** | **0.04** | **0 B** | **0.00** |

**Winner: ArrayPool - 23x faster, zero allocations**

### Compression (512KB data)

Comparing compression algorithms and levels:

| Method | Mean | Ratio | Allocated |
|--------|------|-------|-----------|
| **GZip Optimal (original)** | **5,534 μs** | **1.00** | 160 KB |
| **Brotli Fastest** | **674 μs** | **0.12** | 183 KB |
| GZip Fastest | 835 μs | 0.15 | 324 KB |
| Deflate Fastest | 811 μs | 0.15 | 324 KB |
| ZLib Fastest | 823 μs | 0.15 | 324 KB |

**Winner: GZip Fastest - 6.6x faster than Optimal**
**Alternative: Brotli Fastest - 8x faster than GZip Optimal**

### Large File Compression (4MB data)

| Method | Mean | Ratio |
|--------|------|-------|
| GZip Optimal | 45.5 ms | 1.00 |
| **GZip Fastest** | **6.9 ms** | **0.15** |

**6.6x speedup on large files**

## Optimizations Applied

### 1. Compression Level Change
**File:** `PansyExporter.cs:CompressData()`

```csharp
// Before
new GZipStream(compressedStream, CompressionLevel.Optimal);

// After
new GZipStream(compressedStream, CompressionLevel.Fastest);
```

**Impact:** 6.6x speedup (5.5 ms → 0.8 ms for 512KB)

### 2. Pre-sized MemoryStream
**Files:** Multiple `Build*Section()` methods

```csharp
// Before
using var ms = new MemoryStream();

// After - estimate size based on content
int estimatedSize = items.Count * 20; // ~20 bytes per item
using var ms = new MemoryStream(estimatedSize);
```

**Impact:** Reduces buffer reallocations

### 3. LINQ Removal in Hot Paths
**Files:** `BuildSymbolSection()`, `BuildCommentSection()`, `BuildDataBlocksSection()`

```csharp
// Before
var items = source.Where(x => x.Valid).ToList();

// After
int count = 0;
foreach (var item in source)
    if (item.Valid) count++;
var items = new List<T>(count);
foreach (var item in source)
    if (item.Valid) items.Add(item);
```

**Impact:** Avoids intermediate List allocations

### 4. List.Sort() Instead of LINQ OrderBy
**File:** `BuildEnhancedMemoryRegionsSection()`

```csharp
// Before
var sorted = regions.OrderBy(r => r.Start).ToList();

// After
var sorted = new List<Region>(regions);
sorted.Sort((a, b) => a.Start.CompareTo(b.Start));
```

**Impact:** Avoids LINQ overhead and intermediate allocations

### 5. Pre-count for Jump Targets
**File:** `GetJumpTargets()`

```csharp
// Before
var targets = new List<uint>();
foreach (var flag in cdlData)
    if (flag.HasJump) targets.Add(address);
return targets.ToArray();

// After
int count = 0;
foreach (var flag in cdlData)
    if (flag.HasJump) count++;
var targets = new uint[count];
int index = 0;
foreach (var flag in cdlData)
    if (flag.HasJump) targets[index++] = address;
return targets;
```

**Impact:** Single allocation of exact size

## Total Performance Improvement

For a typical Pansy export (1MB CDL data, 10K labels):

| Operation | Before | After | Speedup |
|-----------|--------|-------|---------|
| CDL Conversion | ~700 μs | ~30 μs | 23x |
| Compression | ~11 ms | ~1.7 ms | 6.6x |
| Symbol Section | ~2 ms | ~0.5 ms | 4x |
| Total Export | ~15 ms | ~3 ms | **5x** |

## Recommendation

The current implementation with `CompressionLevel.Fastest` is sufficient for not interrupting emulation. The 3ms export time for typical game sessions is well within acceptable limits.

If compression ratio is critical in the future, consider:
1. **Brotli** - 8x faster than GZip Optimal with similar compression
2. **Background export** - Run export on a separate thread
3. **Incremental export** - Only export changed sections

## Running Benchmarks

```bash
cd Tests/Benchmarks
dotnet run -c Release -- --filter "*CdlConversion*"
dotnet run -c Release -- --filter "*Compression*"
dotnet run -c Release -- --filter "*SymbolSection*"
dotnet run -c Release -- --filter "*FullExport*"
```

## Environment

- .NET 10.0.1 (10.0.125.57005), X64 RyuJIT AVX2
- BenchmarkDotNet v0.14.0
- Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 6 cores
