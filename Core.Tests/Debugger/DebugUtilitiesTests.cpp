#include "pch.h"
#include "Debugger/DebugUtilities.h"

TEST(DebugUtilitiesTests, CpuTypeMetadataMatchesExpectedMemoryTypeAndProgramCounterWidth) {
	struct ExpectedCpuMetadata {
		CpuType Cpu;
		MemoryType Memory;
		MemoryType PrgRom;
		int PcWidth;
	};

	static constexpr std::array<ExpectedCpuMetadata, 17> kExpected = {{
		{CpuType::Snes, MemoryType::SnesMemory, MemoryType::SnesPrgRom, 6},
		{CpuType::Spc, MemoryType::SpcMemory, MemoryType::SpcRom, 4},
		{CpuType::NecDsp, MemoryType::NecDspMemory, MemoryType::DspProgramRom, 6},
		{CpuType::Sa1, MemoryType::Sa1Memory, MemoryType::SnesPrgRom, 6},
		{CpuType::Gsu, MemoryType::GsuMemory, MemoryType::SnesPrgRom, 6},
		{CpuType::Cx4, MemoryType::Cx4Memory, MemoryType::SnesPrgRom, 6},
		{CpuType::St018, MemoryType::St018Memory, MemoryType::St018PrgRom, 8},
		{CpuType::Gameboy, MemoryType::GameboyMemory, MemoryType::GbPrgRom, 4},
		{CpuType::Nes, MemoryType::NesMemory, MemoryType::NesPrgRom, 4},
		{CpuType::Pce, MemoryType::PceMemory, MemoryType::PcePrgRom, 4},
		{CpuType::Sms, MemoryType::SmsMemory, MemoryType::SmsPrgRom, 4},
		{CpuType::Gba, MemoryType::GbaMemory, MemoryType::GbaPrgRom, 8},
		{CpuType::Ws, MemoryType::WsMemory, MemoryType::WsPrgRom, 5},
		{CpuType::Lynx, MemoryType::LynxMemory, MemoryType::LynxPrgRom, 4},
		{CpuType::Genesis, MemoryType::GenesisMemory, MemoryType::GenesisPrgRom, 6},
		{CpuType::Atari2600, MemoryType::Atari2600Memory, MemoryType::Atari2600PrgRom, 4},
		{CpuType::ChannelF, MemoryType::ChannelFMemory, MemoryType::ChannelFCartRom, 4}
	}};

	for (const ExpectedCpuMetadata& expected : kExpected) {
		EXPECT_EQ(DebugUtilities::GetCpuMemoryType(expected.Cpu), expected.Memory);
		EXPECT_EQ(DebugUtilities::GetPrgRomMemoryType(expected.Cpu), expected.PrgRom);
		EXPECT_EQ(DebugUtilities::GetProgramCounterSize(expected.Cpu), expected.PcWidth);
		EXPECT_TRUE(DebugUtilities::IsRom(expected.PrgRom));
	}
}

TEST(DebugUtilitiesTests, CpuMemoryTypeRoundtripsBackToOwningCpuType) {
	static constexpr std::array<CpuType, 17> kCpuTypes = {{
		CpuType::Snes,
		CpuType::Spc,
		CpuType::NecDsp,
		CpuType::Sa1,
		CpuType::Gsu,
		CpuType::Cx4,
		CpuType::St018,
		CpuType::Gameboy,
		CpuType::Nes,
		CpuType::Pce,
		CpuType::Sms,
		CpuType::Gba,
		CpuType::Ws,
		CpuType::Lynx,
		CpuType::Genesis,
		CpuType::Atari2600,
		CpuType::ChannelF
	}};

	for (CpuType cpuType : kCpuTypes) {
		MemoryType memoryType = DebugUtilities::GetCpuMemoryType(cpuType);
		EXPECT_EQ(DebugUtilities::ToCpuType(memoryType), cpuType);
	}
}

