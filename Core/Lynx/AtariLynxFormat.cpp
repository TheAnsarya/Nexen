#include "pch.h"
#include "Lynx/AtariLynxFormat.h"
#include "Lynx/LynxGameDatabase.h"
#include "Utilities/CRC32.h"
#include "Shared/MessageManager.h"

bool AtariLynxFormat::IsAtariLynxFormat(const uint8_t* data, size_t size) {
	if (size < AtariLynxFormatConstants::HeaderSize) {
		return false;
	}
	return memcmp(data, AtariLynxFormatConstants::Magic, AtariLynxFormatConstants::MagicSize) == 0;
}

bool AtariLynxFormat::IsLnxFormat(const uint8_t* data, size_t size) {
	if (size < 64) {
		return false;
	}
	return data[0] == 'L' && data[1] == 'Y' && data[2] == 'N' && data[3] == 'X';
}

AtariLynxLoadResult AtariLynxFormat::Load(const uint8_t* data, size_t size) {
	AtariLynxLoadResult result;

	if (!IsAtariLynxFormat(data, size)) {
		result.Error = "Not an .atari-lynx file (bad magic)";
		return result;
	}

	// Parse header
	AtariLynxHeader header;
	memcpy(&header, data, sizeof(header));

	// Validate version — reject unknown major versions
	if (header.VersionMajor > AtariLynxFormatConstants::VersionMajor) {
		result.Error = std::format("Unsupported version {}.{}.{}",
			header.VersionMajor, header.VersionMinor, header.VersionPatch);
		return result;
	}

	// Validate ROM size
	if (header.RomSize == 0 || static_cast<size_t>(AtariLynxFormatConstants::HeaderSize) + header.RomSize > size) {
		result.Error = "Invalid ROM size or truncated file";
		return result;
	}

	// Extract ROM data
	const uint8_t* romStart = data + AtariLynxFormatConstants::HeaderSize;
	result.RomData.assign(romStart, romStart + header.RomSize);

	// Verify CRC32
	uint32_t computedCrc = CRC32::GetCRC(result.RomData.data(), static_cast<std::streamoff>(result.RomData.size()));
	result.RomCrc32 = computedCrc;
	if (header.RomCrc32 != 0 && header.RomCrc32 != computedCrc) {
		MessageManager::Log(std::format("Warning: CRC32 mismatch — file {:08x}, computed {:08x}",
			header.RomCrc32, computedCrc));
	}

	// Parse metadata section if present
	if ((header.Flags & AtariLynxFormatConstants::FlagHasMetadata) && header.MetadataOffset > 0) {
		if (header.MetadataOffset + 8 <= size) {
			const uint8_t* metaStart = data + header.MetadataOffset;
			size_t metaAvailable = size - header.MetadataOffset;

			// Verify metadata magic
			if (memcmp(metaStart, AtariLynxFormatConstants::MetaMagic, 4) == 0) {
				uint32_t sectionSize = ReadU32(metaStart + 4);
				if (sectionSize <= metaAvailable - 8) {
					result.HasMetadata = ReadMetadata(metaStart + 8, sectionSize, result.Metadata);
				}
			}
		}
	}

	result.Success = true;
	return result;
}

