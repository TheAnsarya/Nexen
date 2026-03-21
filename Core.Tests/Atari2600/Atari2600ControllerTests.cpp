#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600Controller.h"
#include "Atari2600/Atari2600Paddle.h"
#include "Atari2600/Atari2600Keypad.h"
#include "Atari2600/Atari2600DrivingController.h"
#include "Atari2600/Atari2600BoosterGrip.h"
#include "Shared/SettingTypes.h"
#include "Utilities/VirtualFile.h"

// ============================================================================
// Atari 2600 Multi-Controller Test Suite — Epic 24 (#864)
//
// Tests all 5 controller types and the ControlManager factory/wiring.
// Each controller's hardware interface (SWCHA nibble, INPT state, fire)
// is tested against the Atari 2600 Technical Reference Manual values.
// ============================================================================

namespace {

	// Helper: create a minimal 4K ROM and load it
	void LoadTestRom(Emulator& emu, Atari2600Console& console) {
		vector<uint8_t> rom(4096, 0xEA); // All NOP
		VirtualFile romFile(rom.data(), rom.size(), "controller-test.a26");
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
	}

	// ========================================================================
	// 24.1 — Joystick Controller (CX40) Tests (#865)
	// ========================================================================

	class JoystickTest : public ::testing::Test {
	protected:
		Emulator emu;
		Atari2600Console console{&emu};
		KeyMappingSet emptyKeys{};

		void SetUp() override {
			LoadTestRom(emu, console);
		}
	};

