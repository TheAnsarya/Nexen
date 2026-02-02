```text

BenchmarkDotNet v0.14.0, Windows 10 (10.0.19045.6466/22H2/2022Update)
Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 1 CPU, 12 logical and 6 physical cores
.NET SDK 10.0.101
  [Host]     : .NET 10.0.1 (10.0.125.57005), X64 RyuJIT AVX2
  Job-UOXLGX : .NET 10.0.1 (10.0.125.57005), X64 RyuJIT AVX2

MaxIterationCount=16  

```

| Method | Mean | Error | StdDev | Ratio | RatioSD | Gen0 | Gen1 | Gen2 | Allocated | Alloc Ratio |
| ---------------------------------- | ------------: | ----------: | ----------: | ------: | --------: | ---------: | ---------: | ---------: | -----------: | ------------: |
| Compression_BrotliFastest_Medium | 673.6 μs | 7.58 μs | 5.92 μs | 0.12 | 0.00 | 17.5781 | 2.9297 | 2.9297 | 183.19 KB | 1.15 |
| Compression_DeflateFastest_Medium | 811.5 μs | 11.58 μs | 10.27 μs | 0.15 | 0.00 | 36.1328 | 6.8359 | 4.8828 | 323.85 KB | 2.03 |
| Compression_ZLibFastest_Medium | 822.9 μs | 15.71 μs | 14.70 μs | 0.15 | 0.00 | 37.1094 | 6.8359 | 5.8594 | 323.89 KB | 2.03 |
| Compression_GzipFastest_Medium | 835.3 μs | 14.15 μs | 12.54 μs | 0.15 | 0.00 | 36.1328 | 6.8359 | 4.8828 | 323.89 KB | 2.03 |
| Compression_GzipOptimal_Medium | 5,534.4 μs | 45.60 μs | 35.60 μs | 1.00 | 0.01 | 23.4375 | - | - | 159.53 KB | 1.00 |
| Compression_GzipFastest_Large | 6,863.1 μs | 69.77 μs | 58.26 μs | 1.24 | 0.01 | 187.5000 | 171.8750 | 171.8750 | 2655.43 KB | 16.65 |
| Compression_GzipOptimal_Large | 45,509.0 μs | 472.41 μs | 418.78 μs | 8.22 | 0.09 | - | - | - | 1333.81 KB | 8.36 |
