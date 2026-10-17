#include "pch.h"
#include <string.h>
#include <sstream>
#include "ZipReader.h"

ZipReader::ZipReader() {
	memset(&_zipArchive, 0, sizeof(mz_zip_archive));
}

ZipReader::~ZipReader() {
	if (_initialized) {
		mz_zip_reader_end(&_zipArchive);
	}
}

bool ZipReader::InternalLoadArchive(void* buffer, size_t size) {
	if (_initialized) {
		mz_zip_reader_end(&_zipArchive);
		memset(&_zipArchive, 0, sizeof(mz_zip_archive));
		_initialized = false;
	}

	return mz_zip_reader_init_mem(&_zipArchive, buffer, size, 0) != 0;
}

vector<string> ZipReader::InternalGetFileList() {
	vector<string> fileList;
	if (_initialized) {
		for (int i = 0, len = (int)mz_zip_reader_get_num_files(&_zipArchive); i < len; i++) {
			mz_zip_archive_file_stat file_stat;
			if (!mz_zip_reader_file_stat(&_zipArchive, i, &file_stat)) {
				std::cout << "mz_zip_reader_file_stat() failed!" << std::endl;
			}

			fileList.push_back(file_stat.m_filename);
		}
	}
	return fileList;
}

bool ZipReader::ExtractFile(const string& filename, vector<uint8_t>& output) {
	if (_initialized) {
		int fileIndex = mz_zip_reader_locate_file(&_zipArchive, filename.c_str(), nullptr, 0);
		if (fileIndex < 0) {
#ifdef _DEBUG
			std::cout << "mz_zip_reader_locate_file() failed!" << std::endl;
#endif
			return false;
		}

		mz_zip_archive_file_stat fileStat;
		if (!mz_zip_reader_file_stat(&_zipArchive, fileIndex, &fileStat)) {
#ifdef _DEBUG
			std::cout << "mz_zip_reader_file_stat() failed!" << std::endl;
#endif
			return false;
		}

		output.resize(static_cast<size_t>(fileStat.m_uncomp_size));

		if (!mz_zip_reader_extract_to_mem(&_zipArchive, fileIndex, output.data(), output.size(), 0)) {
#ifdef _DEBUG
			std::cout << "mz_zip_reader_extract_to_mem() failed!" << std::endl;
#endif
			output.clear();
			return false;
		}

		return true;
	}

	return false;
}