std::vector<uint8_t> AtariLynxFormat::Save(const uint8_t* romData, uint32_t romSize, const AtariLynxMetadata& metadata) {
	// Build metadata section first to know its size
	std::vector<uint8_t> metaSection;
	WriteMetadata(metaSection, metadata);

	uint32_t metadataOffset = AtariLynxFormatConstants::HeaderSize + romSize;
	uint32_t totalSize = metadataOffset + 8 + static_cast<uint32_t>(metaSection.size()); // 8 = META magic + section size

	// Build header
	AtariLynxHeader header = {};
	memcpy(header.Magic, AtariLynxFormatConstants::Magic, 8);
	header.VersionMajor = AtariLynxFormatConstants::VersionMajor;
	header.VersionMinor = AtariLynxFormatConstants::VersionMinor;
	header.VersionPatch = AtariLynxFormatConstants::VersionPatch;
	header.Flags = AtariLynxFormatConstants::FlagHasMetadata;
	header.RomSize = romSize;
	header.RomCrc32 = CRC32::GetCRC(const_cast<uint8_t*>(romData), static_cast<std::streamoff>(romSize));
	header.TotalFileSize = totalSize;
	header.MetadataOffset = metadataOffset;
	header.Reserved = 0;

	// Assemble file
	std::vector<uint8_t> output;
	output.reserve(totalSize);

	// Header
	const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
	output.insert(output.end(), headerBytes, headerBytes + sizeof(header));

	// ROM data
	output.insert(output.end(), romData, romData + romSize);

	// Metadata section header
	output.insert(output.end(), AtariLynxFormatConstants::MetaMagic, AtariLynxFormatConstants::MetaMagic + 4);
	WriteU32(output, static_cast<uint32_t>(metaSection.size()));

	// Metadata section data
	output.insert(output.end(), metaSection.begin(), metaSection.end());

	return output;
}

std::vector<uint8_t> AtariLynxFormat::SaveHeaderOnly(const uint8_t* romData, uint32_t romSize) {
	uint32_t totalSize = AtariLynxFormatConstants::HeaderSize + romSize;

	AtariLynxHeader header = {};
	memcpy(header.Magic, AtariLynxFormatConstants::Magic, 8);
	header.VersionMajor = AtariLynxFormatConstants::VersionMajor;
	header.VersionMinor = AtariLynxFormatConstants::VersionMinor;
	header.VersionPatch = AtariLynxFormatConstants::VersionPatch;
	header.Flags = 0; // No metadata
	header.RomSize = romSize;
	header.RomCrc32 = CRC32::GetCRC(const_cast<uint8_t*>(romData), static_cast<std::streamoff>(romSize));
	header.TotalFileSize = totalSize;
	header.MetadataOffset = 0;
	header.Reserved = 0;

	std::vector<uint8_t> output;
	output.reserve(totalSize);

	const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
	output.insert(output.end(), headerBytes, headerBytes + sizeof(header));
	output.insert(output.end(), romData, romData + romSize);

	return output;
}

AtariLynxLoadResult AtariLynxFormat::ConvertFromLnx(const uint8_t* data, size_t size) {
	AtariLynxLoadResult result;

	if (!IsLnxFormat(data, size)) {
		result.Error = "Not an LNX file (bad magic)";
		return result;
	}

	if (size < 64) {
		result.Error = "LNX file too small";
		return result;
	}

	// Parse LNX header
	AtariLynxMetadata& meta = result.Metadata;
	meta.Bank0PageSize = data[4] | (static_cast<uint16_t>(data[5]) << 8);
	meta.Bank1PageSize = data[6] | (static_cast<uint16_t>(data[7]) << 8);

	// Extract cart name (bytes 10-41, 32 chars, null-terminated)
	std::string cartName(reinterpret_cast<const char*>(&data[10]), 32);
	size_t nullPos = cartName.find('\0');
	if (nullPos != std::string::npos) {
		cartName.resize(nullPos);
	}
	meta.CartName = cartName;

	// Extract manufacturer (bytes 42-57, 16 chars, null-terminated)
	std::string manufacturer(reinterpret_cast<const char*>(&data[42]), 16);
	nullPos = manufacturer.find('\0');
	if (nullPos != std::string::npos) {
		manufacturer.resize(nullPos);
	}
	meta.Manufacturer = manufacturer;

	meta.Rotation = data[58];

	// EEPROM (byte 60)
	uint8_t eepromByte = data[60];
	meta.EepromType = eepromByte & 0x0f;
	meta.EepromFlags = (eepromByte >> 6) & 0x03;

	// ROM data is everything after the 64-byte header
	result.RomData.assign(data + 64, data + size);
	result.RomCrc32 = CRC32::GetCRC(result.RomData.data(), static_cast<std::streamoff>(result.RomData.size()));
	result.HasMetadata = true;
	result.Success = true;

	// Enrich metadata from game database
	const LynxGameDatabase::Entry* dbEntry = LynxGameDatabase::Lookup(result.RomCrc32);
	if (dbEntry) {
		if (meta.PlayerCount == 0) {
			meta.PlayerCount = dbEntry->PlayerCount;
		}
	}

	return result;
}

