#pragma once

#include "pch.h"
#include "Utilities/SimpleLock.h"
#include "Utilities/ISerializable.h"

class BaseControlDevice;
class IInputRecorder;
class IInputProvider;
class Emulator;
enum class CpuType : uint8_t;
struct ControllerData;
enum class ControllerType;

/// <summary>
/// Base controller/input device manager for all consoles.
/// Manages input devices, polling, lag detection, and input recording/playback.
/// </summary>
/// <remarks>
/// Architecture:
/// - Abstract base class (platform-specific subclasses for NES, SNES, GB, etc.)
/// - Owns all connected controllers (_controlDevices)
/// - System devices (_systemDevices): Built-in input (power button, etc.)
/// - Input providers: External input sources (movies, rewind, netplay)
/// - Input recorders: Record input (movies, netplay server)
/// 
/// Controller lifecycle:
/// 1. CreateControllerDevice(type, port) - Factory method (platform-specific)
/// 2. RegisterControlDevice() - Add to managed devices
/// 3. UpdateInputState() - Poll input each frame
/// 4. Serialize/Deserialize - Save state support
/// 5. ClearDevices() - Cleanup on console stop
/// 
/// Input flow:
/// 1. UpdateInputState() called each frame
/// 2. Input providers set input (if active)
/// 3. Devices polled for state
/// 4. Input recorders record state
/// 5. SetInputReadFlag() marks input consumed
/// 
/// Lag detection:
/// - Frame is "lag" if input not polled (_wasInputRead = false)
/// - _lagCounter increments on lag frames
/// - Used for TAS lag frame counting
/// 
/// Polling:
/// - _pollCounter tracks total polls (for TAS frame count)
/// - SetInputReadFlag() increments poll counter
/// - ProcessEndOfFrame() checks lag
/// 
/// Features:
/// - Multi-controller support (up to 8 ports + sub-ports)
/// - Controller hot-swapping
/// - Input recording/playback (movies, netplay)
/// - Save state serialization
/// - Lag frame detection
/// - Device type detection (HasControlDevice)
/// 
/// Thread safety: _deviceLock protects controller list access.
/// </remarks>
class BaseControlManager : public ISerializable {
private:
	vector<IInputRecorder*> _inputRecorders;  ///< Input recorders (movies, netplay)
	vector<IInputProvider*> _inputProviders;  ///< Input providers (movies, rewind)

protected:
	Emulator* _emu = nullptr;     ///< Emulator instance
	CpuType _cpuType = {};        ///< CPU type for this console
	SimpleLock _deviceLock;       ///< Device list lock
	vector<shared_ptr<BaseControlDevice>> _systemDevices;   ///< System devices (power, reset)
	vector<shared_ptr<BaseControlDevice>> _controlDevices;  ///< Controllers
	uint32_t _pollCounter = 0;    ///< Total input polls
	uint32_t _lagCounter = 0;     ///< Lag frame counter
	bool _wasInputRead = false;   ///< Input polled this frame flag

	/// <summary>
	/// Register controller device (add to managed list).
	/// </summary>
	void RegisterControlDevice(shared_ptr<BaseControlDevice> controlDevice);

	/// <summary>Clear all devices (called on console stop)</summary>
	void ClearDevices();

public:
	/// <summary>
	/// Construct control manager for console.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="cpuType">CPU type</param>
	BaseControlManager(Emulator* emu, CpuType cpuType);
	
	/// <summary>Virtual destructor for polymorphic deletion</summary>
	virtual ~BaseControlManager();

	/// <summary>Serialize controller state to save state</summary>
	void Serialize(Serializer& s) override;

	/// <summary>
	/// Add system control device (power button, etc.).
	/// </summary>
	void AddSystemControlDevice(shared_ptr<BaseControlDevice> device);

	/// <summary>
	/// Update connected controller devices (for configuration changes).
	/// Platform-specific override.
	/// </summary>
	virtual void UpdateControlDevices() {}
	
	/// <summary>
	/// Update input state for all devices (poll controllers).
	/// Called once per frame.
	/// </summary>
	virtual void UpdateInputState();

	/// <summary>
	/// Process end of frame (lag detection, poll counter).
	/// </summary>
	void ProcessEndOfFrame();

	/// <summary>Mark input as read this frame (prevents lag frame)</summary>
	void SetInputReadFlag();
	
	/// <summary>Get lag frame counter</summary>
	uint32_t GetLagCounter();
	
	/// <summary>Reset lag frame counter</summary>
	void ResetLagCounter();

	/// <summary>
	/// Check if controller type connected.
	/// </summary>
	/// <param name="type">Controller type to check</param>
	bool HasControlDevice(ControllerType type);
	
	/// <summary>Check if keyboard connected (for on-screen keyboard hint)</summary>
	virtual bool IsKeyboardConnected() { return false; }

	/// <summary>Get total input poll count</summary>
	uint32_t GetPollCounter();
	
	/// <summary>Set input poll count (for save state restore)</summary>
	void SetPollCounter(uint32_t value);

	/// <summary>
	/// Reset controller state.
	/// </summary>
	/// <param name="softReset">Soft reset if true, hard reset if false</param>
	virtual void Reset(bool softReset) {}

	/// <summary>Register input provider (movies, rewind, netplay client)</summary>
	void RegisterInputProvider(IInputProvider* provider);
	
	/// <summary>Unregister input provider</summary>
	void UnregisterInputProvider(IInputProvider* provider);

	/// <summary>Register input recorder (movies, netplay server)</summary>
	void RegisterInputRecorder(IInputRecorder* recorder);
	
	/// <summary>Unregister input recorder</summary>
	void UnregisterInputRecorder(IInputRecorder* recorder);

	/// <summary>
	/// Create controller device for console (factory method).
	/// Platform-specific implementation.
	/// </summary>
	/// <param name="type">Controller type</param>
	/// <param name="port">Port number (0-7)</param>
	/// <returns>Controller device instance</returns>
	virtual shared_ptr<BaseControlDevice> CreateControllerDevice(ControllerType type, uint8_t port) = 0;

	/// <summary>Get controller states for all ports</summary>
	vector<ControllerData> GetPortStates();

	/// <summary>
	/// Get controller device by port.
	/// </summary>
	/// <param name="port">Port number (0-7)</param>
	/// <param name="subPort">Sub-port for multitaps (0-3)</param>
	shared_ptr<BaseControlDevice> GetControlDevice(uint8_t port, uint8_t subPort = 0);
	
	/// <summary>Get controller device by index in device list</summary>
	shared_ptr<BaseControlDevice> GetControlDeviceByIndex(uint8_t index);
	
	/// <summary>Refresh controller hub state (for multitaps)</summary>
	void RefreshHubState();
	
	/// <summary>Get all connected controller devices</summary>
	vector<shared_ptr<BaseControlDevice>> GetControlDevices();

	/// <summary>
	/// Get controller device by type (template).
	/// </summary>
	/// <typeparam name="T">Controller device type</typeparam>
	/// <returns>Device instance or nullptr if not found</returns>
	template <typename T>
	shared_ptr<T> GetControlDevice() {
		auto lock = _deviceLock.AcquireSafe();

		for (shared_ptr<BaseControlDevice>& device : _controlDevices) {
			shared_ptr<T> typedDevice = std::dynamic_pointer_cast<T>(device);
			if (typedDevice) {
				return typedDevice;
			}
		}
		return nullptr;
	}
};
