```

BenchmarkDotNet v0.15.8, Windows 10 (10.0.19045.6466/22H2/2022Update)
Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 1 CPU, 12 logical and 6 physical cores
.NET SDK 10.0.102
  [Host]   : .NET 10.0.3 (10.0.3, 10.0.326.7603), X64 RyuJIT x86-64-v3
  ShortRun : .NET 10.0.3 (10.0.3, 10.0.326.7603), X64 RyuJIT x86-64-v3

Job=ShortRun  IterationCount=3  LaunchCount=1  
WarmupCount=3  

```
| Method                          | Mean     | Error     | StdDev   | Ratio | RatioSD | Gen0   | Allocated | Alloc Ratio |
|-------------------------------- |---------:|----------:|---------:|------:|--------:|-------:|----------:|------------:|
| GetContrastTextColor_1024Colors | 96.21 μs | 58.755 μs | 3.221 μs |     ? |       ? | 0.6104 |   4.02 KB |           ? |
|                                 |          |           |          |       |         |        |           |             |
| InvertBrightness_1024Colors     | 43.51 μs |  9.372 μs | 0.514 μs |     ? |       ? | 0.6104 |   4.02 KB |           ? |
|                                 |          |           |          |       |         |        |           |             |
| HslToRgb_1024Colors             | 16.34 μs |  0.627 μs | 0.034 μs |  0.91 |    0.01 | 0.6409 |   4.02 KB |        0.33 |
| RgbToHsl_1024Colors             | 17.94 μs |  4.088 μs | 0.224 μs |  1.00 |    0.02 | 1.9531 |  12.02 KB |        1.00 |
| RgbToHslToRgb_Roundtrip_1024    | 43.69 μs |  0.886 μs | 0.049 μs |  2.44 |    0.03 | 0.6104 |   4.02 KB |        0.33 |