AtariLynxLoadResult AtariLynxFormat::ConvertFromRaw(const uint8_t* data, size_t size) {
	AtariLynxLoadResult result;

	if (size == 0) {
		result.Error = "Empty file";
		return result;
	}

	// Raw ROM data — entire file
	result.RomData.assign(data, data + size);
	result.RomCrc32 = CRC32::GetCRC(result.RomData.data(), static_cast<std::streamoff>(result.RomData.size()));

	// Infer bank sizes from ROM size
	AtariLynxMetadata& meta = result.Metadata;
	meta.Bank0PageSize = static_cast<uint16_t>(size / 256);
	meta.Bank1PageSize = 0;

	// Look up game database for metadata
	const LynxGameDatabase::Entry* dbEntry = LynxGameDatabase::Lookup(result.RomCrc32);
	if (dbEntry) {
		meta.CartName = dbEntry->Name;
		meta.Rotation = static_cast<uint8_t>(dbEntry->Rotation);
		meta.EepromType = static_cast<uint8_t>(dbEntry->EepromType);
		meta.PlayerCount = dbEntry->PlayerCount;
		result.HasMetadata = true;
	} else {
		result.HasMetadata = false;
	}

	result.Success = true;
	return result;
}

std::vector<uint8_t> AtariLynxFormat::ConvertToLnx(const uint8_t* romData, uint32_t romSize, const AtariLynxMetadata& metadata) {
	std::vector<uint8_t> output;
	output.resize(64, 0); // 64-byte LNX header, zero-initialized

	// Magic
	output[0] = 'L';
	output[1] = 'Y';
	output[2] = 'N';
	output[3] = 'X';

	// Bank sizes (LE)
	output[4] = metadata.Bank0PageSize & 0xff;
	output[5] = (metadata.Bank0PageSize >> 8) & 0xff;
	output[6] = metadata.Bank1PageSize & 0xff;
	output[7] = (metadata.Bank1PageSize >> 8) & 0xff;

	// Version
	output[8] = 1;
	output[9] = 0;

	// Cart name (bytes 10-41, 32 chars)
	size_t nameLen = std::min(metadata.CartName.size(), size_t{32});
	memcpy(&output[10], metadata.CartName.data(), nameLen);

	// Manufacturer (bytes 42-57, 16 chars)
	size_t mfgLen = std::min(metadata.Manufacturer.size(), size_t{16});
	memcpy(&output[42], metadata.Manufacturer.data(), mfgLen);

	// Rotation
	output[58] = metadata.Rotation;

	// EEPROM
	output[60] = (metadata.EepromType & 0x0f) | ((metadata.EepromFlags & 0x03) << 6);

	// Append ROM data
	output.insert(output.end(), romData, romData + romSize);

	return output;
}

// ============================================================================
// Private helpers
// ============================================================================

void AtariLynxFormat::WriteMetadata(std::vector<uint8_t>& output, const AtariLynxMetadata& metadata) {
	WriteString(output, metadata.CartName);
	WriteString(output, metadata.Manufacturer);
	output.push_back(metadata.Rotation);
	WriteU16(output, metadata.Bank0PageSize);
	WriteU16(output, metadata.Bank1PageSize);
	output.push_back(metadata.EepromType);
	output.push_back(metadata.EepromFlags);
	output.push_back(metadata.HardwareModel);
	WriteU16(output, metadata.YearOfRelease);
	output.push_back(metadata.PlayerCount);
	output.push_back(metadata.Handedness);
	output.push_back(metadata.ComLynxSupport);
	WriteU16(output, metadata.CountryCode);
	WriteString(output, metadata.Language);
	output.push_back(0); // Custom field count = 0
}

