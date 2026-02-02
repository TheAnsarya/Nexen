```text

BenchmarkDotNet v0.14.0, Windows 10 (10.0.19045.6466/22H2/2022Update)
Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 1 CPU, 12 logical and 6 physical cores
.NET SDK 10.0.102
  [Host]     : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2
  DefaultJob : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2
  ShortRun   : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2

```

| Method | Job | IterationCount | LaunchCount | WarmupCount | FrameCount | Mean | Error | StdDev | Ratio | RatioSD | Gen0 | Gen1 | Allocated | Alloc Ratio |
| ----------- | ----------- | --------------- | ------------ | ------------ | ----------- | ----------: | -----------: | ----------: | ------: | --------: | ---------: | ---------: | -----------: | ------------: |
| **ReadNexen** | **DefaultJob** | **Default** | **Default** | **Default** | **100** | **38.49 μs** | **0.627 μs** | **0.556 μs** | **1.00** | **0.02** | **24.3530** | **4.8218** | **149.41 KB** | **1.00** |
| WriteNexen | DefaultJob | Default | Default | Default | 100 | 62.07 μs | 1.182 μs | 0.987 μs | 1.61 | 0.03 | 5.7373 | 0.1221 | 35.25 KB | 0.24 |
| ReadBk2 | DefaultJob | Default | Default | Default | 100 | 13.89 μs | 0.271 μs | 0.323 μs | 0.36 | 0.01 | 4.3182 | 0.0916 | 26.53 KB | 0.18 |
| WriteBk2 | DefaultJob | Default | Default | Default | 100 | 47.80 μs | 0.576 μs | 0.510 μs | 1.24 | 0.02 | 8.4229 | 0.1831 | 51.85 KB | 0.35 |
| ReadFm2 | DefaultJob | Default | Default | Default | 100 | 17.20 μs | 0.343 μs | 0.381 μs | 0.45 | 0.01 | 20.3552 | 5.6152 | 124.81 KB | 0.84 |
| WriteFm2 | DefaultJob | Default | Default | Default | 100 | 29.72 μs | 0.422 μs | 0.352 μs | 0.77 | 0.01 | 34.2712 | 9.3384 | 209.99 KB | 1.41 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| ReadNexen | ShortRun | 3 | 1 | 3 | 100 | 37.12 μs | 5.537 μs | 0.303 μs | 1.00 | 0.01 | 24.3530 | 6.0425 | 149.41 KB | 1.00 |
| WriteNexen | ShortRun | 3 | 1 | 3 | 100 | 61.49 μs | 28.455 μs | 1.560 μs | 1.66 | 0.04 | 5.7373 | 0.1221 | 35.25 KB | 0.24 |
| ReadBk2 | ShortRun | 3 | 1 | 3 | 100 | 14.05 μs | 1.661 μs | 0.091 μs | 0.38 | 0.00 | 4.3182 | 0.0916 | 26.53 KB | 0.18 |
| WriteBk2 | ShortRun | 3 | 1 | 3 | 100 | 47.94 μs | 2.501 μs | 0.137 μs | 1.29 | 0.01 | 8.4229 | 0.1831 | 51.85 KB | 0.35 |
| ReadFm2 | ShortRun | 3 | 1 | 3 | 100 | 17.89 μs | 2.616 μs | 0.143 μs | 0.48 | 0.00 | 20.3552 | 5.6152 | 124.81 KB | 0.84 |
| WriteFm2 | ShortRun | 3 | 1 | 3 | 100 | 31.73 μs | 15.666 μs | 0.859 μs | 0.85 | 0.02 | 34.2712 | 9.3384 | 209.99 KB | 1.41 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| **ReadNexen** | **DefaultJob** | **Default** | **Default** | **Default** | **1000** | **312.69 μs** | **6.011 μs** | **10.684 μs** | **1.00** | **0.05** | **225.0977** | **131.3477** | **1379.88 KB** | **1.00** |
| WriteNexen | DefaultJob | Default | Default | Default | 1000 | 229.04 μs | 2.109 μs | 1.761 μs | 0.73 | 0.02 | 43.9453 | 1.4648 | 270.19 KB | 0.20 |
| ReadBk2 | DefaultJob | Default | Default | Default | 1000 | 39.13 μs | 0.682 μs | 0.978 μs | 0.13 | 0.01 | 15.8081 | 0.3052 | 96.84 KB | 0.07 |
| WriteBk2 | DefaultJob | Default | Default | Default | 1000 | 240.87 μs | 4.375 μs | 3.878 μs | 0.77 | 0.03 | 63.4766 | 1.9531 | 391.39 KB | 0.28 |
| ReadFm2 | DefaultJob | Default | Default | Default | 1000 | 213.53 μs | 2.432 μs | 2.156 μs | 0.68 | 0.02 | 194.8242 | 138.9160 | 1193.57 KB | 0.86 |
| WriteFm2 | DefaultJob | Default | Default | Default | 1000 | 432.40 μs | 8.619 μs | 14.868 μs | 1.38 | 0.07 | 331.0547 | 253.9063 | 2031.07 KB | 1.47 |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| ReadNexen | ShortRun | 3 | 1 | 3 | 1000 | 312.29 μs | 125.830 μs | 6.897 μs | 1.00 | 0.03 | 225.0977 | 131.3477 | 1379.88 KB | 1.00 |
| WriteNexen | ShortRun | 3 | 1 | 3 | 1000 | 226.80 μs | 52.416 μs | 2.873 μs | 0.73 | 0.02 | 43.9453 | 1.4648 | 270.19 KB | 0.20 |
| ReadBk2 | ShortRun | 3 | 1 | 3 | 1000 | 40.17 μs | 23.286 μs | 1.276 μs | 0.13 | 0.00 | 15.8081 | 0.3052 | 96.84 KB | 0.07 |
| WriteBk2 | ShortRun | 3 | 1 | 3 | 1000 | 234.96 μs | 34.516 μs | 1.892 μs | 0.75 | 0.02 | 63.4766 | 1.9531 | 391.39 KB | 0.28 |
| ReadFm2 | ShortRun | 3 | 1 | 3 | 1000 | 222.30 μs | 129.750 μs | 7.112 μs | 0.71 | 0.02 | 194.8242 | 138.9160 | 1193.57 KB | 0.86 |
| WriteFm2 | ShortRun | 3 | 1 | 3 | 1000 | 422.51 μs | 120.120 μs | 6.584 μs | 1.35 | 0.03 | 331.0547 | 253.9063 | 2031.07 KB | 1.47 |