TEST(DebugUtilitiesTests, BaseCpuMemoryTypesMapBackToExpectedCpuOwners) {
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::SnesMemory), CpuType::Snes);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::SpcMemory), CpuType::Spc);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::NecDspMemory), CpuType::NecDsp);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::Sa1Memory), CpuType::Sa1);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::GsuMemory), CpuType::Gsu);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::Cx4Memory), CpuType::Cx4);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::St018Memory), CpuType::St018);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::GameboyMemory), CpuType::Gameboy);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::NesMemory), CpuType::Nes);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::PceMemory), CpuType::Pce);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::SmsMemory), CpuType::Sms);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::GbaMemory), CpuType::Gba);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::WsMemory), CpuType::Ws);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::LynxMemory), CpuType::Lynx);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::GenesisMemory), CpuType::Genesis);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::Atari2600Memory), CpuType::Atari2600);
	EXPECT_EQ(DebugUtilities::ToCpuType(MemoryType::ChannelFMemory), CpuType::ChannelF);
}

TEST(DebugUtilitiesTests, SubMemoryTypesContinueToMapToExpectedCpuOwners) {
	struct ExpectedOwner {
		MemoryType Type;
		CpuType Cpu;
	};

	static constexpr std::array<ExpectedOwner, 91> kExpected = {{
		{MemoryType::SnesCgRam, CpuType::Snes},
		{MemoryType::SnesPrgRom, CpuType::Snes},
		{MemoryType::SnesSaveRam, CpuType::Snes},
		{MemoryType::SnesSpriteRam, CpuType::Snes},
		{MemoryType::SnesVideoRam, CpuType::Snes},
		{MemoryType::SnesWorkRam, CpuType::Snes},
		{MemoryType::BsxMemoryPack, CpuType::Snes},
		{MemoryType::BsxPsRam, CpuType::Snes},
		{MemoryType::SufamiTurboFirmware, CpuType::Snes},
		{MemoryType::SufamiTurboSecondCart, CpuType::Snes},
		{MemoryType::SufamiTurboSecondCartRam, CpuType::Snes},
		{MemoryType::SnesRegister, CpuType::Snes},
		{MemoryType::SpcRam, CpuType::Spc},
		{MemoryType::SpcRom, CpuType::Spc},
		{MemoryType::SpcDspRegisters, CpuType::Spc},
		{MemoryType::GsuWorkRam, CpuType::Gsu},
		{MemoryType::Sa1InternalRam, CpuType::Sa1},
		{MemoryType::DspDataRam, CpuType::NecDsp},
		{MemoryType::DspDataRom, CpuType::NecDsp},
		{MemoryType::DspProgramRom, CpuType::NecDsp},
		{MemoryType::Cx4DataRam, CpuType::Cx4},
		{MemoryType::St018PrgRom, CpuType::St018},
		{MemoryType::St018DataRom, CpuType::St018},
		{MemoryType::St018WorkRam, CpuType::St018},
		{MemoryType::GbPrgRom, CpuType::Gameboy},
		{MemoryType::GbWorkRam, CpuType::Gameboy},
		{MemoryType::GbCartRam, CpuType::Gameboy},
		{MemoryType::GbHighRam, CpuType::Gameboy},
		{MemoryType::GbBootRom, CpuType::Gameboy},
		{MemoryType::GbVideoRam, CpuType::Gameboy},
		{MemoryType::GbSpriteRam, CpuType::Gameboy},
		{MemoryType::NesChrRam, CpuType::Nes},
		{MemoryType::NesChrRom, CpuType::Nes},
		{MemoryType::NesInternalRam, CpuType::Nes},
		{MemoryType::NesNametableRam, CpuType::Nes},
		{MemoryType::NesMapperRam, CpuType::Nes},
		{MemoryType::NesPaletteRam, CpuType::Nes},
		{MemoryType::NesPpuMemory, CpuType::Nes},
		{MemoryType::NesPrgRom, CpuType::Nes},
		{MemoryType::NesSaveRam, CpuType::Nes},
		{MemoryType::NesSpriteRam, CpuType::Nes},
		{MemoryType::NesWorkRam, CpuType::Nes},
		{MemoryType::PcePrgRom, CpuType::Pce},
		{MemoryType::PceWorkRam, CpuType::Pce},
		{MemoryType::PceSaveRam, CpuType::Pce},
		{MemoryType::PceCdromRam, CpuType::Pce},
		{MemoryType::PceCardRam, CpuType::Pce},
		{MemoryType::PceAdpcmRam, CpuType::Pce},
		{MemoryType::PceArcadeCardRam, CpuType::Pce},
		{MemoryType::PceVideoRam, CpuType::Pce},
		{MemoryType::PceVideoRamVdc2, CpuType::Pce},
		{MemoryType::PcePaletteRam, CpuType::Pce},
		{MemoryType::PceSpriteRam, CpuType::Pce},
		{MemoryType::PceSpriteRamVdc2, CpuType::Pce},
		{MemoryType::SmsPrgRom, CpuType::Sms},
		{MemoryType::SmsWorkRam, CpuType::Sms},
		{MemoryType::SmsCartRam, CpuType::Sms},
		{MemoryType::SmsBootRom, CpuType::Sms},
		{MemoryType::SmsVideoRam, CpuType::Sms},
		{MemoryType::SmsPaletteRam, CpuType::Sms},
		{MemoryType::SmsPort, CpuType::Sms},
		{MemoryType::GbaPrgRom, CpuType::Gba},
		{MemoryType::GbaBootRom, CpuType::Gba},
		{MemoryType::GbaSaveRam, CpuType::Gba},
		{MemoryType::GbaIntWorkRam, CpuType::Gba},
		{MemoryType::GbaExtWorkRam, CpuType::Gba},
		{MemoryType::GbaVideoRam, CpuType::Gba},
		{MemoryType::GbaSpriteRam, CpuType::Gba},
		{MemoryType::GbaPaletteRam, CpuType::Gba},
		{MemoryType::WsPrgRom, CpuType::Ws},
		{MemoryType::WsWorkRam, CpuType::Ws},
		{MemoryType::WsCartRam, CpuType::Ws},
		{MemoryType::WsCartEeprom, CpuType::Ws},
		{MemoryType::WsBootRom, CpuType::Ws},
		{MemoryType::WsInternalEeprom, CpuType::Ws},
		{MemoryType::WsPort, CpuType::Ws},
		{MemoryType::LynxPrgRom, CpuType::Lynx},
		{MemoryType::LynxWorkRam, CpuType::Lynx},
		{MemoryType::LynxBootRom, CpuType::Lynx},
		{MemoryType::LynxSaveRam, CpuType::Lynx},
		{MemoryType::GenesisPrgRom, CpuType::Genesis},
		{MemoryType::GenesisWorkRam, CpuType::Genesis},
		{MemoryType::GenesisVideoRam, CpuType::Genesis},
		{MemoryType::GenesisPaletteRam, CpuType::Genesis},
		{MemoryType::Atari2600PrgRom, CpuType::Atari2600},
		{MemoryType::Atari2600Ram, CpuType::Atari2600},
		{MemoryType::Atari2600TiaRegisters, CpuType::Atari2600},
		{MemoryType::ChannelFBiosRom, CpuType::ChannelF},
		{MemoryType::ChannelFCartRom, CpuType::ChannelF},
		{MemoryType::ChannelFVideoRam, CpuType::ChannelF}
	}};

	for (const ExpectedOwner& expected : kExpected) {
		EXPECT_EQ(DebugUtilities::ToCpuType(expected.Type), expected.Cpu);
	}
}