bool AtariLynxFormat::ReadMetadata(const uint8_t* data, size_t size, AtariLynxMetadata& metadata) {
	size_t offset = 0;

	auto ensureBytes = [&](size_t n) -> bool { return offset + n <= size; };

	// CartName
	size_t consumed = ReadString(data + offset, size - offset, metadata.CartName);
	if (consumed == 0 && size - offset < 2) return false;
	offset += consumed;

	// Manufacturer
	consumed = ReadString(data + offset, size - offset, metadata.Manufacturer);
	offset += consumed;

	// Rotation
	if (!ensureBytes(1)) return false;
	metadata.Rotation = data[offset++];

	// Bank0PageSize
	if (!ensureBytes(2)) return false;
	metadata.Bank0PageSize = ReadU16(data + offset);
	offset += 2;

	// Bank1PageSize
	if (!ensureBytes(2)) return false;
	metadata.Bank1PageSize = ReadU16(data + offset);
	offset += 2;

	// EepromType
	if (!ensureBytes(1)) return false;
	metadata.EepromType = data[offset++];

	// EepromFlags
	if (!ensureBytes(1)) return false;
	metadata.EepromFlags = data[offset++];

	// HardwareModel
	if (!ensureBytes(1)) return false;
	metadata.HardwareModel = data[offset++];

	// YearOfRelease
	if (!ensureBytes(2)) return false;
	metadata.YearOfRelease = ReadU16(data + offset);
	offset += 2;

	// PlayerCount
	if (!ensureBytes(1)) return false;
	metadata.PlayerCount = data[offset++];

	// Handedness
	if (!ensureBytes(1)) return false;
	metadata.Handedness = data[offset++];

	// ComLynxSupport
	if (!ensureBytes(1)) return false;
	metadata.ComLynxSupport = data[offset++];

	// CountryCode
	if (!ensureBytes(2)) return false;
	metadata.CountryCode = ReadU16(data + offset);
	offset += 2;

	// Language
	consumed = ReadString(data + offset, size - offset, metadata.Language);
	offset += consumed;

	// Custom fields — skip them
	if (ensureBytes(1)) {
		uint8_t customCount = data[offset++];
		for (uint8_t i = 0; i < customCount && ensureBytes(4); i++) {
			// Skip tag (2) + read size (2)
			offset += 2; // tag
			if (!ensureBytes(2)) break;
			uint16_t fieldSize = ReadU16(data + offset);
			offset += 2;
			offset += fieldSize; // skip data
		}
	}

	return true;
}

void AtariLynxFormat::WriteString(std::vector<uint8_t>& output, const std::string& str) {
	uint16_t len = static_cast<uint16_t>(std::min(str.size(), size_t{0xffff}));
	WriteU16(output, len);
	output.insert(output.end(), str.begin(), str.begin() + len);
}

size_t AtariLynxFormat::ReadString(const uint8_t* data, size_t available, std::string& out) {
	if (available < 2) {
		out.clear();
		return 0;
	}
	uint16_t len = ReadU16(data);
	if (available < 2 + len) {
		out.clear();
		return 2;
	}
	out.assign(reinterpret_cast<const char*>(data + 2), len);
	return 2 + len;
}

void AtariLynxFormat::WriteU16(std::vector<uint8_t>& output, uint16_t value) {
	output.push_back(value & 0xff);
	output.push_back((value >> 8) & 0xff);
}

void AtariLynxFormat::WriteU32(std::vector<uint8_t>& output, uint32_t value) {
	output.push_back(value & 0xff);
	output.push_back((value >> 8) & 0xff);
	output.push_back((value >> 16) & 0xff);
	output.push_back((value >> 24) & 0xff);
}

uint16_t AtariLynxFormat::ReadU16(const uint8_t* data) {
	return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t AtariLynxFormat::ReadU32(const uint8_t* data) {
	return static_cast<uint32_t>(data[0]) |
		(static_cast<uint32_t>(data[1]) << 8) |
		(static_cast<uint32_t>(data[2]) << 16) |
		(static_cast<uint32_t>(data[3]) << 24);
}
