#pragma once

/// <summary>
/// Identifies all memory regions across all supported emulation platforms.
/// Used by debugger for memory viewer, hex editor, breakpoints, and CDL mapping.
/// </summary>
/// <remarks>
/// Organized by platform with logical groupings:
/// - System-wide memory spaces (CPU addressable): SnesMemory, GbMemory, NesMemory, etc.
/// - ROM regions: PrgRom (program code)
/// - RAM regions: WorkRam (general purpose), SaveRam (battery-backed), VideoRam, SpriteRam
/// - Registers: Memory-mapped hardware registers
/// - Coprocessor memory: DSP, SA-1, Super FX, etc.
///
/// Platforms supported:
/// - SNES (65816): SNES/SPC/SA-1/DSP/GSU/CX4/ST018/BSX
/// - Game Boy: GB/GBC
/// - NES (6502): NES
/// - PC Engine: PCE/TurboGrafx-16
/// - Sega Master System: SMS
/// - Game Boy Advance: GBA
/// - WonderSwan: WS
/// </remarks>
enum class MemoryType {
	// ===== System-wide memory spaces =====
	SnesMemory,    ///< SNES 65816 CPU address space (24-bit, up to 16MB)
	SpcMemory,     ///< SNES SPC700 audio CPU address space (16-bit, 64KB)
	Sa1Memory,     ///< SNES SA-1 coprocessor address space
	NecDspMemory,  ///< SNES NEC DSP coprocessor address space
	GsuMemory,     ///< SNES Super FX (GSU) coprocessor address space
	Cx4Memory,     ///< SNES Cx4 coprocessor address space
	St018Memory,   ///< SNES ST018 coprocessor address space
	GameboyMemory, ///< Game Boy CPU address space (16-bit, 64KB)
	NesMemory,     ///< NES 6502 CPU address space (16-bit, 64KB)
	NesPpuMemory,  ///< NES PPU address space (14-bit, 16KB)
	PceMemory,     ///< PC Engine CPU address space (21-bit, 2MB)
	SmsMemory,     ///< Sega Master System Z80 address space (16-bit, 64KB)
	GbaMemory,     ///< Game Boy Advance ARM7 address space (32-bit, 4GB)
	WsMemory,      ///< WonderSwan address space

	// ===== SNES memory regions =====
	SnesPrgRom,               ///< SNES program ROM (cartridge code/data)
	SnesWorkRam,              ///< SNES Work RAM (128KB general purpose RAM)
	SnesSaveRam,              ///< SNES battery-backed Save RAM (cartridge saves)
	SnesVideoRam,             ///< SNES Video RAM (64KB for tiles/tilemaps)
	SnesSpriteRam,            ///< SNES Sprite RAM (OAM - 544 bytes)
	SnesCgRam,                ///< SNES Color Generator RAM (512 bytes palette)
	SnesRegister,             ///< SNES hardware registers (PPU/APU/DMA/etc.)
	SpcRam,                   ///< SPC700 audio RAM (64KB)
	SpcRom,                   ///< SPC700 boot ROM (64 bytes IPL)
	SpcDspRegisters,          ///< SPC700 DSP registers (128 bytes)
	DspProgramRom,            ///< NEC DSP program ROM
	DspDataRom,               ///< NEC DSP data ROM
	DspDataRam,               ///< NEC DSP data RAM
	Sa1InternalRam,           ///< SA-1 internal RAM (2KB IRAM)
	GsuWorkRam,               ///< Super FX work RAM
	Cx4DataRam,               ///< Cx4 data RAM
	BsxPsRam,                 ///< BS-X Satellaview PSRAM
	BsxMemoryPack,            ///< BS-X memory pack flash
	St018PrgRom,              ///< ST018 program ROM
	St018DataRom,             ///< ST018 data ROM
	St018WorkRam,             ///< ST018 work RAM
	SufamiTurboFirmware,      ///< Sufami Turbo firmware ROM
	SufamiTurboSecondCart,    ///< Sufami Turbo second cartridge ROM
	SufamiTurboSecondCartRam, ///< Sufami Turbo second cartridge RAM