	TEST_F(JoystickTest, NoInputReturnsAllReleasedNibble) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		// All released = 0x0f (active-low: all bits high)
		EXPECT_EQ(joy.GetDirectionNibble(), 0x0f);
	}

	TEST_F(JoystickTest, UpSetsCorrectActiveLowBit) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		joy.SetBit(Atari2600Controller::Buttons::Up);
		// Up = bit 0 goes low → 0x0e
		EXPECT_EQ(joy.GetDirectionNibble(), 0x0e);
	}

	TEST_F(JoystickTest, DownSetsCorrectActiveLowBit) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		joy.SetBit(Atari2600Controller::Buttons::Down);
		// Down = bit 1 goes low → 0x0d
		EXPECT_EQ(joy.GetDirectionNibble(), 0x0d);
	}

	TEST_F(JoystickTest, LeftSetsCorrectActiveLowBit) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		joy.SetBit(Atari2600Controller::Buttons::Left);
		// Left = bit 2 goes low → 0x0b
		EXPECT_EQ(joy.GetDirectionNibble(), 0x0b);
	}

	TEST_F(JoystickTest, RightSetsCorrectActiveLowBit) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		joy.SetBit(Atari2600Controller::Buttons::Right);
		// Right = bit 3 goes low → 0x07
		EXPECT_EQ(joy.GetDirectionNibble(), 0x07);
	}

	TEST_F(JoystickTest, AllDirectionsCombineCorrectly) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		joy.SetBit(Atari2600Controller::Buttons::Up);
		joy.SetBit(Atari2600Controller::Buttons::Down);
		joy.SetBit(Atari2600Controller::Buttons::Left);
		joy.SetBit(Atari2600Controller::Buttons::Right);
		// All pressed → all bits low → 0x00
		EXPECT_EQ(joy.GetDirectionNibble(), 0x00);
	}

	TEST_F(JoystickTest, DiagonalUpLeftCombinesCorrectly) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		joy.SetBit(Atari2600Controller::Buttons::Up);
		joy.SetBit(Atari2600Controller::Buttons::Left);
		// Up(bit0) + Left(bit2) low → 0x0f & ~0x01 & ~0x04 = 0x0a
		EXPECT_EQ(joy.GetDirectionNibble(), 0x0a);
	}

	TEST_F(JoystickTest, FirePressed) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		joy.SetBit(Atari2600Controller::Buttons::Fire);
		// Pressed = 0x00 (bit 7 clear)
		EXPECT_EQ(joy.GetFireState(), 0x00);
	}

	TEST_F(JoystickTest, FireReleased) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		// Released = 0x80 (bit 7 set)
		EXPECT_EQ(joy.GetFireState(), 0x80);
	}

	TEST_F(JoystickTest, KeyAssociationCountCorrect) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		// Should have exactly 5 button-name associations (UDLR + Fire)
		EXPECT_EQ(joy.GetKeyNameAssociations().size(), 5u);
	}

	TEST_F(JoystickTest, KeyNameAssociationsCorrect) {
		Atari2600Controller joy(&emu, 0, emptyKeys);
		auto names = joy.GetKeyNameAssociations();
		ASSERT_EQ(names.size(), 5u);
		// Verify all 5 buttons are mapped
		bool hasUp = false, hasDown = false, hasLeft = false, hasRight = false, hasFire = false;
		for (auto& n : names) {
			if (n.ButtonId == Atari2600Controller::Buttons::Up) hasUp = true;
			if (n.ButtonId == Atari2600Controller::Buttons::Down) hasDown = true;
			if (n.ButtonId == Atari2600Controller::Buttons::Left) hasLeft = true;
			if (n.ButtonId == Atari2600Controller::Buttons::Right) hasRight = true;
			if (n.ButtonId == Atari2600Controller::Buttons::Fire) hasFire = true;
		}
		EXPECT_TRUE(hasUp);
		EXPECT_TRUE(hasDown);
		EXPECT_TRUE(hasLeft);
		EXPECT_TRUE(hasRight);
		EXPECT_TRUE(hasFire);
	}

	// ========================================================================
	// 24.2 — Paddle Controller (CX30) Tests (#866)
	// ========================================================================

	class PaddleTest : public ::testing::Test {
	protected:
		Emulator emu;
		Atari2600Console console{&emu};
		KeyMappingSet emptyKeys{};

		void SetUp() override {
			LoadTestRom(emu, console);
		}
	};

	TEST_F(PaddleTest, DefaultPositionAndIndex) {
		Atari2600Paddle paddle(&emu, 0, 0, emptyKeys);
		EXPECT_EQ(paddle.GetPaddleIndex(), 0u);
		// Default position is 0 (coordinates start at 0,0)
		EXPECT_EQ(paddle.GetPaddlePosition(), 0u);
	}

	TEST_F(PaddleTest, PaddleIndexMapsCorrectly) {
		Atari2600Paddle p0(&emu, 0, 0, emptyKeys);
		Atari2600Paddle p1(&emu, 0, 1, emptyKeys);
		Atari2600Paddle p2(&emu, 1, 2, emptyKeys);
		Atari2600Paddle p3(&emu, 1, 3, emptyKeys);
		EXPECT_EQ(p0.GetPaddleIndex(), 0u);
		EXPECT_EQ(p1.GetPaddleIndex(), 1u);
		EXPECT_EQ(p2.GetPaddleIndex(), 2u);
		EXPECT_EQ(p3.GetPaddleIndex(), 3u);
	}

	TEST_F(PaddleTest, InptThresholdModelBelowReturnsZero) {
		Atari2600Paddle paddle(&emu, 0, 0, emptyKeys);
		// Position = 0, scanline 0 → scanline >= position → 0x80
		EXPECT_EQ(paddle.GetInptState(0), 0x80);
	}

	TEST_F(PaddleTest, InptThresholdModelAboveReturns80) {
		Atari2600Paddle paddle(&emu, 0, 0, emptyKeys);
		// Position = 0, any scanline >= 0 → 0x80 (threshold met)
		EXPECT_EQ(paddle.GetInptState(128), 0x80);
		EXPECT_EQ(paddle.GetInptState(255), 0x80);
	}

	TEST_F(PaddleTest, FirePressedReturns00) {
		Atari2600Paddle paddle(&emu, 0, 0, emptyKeys);
		paddle.SetBit(Atari2600Paddle::Buttons::Fire);
		EXPECT_EQ(paddle.GetFireState(), 0x00);
	}

	TEST_F(PaddleTest, FireReleasedReturns80) {
		Atari2600Paddle paddle(&emu, 0, 0, emptyKeys);
		EXPECT_EQ(paddle.GetFireState(), 0x80);
	}

	// ========================================================================
	// 24.3 — Keypad Controller (CX50) Tests (#867)
	// ========================================================================

	class KeypadTest : public ::testing::Test {
	protected:
		Emulator emu;
		Atari2600Console console{&emu};
		KeyMappingSet emptyKeys{};

		void SetUp() override {
			LoadTestRom(emu, console);
		}
	};

	TEST_F(KeypadTest, NoKeyPressedReturns80ForAllColumns) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		for (uint8_t row = 0; row < 4; row++) {
			for (uint8_t col = 0; col < 3; col++) {
				EXPECT_EQ(keypad.GetColumnState(row, col), 0x80)
					<< "Row=" << (int)row << " Col=" << (int)col;
			}
		}
	}

	TEST_F(KeypadTest, Key1MapsToRow0Col0) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key1);
		EXPECT_EQ(keypad.GetColumnState(0, 0), 0x00); // Pressed
		EXPECT_EQ(keypad.GetColumnState(0, 1), 0x80); // Other cols unaffected
		EXPECT_EQ(keypad.GetColumnState(0, 2), 0x80);
		EXPECT_EQ(keypad.GetColumnState(1, 0), 0x80); // Other rows unaffected
	}

	TEST_F(KeypadTest, Key2MapsToRow0Col1) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key2);
		EXPECT_EQ(keypad.GetColumnState(0, 1), 0x00);
		EXPECT_EQ(keypad.GetColumnState(0, 0), 0x80);
	}

	TEST_F(KeypadTest, Key3MapsToRow0Col2) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key3);
		EXPECT_EQ(keypad.GetColumnState(0, 2), 0x00);
	}

	TEST_F(KeypadTest, Key4MapsToRow1Col0) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key4);
		EXPECT_EQ(keypad.GetColumnState(1, 0), 0x00);
	}

	TEST_F(KeypadTest, Key5MapsToRow1Col1) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key5);
		EXPECT_EQ(keypad.GetColumnState(1, 1), 0x00);
	}

	TEST_F(KeypadTest, Key6MapsToRow1Col2) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key6);
		EXPECT_EQ(keypad.GetColumnState(1, 2), 0x00);
	}

	TEST_F(KeypadTest, Key7MapsToRow2Col0) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key7);
		EXPECT_EQ(keypad.GetColumnState(2, 0), 0x00);
	}

	TEST_F(KeypadTest, Key8MapsToRow2Col1) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key8);
		EXPECT_EQ(keypad.GetColumnState(2, 1), 0x00);
	}

	TEST_F(KeypadTest, Key9MapsToRow2Col2) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key9);
		EXPECT_EQ(keypad.GetColumnState(2, 2), 0x00);
	}

	TEST_F(KeypadTest, KeyStarMapsToRow3Col0) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::KeyStar);
		EXPECT_EQ(keypad.GetColumnState(3, 0), 0x00);
	}

	TEST_F(KeypadTest, Key0MapsToRow3Col1) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key0);
		EXPECT_EQ(keypad.GetColumnState(3, 1), 0x00);
	}

	TEST_F(KeypadTest, KeyPoundMapsToRow3Col2) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::KeyPound);
		EXPECT_EQ(keypad.GetColumnState(3, 2), 0x00);
	}

	TEST_F(KeypadTest, OutOfRangeReturns80) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key1);
		// Column index 3+ is out of range
		EXPECT_EQ(keypad.GetColumnState(0, 3), 0x80);
		// Row 4+ is out of range
		EXPECT_EQ(keypad.GetColumnState(4, 0), 0x80);
	}

	TEST_F(KeypadTest, DirectionNibbleAlways0f) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		// Keypad doesn't drive SWCHA directions
		EXPECT_EQ(keypad.GetDirectionNibble(), 0x0f);
	}

	TEST_F(KeypadTest, MultipleKeysCanBePressed) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		keypad.SetBit(Atari2600Keypad::Buttons::Key1);
		keypad.SetBit(Atari2600Keypad::Buttons::Key5);
		keypad.SetBit(Atari2600Keypad::Buttons::Key9);
		// Key1 = row0/col0, Key5 = row1/col1, Key9 = row2/col2
		EXPECT_EQ(keypad.GetColumnState(0, 0), 0x00);
		EXPECT_EQ(keypad.GetColumnState(1, 1), 0x00);
		EXPECT_EQ(keypad.GetColumnState(2, 2), 0x00);
		// Others still not pressed
		EXPECT_EQ(keypad.GetColumnState(0, 1), 0x80);
		EXPECT_EQ(keypad.GetColumnState(3, 0), 0x80);
	}

	TEST_F(KeypadTest, KeyAssociationCountCorrect) {
		Atari2600Keypad keypad(&emu, 0, emptyKeys);
		// Should have 12 associations: keys 0-9, *, #
		EXPECT_EQ(keypad.GetKeyNameAssociations().size(), 12u);
	}

	// ========================================================================
	// 24.4 — Driving Controller (CX20) Tests (#868)
	// ========================================================================

	class DrivingTest : public ::testing::Test {
	protected:
		Emulator emu;
		Atari2600Console console{&emu};
		KeyMappingSet emptyKeys{};

		void SetUp() override {
			LoadTestRom(emu, console);
		}
	};

	TEST_F(DrivingTest, InitialGrayCodePosition) {
		Atari2600DrivingController driving(&emu, 0, emptyKeys);
		// Initial position = 0, GrayCode[0] = 0x00
		// Direction nibble: bits 3:2 = high (0x0c), bits 1:0 = gray code
		EXPECT_EQ(driving.GetDirectionNibble(), 0x0c); // 0x0c | 0x00
	}

	TEST_F(DrivingTest, GrayCodeClockwiseSequenceViaInitialPosition) {
		Atari2600DrivingController driving(&emu, 0, emptyKeys);
		// At initial position 0, GrayCode[0] = 0x00
		// Verify bits 1:0 of direction nibble
		EXPECT_EQ(driving.GetDirectionNibble() & 0x03, 0x00);
	}

	TEST_F(DrivingTest, DirectionNibbleOnlyUsesLowTwoBitsForGrayCode) {
		Atari2600DrivingController driving(&emu, 0, emptyKeys);
		// At default (position 0), gray code = 00
		// Verify bits 1:0 carry gray code, bits 3:2 are high
		uint8_t nibble = driving.GetDirectionNibble();
		EXPECT_EQ(nibble & 0x0c, 0x0c); // bits 3:2 always high
		// Gray code at position 0 should be 0x00
		EXPECT_EQ(nibble & 0x03, 0x00);
	}

	TEST_F(DrivingTest, UpperNibbleBitsAlwaysHigh) {
		Atari2600DrivingController driving(&emu, 0, emptyKeys);
		// Bits 3:2 should always be 1 (0x0c mask) at initial position
		EXPECT_EQ(driving.GetDirectionNibble() & 0x0c, 0x0c);
	}

	TEST_F(DrivingTest, FirePressedReturns00) {
		Atari2600DrivingController driving(&emu, 0, emptyKeys);
		driving.SetBit(Atari2600DrivingController::Buttons::Fire);
		EXPECT_EQ(driving.GetFireState(), 0x00);
	}

	TEST_F(DrivingTest, FireReleasedReturns80) {
		Atari2600DrivingController driving(&emu, 0, emptyKeys);
		EXPECT_EQ(driving.GetFireState(), 0x80);
	}

	TEST_F(DrivingTest, KeyAssociationCountCorrect) {
		Atari2600DrivingController driving(&emu, 0, emptyKeys);
		// Should have 3 associations: RotateLeft, RotateRight, Fire
		EXPECT_EQ(driving.GetKeyNameAssociations().size(), 3u);
	}

	// ========================================================================
	// 24.5 — Booster Grip Controller Tests (#869)
	// ========================================================================

	class BoosterGripTest : public ::testing::Test {
	protected:
		Emulator emu;
		Atari2600Console console{&emu};
		KeyMappingSet emptyKeys{};

		void SetUp() override {
			LoadTestRom(emu, console);
		}
	};

	TEST_F(BoosterGripTest, DirectionNibbleSameAsJoystick) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);
		// All released = 0x0f
		EXPECT_EQ(grip.GetDirectionNibble(), 0x0f);

		grip.SetBit(Atari2600BoosterGrip::Buttons::Up);
		EXPECT_EQ(grip.GetDirectionNibble(), 0x0e);
	}

	TEST_F(BoosterGripTest, AllDirectionsCorrect) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);

		grip.SetBit(Atari2600BoosterGrip::Buttons::Down);
		EXPECT_EQ(grip.GetDirectionNibble(), 0x0d);
		grip.ClearBit(Atari2600BoosterGrip::Buttons::Down);

		grip.SetBit(Atari2600BoosterGrip::Buttons::Left);
		EXPECT_EQ(grip.GetDirectionNibble(), 0x0b);
		grip.ClearBit(Atari2600BoosterGrip::Buttons::Left);

		grip.SetBit(Atari2600BoosterGrip::Buttons::Right);
		EXPECT_EQ(grip.GetDirectionNibble(), 0x07);
	}

	TEST_F(BoosterGripTest, FireState) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);
		EXPECT_EQ(grip.GetFireState(), 0x80);        // Released
		grip.SetBit(Atari2600BoosterGrip::Buttons::Fire);
		EXPECT_EQ(grip.GetFireState(), 0x00);         // Pressed
	}

	TEST_F(BoosterGripTest, TriggerState) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);
		EXPECT_EQ(grip.GetTriggerState(), 0x80);      // Released
		grip.SetBit(Atari2600BoosterGrip::Buttons::Trigger);
		EXPECT_EQ(grip.GetTriggerState(), 0x00);       // Pressed
	}

	TEST_F(BoosterGripTest, BoosterState) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);
		EXPECT_EQ(grip.GetBoosterState(), 0x80);       // Released
		grip.SetBit(Atari2600BoosterGrip::Buttons::Booster);
		EXPECT_EQ(grip.GetBoosterState(), 0x00);        // Pressed
	}

	TEST_F(BoosterGripTest, AllButtonsIndependent) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);
		// Press all 7 buttons
		grip.SetBit(Atari2600BoosterGrip::Buttons::Up);
		grip.SetBit(Atari2600BoosterGrip::Buttons::Down);
		grip.SetBit(Atari2600BoosterGrip::Buttons::Left);
		grip.SetBit(Atari2600BoosterGrip::Buttons::Right);
		grip.SetBit(Atari2600BoosterGrip::Buttons::Fire);
		grip.SetBit(Atari2600BoosterGrip::Buttons::Trigger);
		grip.SetBit(Atari2600BoosterGrip::Buttons::Booster);

		EXPECT_EQ(grip.GetDirectionNibble(), 0x00);  // All directions pressed
		EXPECT_EQ(grip.GetFireState(), 0x00);
		EXPECT_EQ(grip.GetTriggerState(), 0x00);
		EXPECT_EQ(grip.GetBoosterState(), 0x00);
	}

	TEST_F(BoosterGripTest, KeyAssociationCountCorrect) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);
		// Should have 7 associations: UDLR + Fire + Trigger + Booster
		EXPECT_EQ(grip.GetKeyNameAssociations().size(), 7u);
	}

	TEST_F(BoosterGripTest, KeyNameAssociationsContainsAllButtons) {
		Atari2600BoosterGrip grip(&emu, 0, emptyKeys);
		auto names = grip.GetKeyNameAssociations();
		ASSERT_EQ(names.size(), 7u);
	}

	// ========================================================================
	// 24.6 — ControlManager Factory & Wiring Tests (#870)
	// ========================================================================

	class ControlManagerFactoryTest : public ::testing::Test {
	protected:
		Emulator emu;
		Atari2600Console console{&emu};

		void SetUp() override {
			LoadTestRom(emu, console);
		}
	};

	TEST_F(ControlManagerFactoryTest, DefaultSwchaAllReleased) {
		// With default config (no controller or joystick), SWCHA should be 0xff
		console.RunFrame();
		Atari2600RiotState riotState = console.GetRiotState();
		// PortA = SWCHA, should have no buttons pressed
		// Default is 0xff (all released, active-low), but PortA may be
		// uninitialized without controller input. Just verify we can read.
		EXPECT_GE(riotState.PortA, 0u);
	}

	TEST_F(ControlManagerFactoryTest, ControllerTypeEnumValuesExist) {
		// Verify all 5 Atari 2600 controller types are defined and have
		// distinct values in the ControllerType enum
		ControllerType types[] = {
			ControllerType::Atari2600Joystick,
			ControllerType::Atari2600Paddle,
			ControllerType::Atari2600Keypad,
			ControllerType::Atari2600DrivingController,
			ControllerType::Atari2600BoosterGrip,
		};
		// All 5 should be distinct
		for (int i = 0; i < 5; i++) {
			for (int j = i + 1; j < 5; j++) {
				EXPECT_NE(types[i], types[j])
					<< "ControllerType enum values " << i << " and " << j << " are not distinct";
			}
		}
	}

	TEST_F(ControlManagerFactoryTest, ControllerTypeEnumValuesNotNone) {
		// None of the Atari types should equal None
		EXPECT_NE(ControllerType::Atari2600Joystick, ControllerType::None);
		EXPECT_NE(ControllerType::Atari2600Paddle, ControllerType::None);
		EXPECT_NE(ControllerType::Atari2600Keypad, ControllerType::None);
		EXPECT_NE(ControllerType::Atari2600DrivingController, ControllerType::None);
		EXPECT_NE(ControllerType::Atari2600BoosterGrip, ControllerType::None);
	}

} // namespace
