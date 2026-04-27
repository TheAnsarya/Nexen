#include "pch.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/Input/GenesisController.h"
#include "Utilities/Serializer.h"

namespace {
	std::unique_ptr<GenesisControlManager> CreateControlManager(Emulator& emu, GenesisConsole& console, ControllerType port1Type = ControllerType::GenesisController, ControllerType port2Type = ControllerType::None) {
		emu.Initialize(false);
		GenesisConfig& cfg = emu.GetSettings()->GetGenesisConfig();
		cfg.Port1.Type = port1Type;
		cfg.Port2.Type = port2Type;

		auto manager = std::make_unique<GenesisControlManager>(&emu, &console);
		manager->UpdateControlDevices();
		return manager;
	}

	std::string SaveManagerState(GenesisControlManager& manager) {
		Serializer saver(1, true, SerializeFormat::Binary);
		manager.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		return state.str();
	}

	void LoadManagerState(GenesisControlManager& manager, const std::string& stateData) {
		std::stringstream state(stateData);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		manager.Serialize(loader);
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

	TEST(GenesisControlManagerTests, CreatedDeviceReportsGenesisControllerType) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console);
		ASSERT_NE(controlManager, nullptr);

		auto device = controlManager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		EXPECT_TRUE(device->HasControllerType(ControllerType::GenesisController));
	}

	TEST(GenesisControlManagerTests, ThreeButtonReadPathIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		GenesisConsole consoleA(&emuA);
		std::unique_ptr<GenesisControlManager> managerA = CreateControlManager(emuA, consoleA);
		ASSERT_NE(managerA, nullptr);

		auto deviceA = managerA->GetControlDevice(0);
		ASSERT_NE(deviceA, nullptr);
		deviceA->SetBit(GenesisController::Buttons::Up);
		deviceA->SetBit(GenesisController::Buttons::B);
		deviceA->SetBit(GenesisController::Buttons::C);

		managerA->WriteDataPort(0, 0x40);
		uint8_t baselinePre = managerA->ReadDataPort(0);
		std::string stateData = SaveManagerState(*managerA);

		managerA->WriteDataPort(0, 0x00);
		uint8_t baselineTail = managerA->ReadDataPort(0);

		Emulator emuB;
		GenesisConsole consoleB(&emuB);
		std::unique_ptr<GenesisControlManager> managerB = CreateControlManager(emuB, consoleB);
		ASSERT_NE(managerB, nullptr);

		auto deviceB = managerB->GetControlDevice(0);
		ASSERT_NE(deviceB, nullptr);
		deviceB->SetBit(GenesisController::Buttons::Up);
		deviceB->SetBit(GenesisController::Buttons::B);
		deviceB->SetBit(GenesisController::Buttons::C);
		LoadManagerState(*managerB, stateData);

		managerB->WriteDataPort(0, 0x00);
		uint8_t replayTail = managerB->ReadDataPort(0);

		EXPECT_NE(baselinePre, 0xFFu);
		EXPECT_EQ(baselineTail, replayTail);
	}

	TEST(GenesisControlManagerTests, SixButtonReadPathIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		GenesisConsole consoleA(&emuA);
		std::unique_ptr<GenesisControlManager> managerA = CreateControlManager(emuA, consoleA);
		ASSERT_NE(managerA, nullptr);

		auto deviceA = managerA->GetControlDevice(0);
		ASSERT_NE(deviceA, nullptr);
		deviceA->SetBit(GenesisController::Buttons::X);
		deviceA->SetBit(GenesisController::Buttons::Y);
		deviceA->SetBit(GenesisController::Buttons::Z);
		deviceA->SetBit(GenesisController::Buttons::Mode);

		managerA->WriteDataPort(0, 0x40);
		managerA->WriteDataPort(0, 0x00);
		managerA->WriteDataPort(0, 0x40);
		managerA->WriteDataPort(0, 0x00);
		managerA->WriteDataPort(0, 0x40);
		uint8_t baselinePre = managerA->ReadDataPort(0);
		std::string stateData = SaveManagerState(*managerA);

		managerA->WriteDataPort(0, 0x00);
		uint8_t baselineTail = managerA->ReadDataPort(0);

		Emulator emuB;
		GenesisConsole consoleB(&emuB);
		std::unique_ptr<GenesisControlManager> managerB = CreateControlManager(emuB, consoleB);
		ASSERT_NE(managerB, nullptr);

		auto deviceB = managerB->GetControlDevice(0);
		ASSERT_NE(deviceB, nullptr);
		deviceB->SetBit(GenesisController::Buttons::X);
		deviceB->SetBit(GenesisController::Buttons::Y);
		deviceB->SetBit(GenesisController::Buttons::Z);
		deviceB->SetBit(GenesisController::Buttons::Mode);
		LoadManagerState(*managerB, stateData);

		managerB->WriteDataPort(0, 0x00);
		uint8_t replayTail = managerB->ReadDataPort(0);

		EXPECT_NE(baselinePre, 0xFFu);
		EXPECT_EQ(baselineTail, replayTail);
	}

	TEST(GenesisControlManagerTests, DisconnectedControllerFamilyIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		GenesisConsole consoleA(&emuA);
		std::unique_ptr<GenesisControlManager> managerA = CreateControlManager(emuA, consoleA, ControllerType::None, ControllerType::None);
		ASSERT_NE(managerA, nullptr);

		uint8_t baselineBefore = managerA->ReadDataPort(0);
		std::string stateData = SaveManagerState(*managerA);
		uint8_t baselineAfter = managerA->ReadDataPort(0);

		Emulator emuB;
		GenesisConsole consoleB(&emuB);
		std::unique_ptr<GenesisControlManager> managerB = CreateControlManager(emuB, consoleB, ControllerType::None, ControllerType::None);
		ASSERT_NE(managerB, nullptr);
		LoadManagerState(*managerB, stateData);
		uint8_t replayAfter = managerB->ReadDataPort(0);

		EXPECT_EQ(baselineBefore, 0x7Fu);
		EXPECT_EQ(baselineAfter, replayAfter);
	}
}
