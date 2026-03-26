#pragma once
#include "pch.h"

/// <summary>
/// Channel F BIOS variant identifier.
///
/// Notes:
/// - SystemI covers the original Fairchild/Luxor BIOS pair SL31253 + SL31254.
/// - SystemII covers SL90025 class BIOS variants (including Luxor dump naming).
/// - Unknown means hash is not recognized in the current database.
/// </summary>
enum class ChannelFBiosVariant {
	Unknown = 0,
	SystemI,
	SystemII,
};

/// <summary>
/// Hash-based BIOS database for Fairchild Channel F variants.
///
/// This helper is intentionally read-only and deterministic:
/// - Canonical hashes are embedded in source.
/// - Lookup is by SHA1 or MD5 (case-insensitive hex).
/// - File names are advisory only; hashes are authoritative.
/// </summary>
class ChannelFBiosDatabase {
public:
	struct Entry {
		const char* Name;
		const char* Sha1;
		const char* Md5;
		ChannelFBiosVariant Variant;
	};

	[[nodiscard]] static const Entry* LookupBySha1(const string& sha1);
	[[nodiscard]] static const Entry* LookupByMd5(const string& md5);
	[[nodiscard]] static ChannelFBiosVariant DetectVariant(const string& sha1, const string& md5);
	[[nodiscard]] static bool IsKnownLuxorSystemII(const string& sha1, const string& md5);
	[[nodiscard]] static uint32_t GetEntryCount();

private:
	static constexpr Entry _database[] = {
		{
			"Channel F BIOS SL31253 (Fairchild)",
			"81193965a374d77b99b4743d317824b53c3e3c78",
			"ac9804d4c0e9d07e33472e3726ed15c3",
			ChannelFBiosVariant::SystemI
		},
		{
			"Channel F BIOS SL31254 (Fairchild)",
			"8f70d1b74483ba3a37e86cf16c849d601a8c3d2c",
			"da98f4bb3242ab80d76629021bb27585",
			ChannelFBiosVariant::SystemI
		},
		{
			"Channel F BIOS SL90025 (Luxor)",
			"759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2",
			"95d339631d867c8f1d15a5f2ec26069d",
			ChannelFBiosVariant::SystemII
		},
		{
			"Channel F BIOS (With Hockey+Tennis)",
			"4b0a38b5af525aa598907683f0dcfeaa90d242e0",
			"495aa78eefd90504a15e20dddcc4943f",
			ChannelFBiosVariant::SystemI
		}
	};
};
