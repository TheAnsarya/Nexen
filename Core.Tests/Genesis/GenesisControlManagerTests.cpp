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

	void PrimeSixButtonSession(GenesisControlManager& manager, uint8_t port) {
		manager.WriteControlPort(port, 0x40);
		manager.WriteDataPort(port, 0x00);
		manager.WriteDataPort(port, 0x40);
		manager.WriteDataPort(port, 0x00);
		manager.WriteDataPort(port, 0x40);
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

		controlManager->WriteControlPort(0, 0x40);
		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x40);
		uint8_t preIdentificationRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(preIdentificationRead, 0x4Fu);

		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x40);
		uint8_t sixButtonRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(sixButtonRead, 0x40u);

		controlManager->WriteDataPort(0, 0x00);
		uint8_t lowReadAfterIdentification = controlManager->ReadDataPort(0);
		EXPECT_EQ(lowReadAfterIdentification, 0x3Fu);
	}

	TEST(GenesisControlManagerTests, ThCounterOnlyAdvancesOnLowToHighTransitions) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console);
		ASSERT_NE(controlManager, nullptr);
		controlManager->WriteControlPort(0, 0x40);

		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x00);
		EXPECT_EQ(controlManager->GetThCount(0), 0u);

		controlManager->WriteDataPort(0, 0x40);
		controlManager->WriteDataPort(0, 0x40);
		EXPECT_EQ(controlManager->GetThCount(0), 2u);

		controlManager->WriteDataPort(0, 0x00);
		controlManager->WriteDataPort(0, 0x00);
		EXPECT_EQ(controlManager->GetThCount(0), 2u);

		controlManager->WriteDataPort(0, 0x40);
		EXPECT_EQ(controlManager->GetThCount(0), 4u);
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
		managerA->WriteControlPort(0, 0x40);

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

		PrimeSixButtonSession(*managerA, 0);
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

	TEST(GenesisControlManagerTests, DeterministicPortCapabilitiesAndDigestAreReplayStable) {
		Emulator emuA;
		GenesisConsole consoleA(&emuA);
		std::unique_ptr<GenesisControlManager> managerA = CreateControlManager(emuA, consoleA);
		ASSERT_NE(managerA, nullptr);
		PrimeSixButtonSession(*managerA, 0);
		managerA->WriteControlPort(1, 0x40);
		managerA->WriteDataPort(1, 0x40);
		managerA->WriteDataPort(1, 0x00);

		uint8_t p1Caps = managerA->GetDeterministicPortCapabilities(0);
		uint8_t p1Digest = managerA->GetDeterministicPortDigest(0);
		uint8_t p2Caps = managerA->GetDeterministicPortCapabilities(1);
		uint8_t p2Digest = managerA->GetDeterministicPortDigest(1);
		std::string stateData = SaveManagerState(*managerA);

		Emulator emuB;
		GenesisConsole consoleB(&emuB);
		std::unique_ptr<GenesisControlManager> managerB = CreateControlManager(emuB, consoleB);
		ASSERT_NE(managerB, nullptr);
		LoadManagerState(*managerB, stateData);

		EXPECT_EQ(managerB->GetDeterministicPortCapabilities(0), p1Caps);
		EXPECT_EQ(managerB->GetDeterministicPortDigest(0), p1Digest);
		EXPECT_EQ(managerB->GetDeterministicPortCapabilities(1), p2Caps);
		EXPECT_EQ(managerB->GetDeterministicPortDigest(1), p2Digest);
		EXPECT_NE(p1Caps & 0x10u, 0u);
		EXPECT_NE(p2Caps & 0x20u, 0u);
	}

	TEST(GenesisControlManagerTests, ControlPortDirectionMaskOverridesInputBitsForOutputLines) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console);
		ASSERT_NE(controlManager, nullptr);

		controlManager->WriteControlPort(0, 0x48);

		controlManager->WriteDataPort(0, 0x40);
		uint8_t highWithOutputLow = controlManager->ReadDataPort(0);
		EXPECT_EQ(highWithOutputLow & 0x08u, 0x00u);

		controlManager->WriteDataPort(0, 0x48);
		uint8_t highWithOutputHigh = controlManager->ReadDataPort(0);
		EXPECT_EQ(highWithOutputHigh & 0x08u, 0x08u);
		EXPECT_EQ(highWithOutputHigh & 0x40u, 0x40u);
	}

	TEST(GenesisControlManagerTests, SixButtonSessionTimesOutWithoutThTransitions) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console);
		ASSERT_NE(controlManager, nullptr);

		auto device = controlManager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		device->SetBit(GenesisController::Buttons::X);

		PrimeSixButtonSession(*controlManager, 0);

		uint8_t sixButtonRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(sixButtonRead, 0x7bu);

		controlManager->AdvanceMasterClock(13000);
		uint8_t timedOutRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(timedOutRead, 0x7fu);
	}

	TEST(GenesisControlManagerTests, ThreeButtonDeviceNeverExposesSixButtonSignatureOrExtendedButtons) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console, ControllerType::GenesisController3Buttons, ControllerType::None);
		ASSERT_NE(controlManager, nullptr);

		auto device = controlManager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		device->SetBit(GenesisController::Buttons::X);
		device->SetBit(GenesisController::Buttons::Y);
		device->SetBit(GenesisController::Buttons::Z);
		device->SetBit(GenesisController::Buttons::Mode);

		PrimeSixButtonSession(*controlManager, 0);

		uint8_t highRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(highRead, 0x7fu);

		controlManager->WriteDataPort(0, 0x00);
		uint8_t lowRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(lowRead, 0x33u);
	}

	TEST(GenesisControlManagerTests, ThInputDirectionSwitchPreservesHighInputLevel) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console);
		ASSERT_NE(controlManager, nullptr);

		auto device = controlManager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		device->SetBit(GenesisController::Buttons::Down);

		controlManager->WriteControlPort(0, 0x40);
		controlManager->WriteDataPort(0, 0x00);

		controlManager->WriteControlPort(0, 0x20);
		uint8_t immediateRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(immediateRead & 0x40u, 0x40u);

		controlManager->AdvanceMasterClock(200);
		uint8_t delayedRead = controlManager->ReadDataPort(0);
		EXPECT_EQ(delayedRead & 0x40u, 0x40u);
	}

	TEST(GenesisControlManagerTests, CreateControllerDeviceRespectsGenesisThreeButtonControllerType) {
		Emulator emu;
		GenesisConsole console(&emu);
		std::unique_ptr<GenesisControlManager> controlManager = CreateControlManager(emu, console, ControllerType::GenesisController3Buttons, ControllerType::None);
		ASSERT_NE(controlManager, nullptr);

		auto device = controlManager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		EXPECT_TRUE(device->HasControllerType(ControllerType::GenesisController3Buttons));
		EXPECT_FALSE(device->HasControllerType(ControllerType::GenesisController));
	}

	TEST(GenesisControlManagerTests, ConstructorImmediatelyBuildsGameplayPortDevices) {
		Emulator emu;
		emu.Initialize(false);
		GenesisConfig& cfg = emu.GetSettings()->GetGenesisConfig();
		cfg.Port1.Type = ControllerType::GenesisController;
		cfg.Port2.Type = ControllerType::GenesisController3Buttons;

		GenesisConsole console(&emu);
		auto manager = std::make_unique<GenesisControlManager>(&emu, &console);
		ASSERT_NE(manager, nullptr);

		auto p1 = manager->GetControlDevice(0);
		auto p2 = manager->GetControlDevice(1);
		ASSERT_NE(p1, nullptr);
		ASSERT_NE(p2, nullptr);
		EXPECT_TRUE(p1->HasControllerType(ControllerType::GenesisController));
		EXPECT_TRUE(p2->HasControllerType(ControllerType::GenesisController3Buttons));
	}

	TEST(GenesisControlManagerTests, SerializeLoadRebuildsDevicesAndPreservesLatches) {
		Emulator emuA;
		GenesisConsole consoleA(&emuA);
		auto managerA = CreateControlManager(emuA, consoleA, ControllerType::GenesisController, ControllerType::GenesisController3Buttons);
		ASSERT_NE(managerA, nullptr);

		managerA->WriteControlPort(0, 0x48);
		managerA->WriteDataPort(0, 0x40);
		managerA->WriteControlPort(1, 0x40);
		managerA->WriteDataPort(1, 0x00);
		managerA->AdvanceMasterClock(256);

		uint8_t p1Data = managerA->GetDataPortWriteLatch(0);
		uint8_t p1Ctrl = managerA->GetControlPortWriteLatch(0);
		uint8_t p2Data = managerA->GetDataPortWriteLatch(1);
		uint8_t p2Ctrl = managerA->GetControlPortWriteLatch(1);
		uint8_t p1ThCount = managerA->GetThCount(0);
		uint8_t p2ThCount = managerA->GetThCount(1);
		std::string stateData = SaveManagerState(*managerA);

		Emulator emuB;
		GenesisConsole consoleB(&emuB);
		auto managerB = CreateControlManager(emuB, consoleB, ControllerType::GenesisController, ControllerType::GenesisController3Buttons);
		ASSERT_NE(managerB, nullptr);
		LoadManagerState(*managerB, stateData);

		auto p1 = managerB->GetControlDevice(0);
		auto p2 = managerB->GetControlDevice(1);
		ASSERT_NE(p1, nullptr);
		ASSERT_NE(p2, nullptr);

		EXPECT_EQ(managerB->GetDataPortWriteLatch(0), p1Data);
		EXPECT_EQ(managerB->GetControlPortWriteLatch(0), p1Ctrl);
		EXPECT_EQ(managerB->GetDataPortWriteLatch(1), p2Data);
		EXPECT_EQ(managerB->GetControlPortWriteLatch(1), p2Ctrl);
		EXPECT_EQ(managerB->GetThCount(0), p1ThCount);
		EXPECT_EQ(managerB->GetThCount(1), p2ThCount);
		EXPECT_TRUE(p1->HasControllerType(ControllerType::GenesisController));
		EXPECT_TRUE(p2->HasControllerType(ControllerType::GenesisController3Buttons));
	}

	TEST(GenesisControlManagerTests, DefaultMappingFallbackStillProducesReadableButtonState) {
		Emulator emu;
		emu.Initialize(false);
		GenesisConfig& cfg = emu.GetSettings()->GetGenesisConfig();
		cfg.Port1.Type = ControllerType::GenesisController;
		cfg.Port1.Keys = {};

		GenesisConsole console(&emu);
		auto manager = std::make_unique<GenesisControlManager>(&emu, &console);
		ASSERT_NE(manager, nullptr);

		auto device = manager->GetControlDevice(0);
		ASSERT_NE(device, nullptr);
		device->SetBit(GenesisController::Buttons::Up);
		device->SetBit(GenesisController::Buttons::A);
		device->SetBit(GenesisController::Buttons::Start);

		manager->WriteControlPort(0, 0x40);
		manager->WriteDataPort(0, 0x00);
		uint8_t lowRead = manager->ReadDataPort(0);
		manager->WriteDataPort(0, 0x40);
		uint8_t highRead = manager->ReadDataPort(0);

		EXPECT_NE(lowRead, 0x7fu);
		EXPECT_NE(highRead, 0x7fu);
		EXPECT_EQ((highRead & 0x01u), 0x00u);
	}
}
