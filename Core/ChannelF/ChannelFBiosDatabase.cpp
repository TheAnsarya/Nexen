#include "pch.h"
#include "ChannelF/ChannelFBiosDatabase.h"

namespace {
	string ToLowerHex(const string& value) {
		string normalized = value;
		std::transform(normalized.begin(), normalized.end(), normalized.begin(),
			[](unsigned char ch) { return (char)std::tolower(ch); });
		return normalized;
	}
}

const ChannelFBiosDatabase::Entry* ChannelFBiosDatabase::LookupBySha1(const string& sha1) {
	string normalized = ToLowerHex(sha1);
	for(const Entry& entry : _database) {
		if(normalized == entry.Sha1) {
			return &entry;
		}
	}
	return nullptr;
}

const ChannelFBiosDatabase::Entry* ChannelFBiosDatabase::LookupByMd5(const string& md5) {
	string normalized = ToLowerHex(md5);
	for(const Entry& entry : _database) {
		if(normalized == entry.Md5) {
			return &entry;
		}
	}
	return nullptr;
}

ChannelFBiosVariant ChannelFBiosDatabase::DetectVariant(const string& sha1, const string& md5) {
	const Entry* sha1Entry = LookupBySha1(sha1);
	if(sha1Entry) {
		return sha1Entry->Variant;
	}

	const Entry* md5Entry = LookupByMd5(md5);
	if(md5Entry) {
		return md5Entry->Variant;
	}

	return ChannelFBiosVariant::Unknown;
}

bool ChannelFBiosDatabase::IsKnownLuxorSystemII(const string& sha1, const string& md5) {
	const string luxorSha1 = "759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2";
	const string luxorMd5 = "95d339631d867c8f1d15a5f2ec26069d";
	string normalizedSha1 = ToLowerHex(sha1);
	string normalizedMd5 = ToLowerHex(md5);
	return normalizedSha1 == luxorSha1 || normalizedMd5 == luxorMd5;
}

uint32_t ChannelFBiosDatabase::GetEntryCount() {
	return (uint32_t)std::size(_database);
}
