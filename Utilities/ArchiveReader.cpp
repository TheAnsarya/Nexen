#include "pch.h"
#include "ArchiveReader.h"
#include <cctype>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <ranges>
#include "FolderUtilities.h"
#include "ZipReader.h"

ArchiveReader::~ArchiveReader() {
	_buffer.reset();
}

bool ArchiveReader::GetStream(const string& filename, std::stringstream& stream) {
	if (_initialized) {
		vector<uint8_t> fileData;
		if (ExtractFile(filename, fileData)) {
			stream.write((char*)fileData.data(), fileData.size());
			return true;
		}
	}
	return false;
}

vector<string> ArchiveReader::GetFileList(std::initializer_list<string> extensions) {
	if (extensions.size() == 0) {
		return InternalGetFileList();
	}

	std::unordered_set<string> extMap(extensions);

	vector<string> filenames;
	for (const string& filename : InternalGetFileList()) {
		string lcFilename = filename;
		std::ranges::transform(lcFilename, lcFilename.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		string ext = FolderUtilities::GetExtension(lcFilename);
		if (extMap.contains(ext)) {
			filenames.push_back(filename);
		}
	}

	return filenames;
}

bool ArchiveReader::CheckFile(const string& filename) {
	vector<string> files = InternalGetFileList();
	return std::ranges::find(files, filename) != files.end();
}

bool ArchiveReader::LoadArchive(std::istream& in) {
	in.seekg(0, std::ios::end);
	std::streampos filesize = in.tellg();
	in.seekg(0, std::ios::beg);

	_buffer = std::make_unique<uint8_t[]>((uint32_t)filesize);
	in.read((char*)_buffer.get(), filesize);
	in.seekg(0, std::ios::beg);
	bool result = LoadArchive(_buffer.get(), (size_t)filesize);
	return result;
}

bool ArchiveReader::LoadArchive(vector<uint8_t>& data) {
	return LoadArchive(data.data(), data.size());
}

bool ArchiveReader::LoadArchive(void* buffer, size_t size) {
	if (InternalLoadArchive(buffer, size)) {
		_initialized = true;
		return true;
	}
	return false;
}

bool ArchiveReader::LoadArchive(const string& filename) {
	ifstream in(filename, std::ios::binary | std::ios::in);
	if (in.good()) {
		LoadArchive(in);
		in.close();
	}
	return false;
}

unique_ptr<ArchiveReader> ArchiveReader::GetReader(std::istream& in) {
	uint8_t header[2] = {0, 0};
	in.read((char*)header, 2);

	unique_ptr<ArchiveReader> reader;
	if (memcmp(header, "PK", 2) == 0) {
		reader = std::make_unique<ZipReader>();
	}

	if (reader) {
		reader->LoadArchive(in);
	}
	return reader;
}

unique_ptr<ArchiveReader> ArchiveReader::GetReader(const string& filepath) {
	ifstream in(filepath, std::ios::in | std::ios::binary);
	if (in) {
		return GetReader(in);
	}
	return nullptr;
}
