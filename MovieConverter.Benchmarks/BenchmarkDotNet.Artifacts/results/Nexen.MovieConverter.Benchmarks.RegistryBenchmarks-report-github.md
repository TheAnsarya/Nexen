```text

BenchmarkDotNet v0.14.0, Windows 10 (10.0.19045.6466/22H2/2022Update)
Intel Core i7-8700K CPU 3.70GHz (Coffee Lake), 1 CPU, 12 logical and 6 physical cores
.NET SDK 10.0.102
  [Host]     : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2
  DefaultJob : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2
  ShortRun   : .NET 10.0.2 (10.0.225.61305), X64 RyuJIT AVX2

```

| Method | Job | IterationCount | LaunchCount | WarmupCount | Mean | Error | StdDev | Median | Ratio | RatioSD | Gen0 | Gen1 | Allocated | Alloc Ratio |
| ----------------------------------- | ----------- | --------------- | ------------ | ------------ | -------------: | ------------: | -----------: | -------------: | -------: | --------: | -------: | -------: | ----------: | ------------: |
| GetConverterByFormat_Nexen | DefaultJob | Default | Default | Default | 2.175 ns | 0.0553 ns | 0.0490 ns | 2.160 ns | 1.00 | 0.03 | - | - | - | NA |
| GetConverterByFormat_Bk2 | DefaultJob | Default | Default | Default | 2.298 ns | 0.0952 ns | 0.1019 ns | 2.264 ns | 1.06 | 0.05 | - | - | - | NA |
| GetConverterByExtension_NexenMovie | DefaultJob | Default | Default | Default | 12.530 ns | 0.3061 ns | 0.2714 ns | 12.394 ns | 5.76 | 0.17 | - | - | - | NA |
| GetConverterByExtension_Bk2 | DefaultJob | Default | Default | Default | 10.390 ns | 0.2567 ns | 0.5245 ns | 10.444 ns | 4.78 | 0.26 | - | - | - | NA |
| GetConverterByExtension_WithoutDot | DefaultJob | Default | Default | Default | 19.721 ns | 0.3680 ns | 0.3442 ns | 19.738 ns | 9.07 | 0.25 | 0.0051 | - | 32 B | NA |
| DetectFormat | DefaultJob | Default | Default | Default | 21.302 ns | 0.4555 ns | 0.8213 ns | 21.239 ns | 9.80 | 0.43 | 0.0051 | - | 32 B | NA |
| GetAllConverters | DefaultJob | Default | Default | Default | 32.607 ns | 0.6588 ns | 1.7700 ns | 32.518 ns | 15.00 | 0.87 | 0.0178 | - | 112 B | NA |
| GetReadableFormats | DefaultJob | Default | Default | Default | 93.161 ns | 2.5363 ns | 7.1950 ns | 90.617 ns | 42.85 | 3.42 | 0.0293 | - | 184 B | NA |
| GetWritableFormats | DefaultJob | Default | Default | Default | 95.514 ns | 1.9159 ns | 2.0500 ns | 95.172 ns | 43.93 | 1.32 | 0.0267 | - | 168 B | NA |
| GetOpenFileFilter | DefaultJob | Default | Default | Default | 1,127.450 ns | 17.5034 ns | 16.3727 ns | 1,127.178 ns | 518.60 | 13.36 | 0.5970 | 0.0019 | 3752 B | NA |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
| GetConverterByFormat_Nexen | ShortRun | 3 | 1 | 3 | 2.416 ns | 9.4759 ns | 0.5194 ns | 2.166 ns | 1.03 | 0.26 | - | - | - | NA |
| GetConverterByFormat_Bk2 | ShortRun | 3 | 1 | 3 | 2.431 ns | 2.0709 ns | 0.1135 ns | 2.484 ns | 1.03 | 0.18 | - | - | - | NA |
| GetConverterByExtension_NexenMovie | ShortRun | 3 | 1 | 3 | 12.670 ns | 0.6182 ns | 0.0339 ns | 12.671 ns | 5.39 | 0.90 | - | - | - | NA |
| GetConverterByExtension_Bk2 | ShortRun | 3 | 1 | 3 | 9.837 ns | 1.5128 ns | 0.0829 ns | 9.835 ns | 4.19 | 0.70 | - | - | - | NA |
| GetConverterByExtension_WithoutDot | ShortRun | 3 | 1 | 3 | 20.658 ns | 7.1066 ns | 0.3895 ns | 20.552 ns | 8.79 | 1.47 | 0.0051 | - | 32 B | NA |
| DetectFormat | ShortRun | 3 | 1 | 3 | 20.402 ns | 1.3375 ns | 0.0733 ns | 20.428 ns | 8.68 | 1.45 | 0.0051 | - | 32 B | NA |
| GetAllConverters | ShortRun | 3 | 1 | 3 | 29.971 ns | 3.4833 ns | 0.1909 ns | 29.869 ns | 12.76 | 2.13 | 0.0178 | - | 112 B | NA |
| GetReadableFormats | ShortRun | 3 | 1 | 3 | 83.339 ns | 2.4250 ns | 0.1329 ns | 83.412 ns | 35.47 | 5.91 | 0.0293 | - | 184 B | NA |
| GetWritableFormats | ShortRun | 3 | 1 | 3 | 109.031 ns | 24.7120 ns | 1.3545 ns | 109.492 ns | 46.41 | 7.75 | 0.0267 | - | 168 B | NA |
| GetOpenFileFilter | ShortRun | 3 | 1 | 3 | 1,133.526 ns | 153.5187 ns | 8.4149 ns | 1,131.808 ns | 482.48 | 80.47 | 0.5970 | 0.0019 | 3752 B | NA |
