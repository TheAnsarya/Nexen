#pragma once

#include "pch.h"

using std::wstring;

/// <summary>IP protocol type for NAT port mapping</summary>
enum class IPProtocol {
	TCP = 0, ///< Transmission Control Protocol
	UDP = 1  ///< User Datagram Protocol
};

/// <summary>
/// UPnP (Universal Plug and Play) NAT port mapping utility.
/// Automatically configures router port forwarding for network play.
/// </summary>
/// <remarks>
/// UPnP allows applications to request port forwarding without manual router configuration.
/// Used for: Netplay, remote debugging, multiplayer gaming.
///
/// Workflow:
/// 1. Detect local network IP addresses
/// 2. Discover UPnP-enabled routers via SSDP
/// 3. Request port mapping via UPnP Internet Gateway Device (IGD) protocol
///
/// Requirements:
/// - Router must support UPnP IGD
/// - UPnP must be enabled in router settings
/// - Local network must have functional gateway
///
/// Platform: Windows only (uses COM/IUPnPNAT)
/// Security note: UPnP can be security risk - some routers have vulnerable implementations.
/// </remarks>
class UPnPPortMapper {
private:
	/// <summary>
	/// Get list of local IP addresses on all network interfaces.
	/// </summary>
	/// <returns>Vector of IP addresses as wide strings</returns>
	/// <remarks>
	/// Enumerates all network adapters and extracts IPv4 addresses.
	/// Excludes loopback (127.0.0.1) and link-local addresses.
	/// </remarks>
	static vector<wstring> GetLocalIPs();

public:
	/// <summary>
	/// Add NAT port mapping on router.
	/// </summary>
	/// <param name="internalPort">Local port on this machine</param>
	/// <param name="externalPort">External port on router (visible to internet)</param>
	/// <param name="protocol">TCP or UDP protocol</param>
	/// <returns>True if mapping added successfully</returns>
	/// <remarks>
	/// Creates persistent port forwarding rule:
	/// Internet → Router:externalPort → LocalIP:internalPort
	///
	/// Mapping persists until RemoveNATPortMapping() called or router rebooted.
	/// Multiple mappings can exist for same internal port with different protocols.
	/// </remarks>
	static bool AddNATPortMapping(uint16_t internalPort, uint16_t externalPort, IPProtocol protocol);

	/// <summary>
	/// Remove NAT port mapping from router.
	/// </summary>
	/// <param name="externalPort">External port to unmap</param>
	/// <param name="protocol">TCP or UDP protocol</param>
	/// <returns>True if mapping removed successfully</returns>
	/// <remarks>
	/// Removes port forwarding rule created by AddNATPortMapping().
	/// Always call this on shutdown to clean up port mappings.
	/// </remarks>
	static bool RemoveNATPortMapping(uint16_t externalPort, IPProtocol protocol);
};