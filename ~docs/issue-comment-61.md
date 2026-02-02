## Progress Update

### Migrated (3 batches committed)

**Batch 1 (d5400eef):**

- NesSoundMixer: `_outputBuffer` to `unique_ptr<int16_t[]>`
- NesNtscFilter: `_ntscBuffer` to `unique_ptr<uint32_t[]>`
- BaseNesPpu: `_outputBuffers` to `array<unique_ptr<uint16_t[]>, 2>`
- NesPpu: local `mergedBuffer` to `unique_ptr`
- NesMemoryManager: `_internalRam`, `_ramReadHandlers`, `_ramWriteHandlers` to `unique_ptr`

**Batch 2 (e6f62fd3):**

- OggReader: `_oggBuffer`, `_outputBuffer` to `unique_ptr<int16_t[]>`
- NesEventManager: `_ppuBuffer` to `unique_ptr<uint16_t[]>`
- HdPackBuilder: local `pngBuffer` to `unique_ptr<uint32_t[]>`

**Batch 3 (45c8c55f):**

- MMC5: `_fillNametable`, `_emptyNametable` to `unique_ptr<uint8_t[]>`
- HdData: `ScreenTiles` to `unique_ptr<HdPpuPixelInfo[]>`

### Remaining (complex ownership patterns - separate sub-issue needed)

**BaseMapper (7 arrays):** `_chrRam`, `_chrRom`, `_prgRom`, `_saveRam`, `_workRam`, `_mapperRam`, `_nametableRam`

- Protected members accessed by 100+ mapper subclasses
- Used with `RegisterMemory()` which takes raw pointers
- Would require extensive refactoring across entire mapper hierarchy

**StudyBox.h and Fds.cpp:** These delete `_prgRom` (from BaseMapper) to replace with firmware data loaded via `FirmwareHelper::LoadFdsFirmware()` which takes `uint8_t**`

- Ownership transfer pattern: subclass deletes base's memory and re-assigns
- Requires design changes to FirmwareHelper and mapper initialization

**Recommendation:** Create separate sub-issue for BaseMapper array refactoring as it requires architectural changes beyond simple smart pointer migration.