TEST(DebugUtilitiesTests, MemoryClassificationParityRemainsStable) {
	static constexpr std::array<MemoryType, 24> kPpuMemoryTypes = {{
		MemoryType::SnesVideoRam,
		MemoryType::SnesSpriteRam,
		MemoryType::SnesCgRam,
		MemoryType::GbVideoRam,
		MemoryType::GbSpriteRam,
		MemoryType::NesChrRam,
		MemoryType::NesChrRom,
		MemoryType::NesSpriteRam,
		MemoryType::NesPaletteRam,
		MemoryType::NesNametableRam,
		MemoryType::NesSecondarySpriteRam,
		MemoryType::NesPpuMemory,
		MemoryType::PceVideoRam,
		MemoryType::PceVideoRamVdc2,
		MemoryType::PcePaletteRam,
		MemoryType::PceSpriteRam,
		MemoryType::PceSpriteRamVdc2,
		MemoryType::SmsVideoRam,
		MemoryType::SmsPaletteRam,
		MemoryType::GbaVideoRam,
		MemoryType::GbaSpriteRam,
		MemoryType::GbaPaletteRam,
		MemoryType::GenesisVideoRam,
		MemoryType::GenesisPaletteRam
	}};

	static constexpr std::array<MemoryType, 24> kRomMemoryTypes = {{
		MemoryType::SnesPrgRom,
		MemoryType::GbPrgRom,
		MemoryType::GbBootRom,
		MemoryType::NesPrgRom,
		MemoryType::NesChrRom,
		MemoryType::PcePrgRom,
		MemoryType::DspDataRom,
		MemoryType::DspProgramRom,
		MemoryType::St018PrgRom,
		MemoryType::St018DataRom,
		MemoryType::SufamiTurboFirmware,
		MemoryType::SufamiTurboSecondCart,
		MemoryType::SpcRom,
		MemoryType::SmsPrgRom,
		MemoryType::SmsBootRom,
		MemoryType::GbaPrgRom,
		MemoryType::GbaBootRom,
		MemoryType::WsPrgRom,
		MemoryType::LynxPrgRom,
		MemoryType::LynxBootRom,
		MemoryType::GenesisPrgRom,
		MemoryType::Atari2600PrgRom,
		MemoryType::ChannelFBiosRom,
		MemoryType::ChannelFCartRom
	}};

	static constexpr std::array<MemoryType, 10> kNonVolatileRamMemoryTypes = {{
		MemoryType::NesSaveRam,
		MemoryType::GbCartRam,
		MemoryType::SnesSaveRam,
		MemoryType::SufamiTurboSecondCartRam,
		MemoryType::PceSaveRam,
		MemoryType::SnesRegister,
		MemoryType::SmsCartRam,
		MemoryType::GbaSaveRam,
		MemoryType::WsCartRam,
		MemoryType::LynxSaveRam
	}};

	for (MemoryType memoryType : kPpuMemoryTypes) {
		EXPECT_TRUE(DebugUtilities::IsPpuMemory(memoryType));
	}

	for (MemoryType memoryType : kRomMemoryTypes) {
		EXPECT_TRUE(DebugUtilities::IsRom(memoryType));
		EXPECT_FALSE(DebugUtilities::IsVolatileRam(memoryType));
	}

	for (MemoryType memoryType : kNonVolatileRamMemoryTypes) {
		EXPECT_FALSE(DebugUtilities::IsVolatileRam(memoryType));
	}

	EXPECT_FALSE(DebugUtilities::IsPpuMemory(MemoryType::SnesMemory));
	EXPECT_FALSE(DebugUtilities::IsPpuMemory(MemoryType::NesMemory));
	EXPECT_FALSE(DebugUtilities::IsRom(MemoryType::GbaVideoRam));
	EXPECT_FALSE(DebugUtilities::IsRom(MemoryType::SnesWorkRam));
	EXPECT_TRUE(DebugUtilities::IsVolatileRam(MemoryType::SnesWorkRam));
	EXPECT_TRUE(DebugUtilities::IsVolatileRam(MemoryType::GbWorkRam));
}
