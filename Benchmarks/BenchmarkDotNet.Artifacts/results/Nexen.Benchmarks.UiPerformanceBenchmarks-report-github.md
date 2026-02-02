```text

BenchmarkDotNet v0.14.0, Windows 10 (10.0.19045.6466/22H2/2022Update)
Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 1 CPU, 12 logical and 6 physical cores
.NET SDK 10.0.102
  [Host]   : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2
  ShortRun : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2

Job=ShortRun  IterationCount=3  LaunchCount=1  
WarmupCount=3  

```

| Method | Mean | Error | StdDev | Gen0 | Gen1 | Allocated |
| ------------------------------- | ------------: | -------------: | ------------: | --------: | -------: | ----------: |
| AllocateListEveryFrame | 727.6 ns | 173.76 ns | 9.52 ns | 0.3347 | 0.0019 | 2104 B |
| ReuseListAcrossFrames | 885.9 ns | 64.42 ns | 3.53 ns | - | - | - |
| AllocateHashSetEveryFrame | 1,628.3 ns | 987.63 ns | 54.14 ns | 0.1125 | - | 712 B |
| ReuseHashSetAcrossFrames | 1,628.8 ns | 496.30 ns | 27.20 ns | - | - | - |
| ReuseDictionaryWithPooledLists | 1,968.0 ns | 396.49 ns | 21.73 ns | - | - | - |
| GetBrushCached | 2,351.4 ns | 1,263.00 ns | 69.23 ns | 0.3281 | - | 2072 B |
| AllocateDictionaryWithLists | 4,999.1 ns | 5,006.24 ns | 274.41 ns | 1.5564 | 0.0534 | 9800 B |
| GetBrushCachedRepeated | 7,692.0 ns | 1,678.69 ns | 92.01 ns | 1.2665 | - | 8024 B |
| GetBrushUncached | 34,436.2 ns | 20,089.89 ns | 1,101.19 ns | 25.0854 | 7.9346 | 157720 B |
