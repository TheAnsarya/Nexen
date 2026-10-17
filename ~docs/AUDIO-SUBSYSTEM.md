# Audio Subsystem Documentation

This document covers Nexen's audio emulation, mixing, and output subsystem.

---

## Architecture Overview

```text
Audio Architecture
══════════════════

┌─────────────────────────────────────────────────────────────────┐
│                    Per-Console APU                             │
│   (NES APU, SNES SPC700, GB APU, SMS PSG, etc.)               │
└─────────────────────────────────────────────────────────────────┘
              │
              │ Samples at console rate
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     SoundMixer                                 │
│   (Mixing, effects, resampling)                                │
└─────────────────────────────────────────────────────────────────┘
              │
    ┌─────────┼─────────┬──────────────┐
    │        │       │            │
    ▼        ▼       ▼            ▼
┌───────┐ ┌───────┐ ┌─────────┐ ┌───────────┐
│Equaliz│ │CrossFd│ │ Reverb  │ │ Resampler │
│  er   │ │Filter │ │ Filter  │ │ Hermite   │
└───────┘ └───────┘ └─────────┘ └───────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   IAudioDevice                                 │
│   (Platform audio output - DirectSound, CoreAudio, ALSA)      │
└─────────────────────────────────────────────────────────────────┘
```

## Directory Structure

```text
Core/Shared/Audio/
├── SoundMixer.h/cpp           - Central audio coordinator
├── SoundResampler.h/cpp       - Rate conversion
├── BaseSoundManager.h/cpp   - Audio device abstraction
├── WaveRecorder.h/cpp       - WAV file recording
├── PcmReader.h/cpp         - PCM file playback
├── AudioPlayerHud.h/cpp       - Audio player UI
└── AudioPlayerTypes.h       - Player type definitions

Utilities/Audio/
├── HermiteResampler.h       - High-quality resampling
├── Equalizer.h/cpp         - Parametric EQ
├── CrossFeedFilter.h/cpp     - Stereo separation
└── ReverbFilter.h/cpp       - Room reverb
```

---

## Core Components

### SoundMixer (SoundMixer.h)

Central audio mixing and processing hub.

**Key Responsibilities:**

- Combine audio from multiple sources
- Apply effects (EQ, crossfeed, reverb)
- Resample to output rate
- Record to file

**Audio Flow:**

```cpp
void PlayAudioBuffer(int16_t* samples, uint32_t count, uint32_t sourceRate) {
    // 1. Mix with other audio providers
    // 2. Apply equalizer
    // 3. Apply crossfeed
    // 4. Apply reverb
    // 5. Resample to target rate
    // 6. Send to audio device
    // 7. Record if enabled
}
```

**Audio Providers:**

```cpp
// Register audio source (APU, expansion audio, CD-ROM)
void RegisterAudioProvider(IAudioProvider* provider);
void UnregisterAudioProvider(IAudioProvider* provider);
```

### SoundResampler (SoundResampler.h)

Converts between sample rates.

**Features:**

- Hermite interpolation (high quality)
- Handles arbitrary rate ratios
- Low-latency processing

**Common Rate Conversions:**

| Source | Rate | Target |
| -------- | ------ | -------- |
| NES APU | ~44100 Hz | 48000 Hz |
| SNES SPC | 32040 Hz | 48000 Hz |
| GB APU | 65536 Hz | 48000 Hz |
| SMS PSG | 44100 Hz | 48000 Hz |

---

## Per-System Audio

### NES Audio (Core/NES/APU/)

**Channels:**

1. **Pulse 1** - Square wave with sweep
2. **Pulse 2** - Square wave with sweep
3. **Triangle** - Triangle wave (bass)
4. **Noise** - Pseudo-random noise
5. **DMC** - Delta modulation (samples)

**Expansion Audio:**

- **VRC6** - 2 pulse + 1 sawtooth
- **VRC7** - 6-channel FM synthesis
- **FDS** - Wavetable synthesis
- **MMC5** - 2 extra pulse + PCM
- **N163** - 8-channel wavetable
- **Sunsoft 5B** - 3-channel PSG

### SNES Audio (Core/SNES/Spc/)

**Architecture:**

- SPC700 CPU (8-bit, Sony)
- S-DSP (8-channel)
- 64KB audio RAM

**Features:**

- 8 channels with BRR compression
- ADSR envelopes
- Echo/reverb (built-in DSP)
- Noise channel
- Pitch modulation

### Game Boy Audio (Core/Gameboy/APU/)

**Channels:**

1. **Pulse 1** - Square wave with sweep
2. **Pulse 2** - Square wave
3. **Wave** - 32-sample wavetable
4. **Noise** - LFSR noise

