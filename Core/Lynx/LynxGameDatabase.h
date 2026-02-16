#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Lynx game identification database.
///
/// Provides CRC32-based lookup for game metadata including title, rotation,
/// EEPROM type, and player count. Used to auto-detect properties for ROMs
/// that lack proper LNX header information (headerless .o/.lyx files) and
/// to verify header data against known-good values.
///
/// Entries are sourced from No-Intro DAT verification and manual testing.
/// The database is embedded as a constexpr array — no external files needed.
/// </summary>
class LynxGameDatabase {
public:
	struct Entry {
		uint32_t PrgCrc32;
		const char* Name;
		LynxRotation Rotation;
		LynxEepromType EepromType;
		uint8_t PlayerCount;
	};

	/// <summary>
	/// Look up a game entry by CRC32 of the PRG ROM data (excluding LNX header).
	/// Returns nullptr if not found.
	/// </summary>
	static const Entry* Lookup(uint32_t prgCrc32);

	/// <summary>
	/// Get the total number of entries in the database.
	/// </summary>
	static uint32_t GetEntryCount();

private:
	static constexpr Entry _database[] = {
		// === Commercial Titles (No-Intro verified) ===
		// Sorted alphabetically by title

		{ 0x5ad1d1f5, "APB - All Points Bulletin", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x79e3b6c8, "Awesome Golf", LynxRotation::Right, LynxEepromType::None, 1 },
		{ 0x8dcbc49b, "Baseball Heroes", LynxRotation::Right, LynxEepromType::None, 2 },
		{ 0x56c33027, "Batman Returns", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x8d15d475, "Basketbrawl", LynxRotation::Right, LynxEepromType::None, 4 },
		{ 0x9e6f7bdd, "BattleWheels", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x2b2fedc4, "Battlezone 2000", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x850dc19d, "Bill & Ted's Excellent Adventure", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xbfe1e00f, "Block Out", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xf84ef526, "Blue Lightning", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x44ea7b47, "Bubble Trouble", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x8b8de924, "California Games", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0xe8d1a22c, "Checkered Flag", LynxRotation::None, LynxEepromType::None, 6 },
		{ 0x1d0dab8a, "Chip's Challenge", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x8bbbca0d, "Crystal Mines II", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x15bbb238, "Cyber Virus", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x5f80a87f, "Desert Strike", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x03d653b0, "Dinolympics", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x0d38e3e0, "Dirty Larry - Renegade Cop", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x3b834027, "Double Dragon", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x7f9b3319, "Dracula the Undead", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x4dfe876d, "Electrocop", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x7a25826c, "European Soccer Challenge", LynxRotation::Right, LynxEepromType::None, 4 },
		{ 0xf5f7f797, "Eye of the Beholder", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x83ed3b73, "Fat Bobby", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xb9d462b2, "Fidelity Ultimate Chess Challenge", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x06ac1a94, "Gauntlet - The Third Encounter", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0x66efc04a, "Gates of Zendocon", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x5a08a3f2, "Gordo 106 - The Mutated Lab Monkey", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xc38e3a76, "Hard Drivin'", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x0f83a5de, "Hockey", LynxRotation::Right, LynxEepromType::None, 2 },
		{ 0xf14f4fb1, "Hydra", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xa41a5c16, "Ishido - The Way of Stones", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x39e3c38b, "Jimmy Connors' Tennis", LynxRotation::Right, LynxEepromType::None, 4 },
		{ 0xbe94aa36, "Joust", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x5c5a4aa4, "Klax", LynxRotation::Right, LynxEepromType::None, 2 },
		{ 0x0214f80d, "Krazy Ace - Miniature Golf", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0xbfe75421, "Kung Food", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xe7e37caa, "Lemmings", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x45ce0898, "Lynx Casino", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0x0fbd3d2f, "Malibu Bikini Volleyball", LynxRotation::Right, LynxEepromType::None, 4 },
		{ 0x4fadd4c2, "Marlboro Go!", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x36bd9b42, "Ms. Pac-Man", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x9abb2c41, "NFL Football", LynxRotation::Right, LynxEepromType::None, 2 },
		{ 0x7a36f2c2, "Ninja Gaiden III", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xabc2c8bf, "Ninja Gaiden", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x1c04b2b1, "Pac-Land", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x13dbcb61, "Paperboy", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x58a3a68d, "Pinball Jam", LynxRotation::Right, LynxEepromType::None, 1 },
		{ 0x53a67955, "Pit-Fighter", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0xec549917, "Power Factor", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x38e57e42, "QIX", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0xf8c53dd5, "Rampage", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0x1d86a0f2, "Rampart", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x01866a79, "Road Blasters", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x00c6c6f8, "RoboSquash", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x6c5c1e5c, "Robotron 2084", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xe8b3b8d9, "Rygar", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x77ad1b78, "S.T.U.N. Runner", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x95c60ee4, "Scrapyard Dog", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x06cfb29b, "Shadow of the Beast", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x6f4b6608, "Shanghai", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xe2bd4f23, "Steel Talons", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x68e583c0, "Super Asteroids & Missile Command", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0xbd3082a8, "Super Off-Road", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0x4c97e35e, "Super Skweek", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x68a78537, "Switchblade II", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0xc2c18d2b, "Todd's Adventures in Slime World", LynxRotation::None, LynxEepromType::None, 8 },
		{ 0x34d83ddd, "Toki", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x1cb23afe, "Tournament Cyberball 2072", LynxRotation::Right, LynxEepromType::None, 4 },
		{ 0x5cc68ec0, "Turbo Sub", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0xa0de9d68, "Viking Child", LynxRotation::None, LynxEepromType::None, 1 },
		{ 0x27cd79f2, "Warbirds", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0x91e0db6f, "World Class Soccer", LynxRotation::Right, LynxEepromType::None, 4 },
		{ 0xb8bc76fb, "Xenophobe", LynxRotation::None, LynxEepromType::None, 4 },
		{ 0xaac432e4, "Xybots", LynxRotation::None, LynxEepromType::None, 2 },
		{ 0xee7c0a5c, "Zarlor Mercenary", LynxRotation::None, LynxEepromType::None, 4 },

		// === Homebrew with EEPROM ===
		{ 0xb0e94717, "Growing Ties", LynxRotation::None, LynxEepromType::Eeprom93c46, 1 },
		{ 0xdc8713ee, "Ynxa", LynxRotation::None, LynxEepromType::Eeprom93c46, 1 },
		{ 0x0fa40782, "Raid on TriCity", LynxRotation::None, LynxEepromType::Eeprom93c46, 1 },
		{ 0x4f2fa617, "Star Blader", LynxRotation::None, LynxEepromType::Eeprom93c46, 1 },
	};
};
