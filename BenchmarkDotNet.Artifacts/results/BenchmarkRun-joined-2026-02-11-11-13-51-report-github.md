```

BenchmarkDotNet v0.15.8, Windows 10 (10.0.19045.6466/22H2/2022Update)
Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 1 CPU, 12 logical and 6 physical cores
.NET SDK 10.0.102
  [Host]   : .NET 10.0.3 (10.0.3, 10.0.326.7603), X64 RyuJIT x86-64-v3
  ShortRun : .NET 10.0.3 (10.0.3, 10.0.326.7603), X64 RyuJIT x86-64-v3

Job=ShortRun  IterationCount=3  LaunchCount=1  
WarmupCount=3  

```
| Type                      | Method                                       | Mean         | Error         | StdDev       | Ratio | RatioSD | Gen0    | Gen1   | Allocated | Alloc Ratio |
|-------------------------- |--------------------------------------------- |-------------:|--------------:|-------------:|------:|--------:|--------:|-------:|----------:|------------:|
| ColorConversionBenchmarks | GetContrastTextColor_1024Colors              | 95,139.01 ns | 47,731.413 ns | 2,616.320 ns |     ? |       ? |  0.6104 |      - |    4120 B |           ? |
|                           |                                              |              |               |              |       |         |         |        |           |             |
| ColorConversionBenchmarks | InvertBrightness_1024Colors                  | 45,960.30 ns |  2,015.816 ns |   110.494 ns |     ? |       ? |  0.6104 |      - |    4120 B |           ? |
|                           |                                              |              |               |              |       |         |         |        |           |             |
| Utf8UtilitiesBenchmarks   | GetStringFromArray_VsEncodingGetString_Short |     14.92 ns |     19.874 ns |     1.089 ns | 0.001 |    0.00 |  0.0076 |      - |      48 B |       0.001 |
| Utf8UtilitiesBenchmarks   | GetStringFromArray_Custom_Short              |     16.85 ns |      1.437 ns |     0.079 ns | 0.001 |    0.00 |  0.0076 |      - |      48 B |       0.001 |
| GameDataManagerBenchmarks | FormatGameFolderName_Typical                 |     34.31 ns |      8.920 ns |     0.489 ns | 0.002 |    0.00 |  0.0140 |      - |      88 B |       0.002 |
| GameDataManagerBenchmarks | GetSystemFolderName_AllSystems               |     68.33 ns |    205.578 ns |    11.268 ns | 0.004 |    0.00 |  0.0216 |      - |     136 B |       0.002 |
| GameDataManagerBenchmarks | SanitizeRomName_Typical                      |    132.05 ns |     36.432 ns |     1.997 ns | 0.007 |    0.00 |  0.0165 |      - |     104 B |       0.002 |
| GameDataManagerBenchmarks | BuildPath_Typical                            |    172.48 ns |     16.054 ns |     0.880 ns | 0.010 |    0.00 |  0.0432 |      - |     272 B |       0.005 |
| GameDataManagerBenchmarks | SanitizeRomName_Long                         |    245.32 ns |     36.669 ns |     2.010 ns | 0.014 |    0.00 |  0.3018 |      - |    1896 B |       0.034 |
| UiPerformanceBenchmarks   | AllocateListEveryFrame                       |    688.84 ns |    139.096 ns |     7.624 ns | 0.038 |    0.00 |  0.3347 | 0.0019 |    2104 B |       0.038 |
| UiPerformanceBenchmarks   | ReuseListAcrossFrames                        |    857.97 ns |    294.424 ns |    16.138 ns | 0.048 |    0.00 |       - |      - |         - |       0.000 |
| GameDataManagerBenchmarks | SanitizeRomName_AllVariants                  |  1,276.13 ns |    216.830 ns |    11.885 ns | 0.071 |    0.00 |  0.1965 |      - |    1240 B |       0.022 |
| UiPerformanceBenchmarks   | ReuseHashSetAcrossFrames                     |  1,542.02 ns |     59.582 ns |     3.266 ns | 0.086 |    0.00 |       - |      - |         - |       0.000 |
| UiPerformanceBenchmarks   | AllocateHashSetEveryFrame                    |  1,543.98 ns |    812.045 ns |    44.511 ns | 0.086 |    0.00 |  0.1125 |      - |     712 B |       0.013 |
| UiPerformanceBenchmarks   | ReuseDictionaryWithPooledLists               |  2,013.30 ns |  5,928.222 ns |   324.946 ns | 0.112 |    0.02 |       - |      - |         - |       0.000 |
| UiPerformanceBenchmarks   | GetBrushCached                               |  2,354.40 ns |    555.963 ns |    30.474 ns | 0.131 |    0.00 |  0.3281 |      - |    2072 B |       0.037 |
| UiPerformanceBenchmarks   | AllocateDictionaryWithLists                  |  4,437.67 ns |    489.041 ns |    26.806 ns | 0.246 |    0.00 |  1.5564 | 0.0534 |    9800 B |       0.175 |
| UiPerformanceBenchmarks   | GetBrushCachedRepeated                       |  7,338.53 ns |  2,469.096 ns |   135.339 ns | 0.407 |    0.01 |  1.2741 |      - |    8024 B |       0.143 |
| Utf8UtilitiesBenchmarks   | GetStringFromArray_ShortStrings              | 18,016.42 ns |  3,403.638 ns |   186.565 ns | 1.000 |    0.01 |  8.9111 | 1.4648 |   56024 B |       1.000 |
| Utf8UtilitiesBenchmarks   | GetStringFromArray_NullTerminated            | 21,170.71 ns |  3,745.981 ns |   205.330 ns | 1.175 |    0.01 | 10.1929 | 2.0142 |   64024 B |       1.143 |
| UiPerformanceBenchmarks   | GetBrushUncached                             | 33,445.44 ns | 15,885.060 ns |   870.714 ns | 1.857 |    0.05 | 25.0854 | 7.9346 |  157720 B |       2.815 |
| Utf8UtilitiesBenchmarks   | GetStringFromArray_LongStrings               | 53,031.13 ns | 38,532.305 ns | 2,112.086 ns | 2.944 |    0.10 | 29.1748 | 0.5493 |  183224 B |       3.270 |
|                           |                                              |              |               |              |       |         |         |        |           |             |
| ColorConversionBenchmarks | HslToRgb_1024Colors                          | 16,112.25 ns |  8,296.572 ns |   454.763 ns |  0.80 |    0.03 |  0.6409 |      - |    4120 B |        0.17 |
| ColorConversionBenchmarks | RgbToHsl_1024Colors                          | 20,154.76 ns |  9,653.374 ns |   529.134 ns |  1.00 |    0.03 |  3.8757 |      - |   24600 B |        1.00 |
| ColorConversionBenchmarks | RgbToHslToRgb_Roundtrip_1024                 | 44,709.91 ns |  5,515.041 ns |   302.298 ns |  2.22 |    0.05 |  0.6104 |      - |    4120 B |        0.17 |
