```

BenchmarkDotNet v0.15.8, Windows 10 (10.0.19045.6466/22H2/2022Update)
Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 1 CPU, 12 logical and 6 physical cores
.NET SDK 10.0.102
  [Host]   : .NET 10.0.3 (10.0.3, 10.0.326.7603), X64 RyuJIT x86-64-v3
  ShortRun : .NET 10.0.3 (10.0.3, 10.0.326.7603), X64 RyuJIT x86-64-v3

Job=ShortRun  IterationCount=3  LaunchCount=1  
WarmupCount=3  

```
| Method                            | Mean         | Error        | StdDev     | Ratio | RatioSD | Gen0     | Gen1     | Gen2     | Allocated | Alloc Ratio |
|---------------------------------- |-------------:|-------------:|-----------:|------:|--------:|---------:|---------:|---------:|----------:|------------:|
| CdlConversion_ArrayPool_Medium    |     15.32 μs |     5.738 μs |   0.315 μs |  0.04 |    0.00 |        - |        - |        - |         - |        0.00 |
| CdlConversion_BlockCopy_Medium    |     79.09 μs |    38.208 μs |   2.094 μs |  0.22 |    0.01 |  30.3955 |  30.3955 |  30.3955 |  524317 B |        1.00 |
| CdlConversion_SpanCopy_Medium     |     81.39 μs |    46.190 μs |   2.532 μs |  0.23 |    0.01 |  30.5176 |  30.5176 |  30.5176 |  524316 B |        1.00 |
| CdlConversion_Original_Medium     |    355.32 μs |    17.905 μs |   0.981 μs |  1.00 |    0.00 |  41.0156 |  41.0156 |  41.0156 |  524325 B |        1.00 |
|                                   |              |              |            |       |         |          |          |          |           |             |
| Compression_BrotliFastest_Medium  |    671.47 μs |   227.364 μs |  12.463 μs |  0.12 |    0.00 |  18.5547 |   3.9063 |   3.9063 |  187590 B |        1.15 |
| Compression_DeflateFastest_Medium |    719.64 μs |    75.234 μs |   4.124 μs |  0.13 |    0.00 |  37.1094 |   6.8359 |   5.8594 |  331626 B |        2.03 |
| Compression_GzipFastest_Medium    |    721.42 μs |    81.127 μs |   4.447 μs |  0.13 |    0.00 |  37.1094 |   6.8359 |   5.8594 |  331674 B |        2.03 |
| Compression_ZLibFastest_Medium    |    724.03 μs |    84.196 μs |   4.615 μs |  0.13 |    0.00 |  37.1094 |   6.8359 |   5.8594 |  331666 B |        2.03 |
| Compression_GzipOptimal_Medium    |  5,423.85 μs |   156.354 μs |   8.570 μs |  1.00 |    0.00 |  23.4375 |        - |        - |  163352 B |        1.00 |
|                                   |              |              |            |       |         |          |          |          |           |             |
| Compression_GzipFastest_Large     |  6,217.63 μs | 4,042.496 μs | 221.583 μs |     ? |       ? | 203.1250 | 187.5000 | 187.5000 | 2719290 B |           ? |
| Compression_GzipOptimal_Large     | 45,871.86 μs | 5,094.672 μs | 279.256 μs |     ? |       ? |        - |        - |        - | 1365762 B |           ? |
|                                   |              |              |            |       |         |          |          |          |           |             |
| DataBlocks_Span_Medium            |    221.75 μs |    35.150 μs |   1.927 μs |  0.48 |    0.00 |   4.8828 |   0.2441 |        - |   31896 B |        0.72 |
| DataBlocks_Original_Medium        |    462.34 μs |    23.066 μs |   1.264 μs |  1.00 |    0.00 |   6.8359 |        - |        - |   44296 B |        1.00 |
| DataBlocks_NoLinq_Medium          |    465.36 μs |    50.708 μs |   2.779 μs |  1.01 |    0.01 |   4.8828 |        - |        - |   31896 B |        0.72 |
|                                   |              |              |            |       |         |          |          |          |           |             |
| FullExport_Optimized_Medium       |  1,885.95 μs |   724.317 μs |  39.702 μs |  0.28 |    0.01 |  97.6563 |  42.9688 |  39.0625 | 1272524 B |        0.84 |
| FullExport_Original_Medium        |  6,649.37 μs | 2,358.715 μs | 129.289 μs |  1.00 |    0.02 | 117.1875 |  54.6875 |  46.8750 | 1510726 B |        1.00 |
|                                   |              |              |            |       |         |          |          |          |           |             |
| JumpTargets_Presized_Medium       |    524.09 μs |   386.584 μs |  21.190 μs |  1.00 |    0.04 |   8.7891 |   8.7891 |   8.7891 |  209771 B |        0.80 |
| JumpTargets_Original_Medium       |    524.54 μs |   132.522 μs |   7.264 μs |  1.00 |    0.02 |  26.3672 |   5.8594 |   5.8594 |  262544 B |        1.00 |
| JumpTargets_SpanPrecount_Medium   |    704.70 μs |   467.169 μs |  25.607 μs |  1.34 |    0.05 |   1.9531 |   1.9531 |   1.9531 |   89001 B |        0.34 |
| JumpTargets_Simd_Medium           |    779.31 μs |   305.510 μs |  16.746 μs |  1.49 |    0.03 |   1.9531 |   1.9531 |   1.9531 |   89001 B |        0.34 |
|                                   |              |              |            |       |         |          |          |          |           |             |
| SymbolSection_BufferWriter_Medium |     22.26 μs |     2.580 μs |   0.141 μs |  0.40 |    0.00 |   6.6833 |   0.2747 |        - |   42064 B |        0.31 |
| SymbolSection_NoLinq_Medium       |     49.34 μs |     4.899 μs |   0.269 μs |  0.89 |    0.01 |  13.0615 |   0.5493 |        - |   82168 B |        0.61 |
| SymbolSection_Original_Medium     |     55.35 μs |     7.164 μs |   0.393 μs |  1.00 |    0.01 |  21.2402 |        - |        - |  133736 B |        1.00 |
