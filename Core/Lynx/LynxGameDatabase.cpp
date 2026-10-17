#include "pch.h"
#include "Lynx/LynxGameDatabase.h"

const LynxGameDatabase::Entry* LynxGameDatabase::Lookup(uint32_t prgCrc32) {
	for (const auto& entry : _database) {
		if (entry.PrgCrc32 == prgCrc32) {
			return &entry;
		}
	}
	return nullptr;
}

uint32_t LynxGameDatabase::GetEntryCount() {
	return static_cast<uint32_t>(sizeof(_database) / sizeof(_database[0]));
}
