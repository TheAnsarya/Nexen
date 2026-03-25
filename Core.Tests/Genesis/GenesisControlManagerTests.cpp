#include "pch.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/Input/GenesisController.h"

namespace {
	std::unique_ptr<GenesisControlManager> CreateControlManager(Emulator& emu, GenesisConsole& console) {
		emu.Initialize(false);
		GenesisConfig& cfg = emu.GetSettings()->GetGenesisConfig();
		cfg.Port1.Type = ControllerType::GenesisController;

		auto manager = std::make_unique<GenesisControlManager>(&emu, &console);
		manager->UpdateControlDevices();
		return manager;
	}

	TEST(GenesisControlManagerTests, SixButtonReadPathActivatesAfterThIdentification) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console);
		ASSERT_NE(controlManager, nullptr);

		auto device = controlManager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		device->SetBit(GenesisController::Buttons::B);
		device->SetBit(GenesisController::Buttons::C);
		device->SetBit(GenesisController::Buttons::X);
		device->SetBit(GenesisController::Buttons::Y);
		device->SetBit(GenesisController::Buttons::Z);
		device->SetBit(GenesisController::Buttons::Mode);

		controlManager->WriteDataPort(0, 0x40);
		uint8_t preIdentificationRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(preIdentificationRead, 0x4Fu);

		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x40);
		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x40);

		uint8_t sixButtonRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(sixButtonRead, 0x40u);

		controlManager->WriteDataPort(0, 0x00);
		uint8_t lowReadAfterIdentification = controlManager->ReadDataPort(0);
		EXPECT_EQ(lowReadAfterIdentification, 0x3Fu);
	}

	TEST(GenesisControlManagerTests, ThCounterOnlyAdvancesOnStateTransitions) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console);
		ASSERT_NE(controlManager, nullptr);

		auto device = controlManager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		device->SetBit(GenesisController::Buttons::B);
		device->SetBit(GenesisController::Buttons::C);
		device->SetBit(GenesisController::Buttons::X);
		device->SetBit(GenesisController::Buttons::Y);
		device->SetBit(GenesisController::Buttons::Z);
		device->SetBit(GenesisController::Buttons::Mode);

		controlManager->WriteDataPort(0, 0x40);
		controlManager->WriteDataPort(0, 0x40);
		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x40);
		controlManager->WriteDataPort(0, 0x40);

		uint8_t readBeforeFourthTransition = controlManager->ReadDataPort(0);
		EXPECT_EQ(readBeforeFourthTransition, 0x4Fu);

		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x40);

		uint8_t readAfterFourthTransition = controlManager->ReadDataPort(0);
		EXPECT_EQ(readAfterFourthTransition, 0x40u);
	}
}