### GBA Audio (Core/GBA/APU/)

**Channels:**

- GB-compatible (4 channels)
- **Direct Sound A** - PCM channel
- **Direct Sound B** - PCM channel

### SMS/Game Gear Audio (Core/SMS/)

**SN76489 PSG:**

- 3 square wave channels
- 1 noise channel

**Optional FM (YM2413):**

- 9 FM channels (or 6 FM + 5 rhythm)

### PC Engine Audio (Core/PCE/)

**6-Channel PSG:**

- 6 identical wavetable channels
- 32-sample 5-bit waveforms
- LFO modulation on channels 5-6

**CD-ROM Audio:**

- Red Book audio tracks
- ADPCM sample playback

### WonderSwan Audio (Core/WS/APU/)

**4-Channel + Voice:**

- 4 wavetable channels
- Voice input/output
- Stereo headphone support

---

## Effects Processing

### Equalizer

Parametric EQ for tone shaping.

**Bands:**

- Bass (low frequencies)
- Mid (medium frequencies)
- Treble (high frequencies)

**Configuration:**

```cpp
struct EqualizerSettings {
    int8_t Bass = 0;     // -20 to +20 dB
    int8_t Mid = 0;   // -20 to +20 dB
    int8_t Treble = 0;   // -20 to +20 dB
};
```

### CrossFeed Filter

Reduces stereo separation for headphone listening.

**Purpose:**

- Simulates speaker bleed
- Reduces fatigue on headphones
- More natural stereo image

**Configuration:**

```cpp
struct CrossFeedSettings {
    bool Enabled = false;
    uint8_t Strength = 30;  // 0-100%
};
```

### Reverb Filter

Adds artificial room acoustics.

**Parameters:**

- Wet/dry mix
- Room size
- Decay time
- Pre-delay

---

## Audio Recording

### WaveRecorder (WaveRecorder.h)

Records audio to WAV files.

**Features:**

- 16-bit PCM format
- Configurable sample rate
- Async write (thread-safe)

**Usage:**

```cpp
// Start recording
waveRecorder->StartRecording("output.wav", sampleRate);

// Stop recording
waveRecorder->StopRecording();
```

**File Format:**

- RIFF header
- Format chunk (16-bit, stereo)
- Data chunk (interleaved samples)

---

## Sample Rates

### Console Native Rates

| System | APU Rate | Notes |
| -------- | ---------- | ------- |
| NES | ~1,789,773 Hz | CPU clock / APU divider |
| SNES SPC | 32,040 Hz | Fixed DSP rate |
| Game Boy | 65,536 Hz | 2^16 Hz exactly |
| GBA | 32,768 Hz | 2^15 Hz |
| SMS | 3,579,545 Hz | Z80 clock / PSG divider |
| PC Engine | 3,579,545 Hz | Similar to SMS |
| WonderSwan | ~24,000 Hz | Variable |

### Resampling

All audio resampled to device rate (typically 48000 Hz).

**Quality Modes:**

- **Hermite** - High quality, smooth interpolation
- **Linear** - Lower quality, faster
- **None** - Nearest sample (aliasing)

---

## Latency Considerations

### Audio Buffer Size

```cpp
// Buffer size affects latency
uint32_t bufferSize = 2048;  // ~42ms at 48000 Hz
```

**Trade-offs:**

- Smaller buffer → Lower latency, higher risk of underrun
- Larger buffer → Higher latency, stable audio

### Recommended Settings

| Use Case | Buffer Size | Latency |
| ---------- | ------------- | --------- |
| Low latency | 1024 | ~21ms |
| Balanced | 2048 | ~42ms |
| Stable | 4096 | ~85ms |

---

## Thread Safety

**Safe Patterns:**

```cpp
// safe_ptr for async recorder access
safe_ptr<WaveRecorder> _waveRecorder;

// Provider list protected by mutex
void RegisterAudioProvider(IAudioProvider* provider) {
    lock_guard<mutex> lock(_providerLock);
    _audioProviders.push_back(provider);
}
```

---

## Related Documentation

- [NES-CORE.md](NES-CORE.md) - NES APU details
- [SNES-CORE.md](SNES-CORE.md) - SPC700/S-DSP details
- [GB-GBA-CORE.md](GB-GBA-CORE.md) - GB/GBA APU details
- [SMS-PCE-WS-CORE.md](SMS-PCE-WS-CORE.md) - Other system audio
- [ARCHITECTURE-OVERVIEW.md](ARCHITECTURE-OVERVIEW.md) - Overall structure