	// ===== Game Boy memory regions =====
	GbPrgRom,    ///< Game Boy program ROM (cartridge)
	GbWorkRam,   ///< Game Boy work RAM (8KB internal + optional cart RAM)
	GbCartRam,   ///< Game Boy cartridge RAM (battery-backed saves)
	GbHighRam,   ///< Game Boy high RAM (HRAM - 127 bytes)
	GbBootRom,   ///< Game Boy boot ROM (256 bytes)
	GbVideoRam,  ///< Game Boy video RAM (8KB for tiles)
	GbSpriteRam, ///< Game Boy sprite RAM (OAM - 160 bytes)

	// ===== NES memory regions =====
	NesPrgRom,             ///< NES program ROM (cartridge code)
	NesInternalRam,        ///< NES internal RAM (2KB, mirrored to 8KB)
	NesWorkRam,            ///< NES work RAM (optional cartridge RAM, not battery-backed)
	NesSaveRam,            ///< NES save RAM (battery-backed cartridge RAM)
	NesNametableRam,       ///< NES nametable RAM (PPU - 2KB, mirrored)
	NesMapperRam,          ///< NES mapper-specific RAM
	NesSpriteRam,          ///< NES sprite RAM (OAM - 256 bytes)
	NesSecondarySpriteRam, ///< NES secondary OAM (32 bytes for sprite evaluation)
	NesPaletteRam,         ///< NES palette RAM (32 bytes)
	NesChrRam,             ///< NES CHR RAM (pattern tables - 8KB)
	NesChrRom,             ///< NES CHR ROM (pattern tables - read-only)

	// ===== PC Engine memory regions =====
	PcePrgRom,        ///< PC Engine program ROM (HuCard)
	PceWorkRam,       ///< PC Engine work RAM (8KB)
	PceSaveRam,       ///< PC Engine save RAM (battery-backed)
	PceCdromRam,      ///< PC Engine CD-ROM system RAM
	PceCardRam,       ///< PC Engine TurboChip RAM
	PceAdpcmRam,      ///< PC Engine ADPCM audio RAM
	PceArcadeCardRam, ///< PC Engine Arcade Card RAM
	PceVideoRam,      ///< PC Engine video RAM (VDC1 - 64KB)
	PceVideoRamVdc2,  ///< PC Engine video RAM (VDC2 for SuperGrafx - 64KB)
	PceSpriteRam,     ///< PC Engine sprite RAM (VDC1 SAT)
	PceSpriteRamVdc2, ///< PC Engine sprite RAM (VDC2 SAT)
	PcePaletteRam,    ///< PC Engine palette RAM (512 bytes)

	// ===== Sega Master System memory regions =====
	SmsPrgRom,     ///< SMS program ROM (cartridge)
	SmsWorkRam,    ///< SMS work RAM (8KB)
	SmsCartRam,    ///< SMS cartridge RAM (battery-backed saves)
	SmsBootRom,    ///< SMS boot ROM (BIOS)
	SmsVideoRam,   ///< SMS video RAM (16KB)
	SmsPaletteRam, ///< SMS palette RAM (32 bytes)
	SmsPort,       ///< SMS I/O ports

	// ===== Game Boy Advance memory regions =====
	GbaPrgRom,     ///< GBA program ROM (cartridge, up to 32MB)
	GbaBootRom,    ///< GBA boot ROM (BIOS - 16KB)
	GbaSaveRam,    ///< GBA save RAM (SRAM/Flash/EEPROM)
	GbaIntWorkRam, ///< GBA internal work RAM (32KB, fast)
	GbaExtWorkRam, ///< GBA external work RAM (256KB, slower)
	GbaVideoRam,   ///< GBA video RAM (96KB)
	GbaSpriteRam,  ///< GBA sprite RAM (OAM - 1KB)
	GbaPaletteRam, ///< GBA palette RAM (1KB)

	// ===== WonderSwan memory regions =====
	WsPrgRom,         ///< WonderSwan program ROM
	WsWorkRam,        ///< WonderSwan work RAM
	WsCartRam,        ///< WonderSwan cartridge RAM
	WsCartEeprom,     ///< WonderSwan cartridge EEPROM
	WsBootRom,        ///< WonderSwan boot ROM
	WsInternalEeprom, ///< WonderSwan internal EEPROM
	WsPort,           ///< WonderSwan I/O ports

	/// <summary>Sentinel value for invalid/unspecified memory type</summary>
	None
};