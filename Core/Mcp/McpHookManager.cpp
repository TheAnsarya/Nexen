#include "pch.h"
#include "Mcp/McpHookManager.h"

McpHookManager::McpHookManager()
{
}

size_t McpHookManager::GetQueuedEventCount()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _events.size();
}

size_t McpHookManager::GetDroppedEventCount()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _droppedEvents;
}

int32_t McpHookManager::RegisterHook(McpHookKind kind, CpuType cpu, uint32_t startAddr, uint32_t endAddr,
	uint32_t matchValue, uint32_t matchValueMask, uint32_t xValue, uint32_t xMask)
{
	std::lock_guard<std::mutex> lock(_mutex);
	int32_t handle = _nextHandle.fetch_add(1);
	McpHook h;
	h.Handle = handle;
	h.Kind = kind;
	h.Cpu = cpu;
	h.StartAddr = startAddr;
	h.EndAddr = endAddr;
	h.MatchValue = matchValue;
	h.MatchValueMask = matchValueMask;
	h.XValue = xValue;
	h.XMask = xMask;
	h.Active = true;
	h.ValueMatchEnabled = (matchValueMask != 0);
	_hooks.push_back(h);
	UpdateHasAnyFlag();
	return handle;
}

bool McpHookManager::UnregisterHook(int32_t handle)
{
	std::lock_guard<std::mutex> lock(_mutex);
	for(auto it = _hooks.begin(); it != _hooks.end(); ++it) {
		if(it->Handle == handle) {
			_hooks.erase(it);
			UpdateHasAnyFlag();
			return true;
		}
	}
	return false;
}

size_t McpHookManager::CopyActiveHooks(McpHook* out, size_t maxCount)
{
	std::lock_guard<std::mutex> lock(_mutex);
	size_t n = std::min(_hooks.size(), maxCount);
	for(size_t i = 0; i < n; i++) {
		out[i] = _hooks[i];
	}
	return n;
}

void McpHookManager::OnMemoryOperation(CpuType cpu, uint32_t addr, uint32_t value,
	McpHookKind kind, uint32_t frameNumber, uint32_t xValue, const McpCpuSnapshot& snapshot)
{
	_callCount.fetch_add(1, std::memory_order_relaxed);

	// Double-checked fast path: HasAnyHooks was true when we entered the
	// caller, but another thread may have cleared all hooks since. The
	// mutex-guarded loop below handles the empty case harmlessly.
	std::lock_guard<std::mutex> lock(_mutex);
	if(_hooks.empty()) {
		return;
	}

	for(const McpHook& h : _hooks) {
		if(!h.Active) continue;
		if(h.Kind != kind) continue;
		if(h.Cpu != cpu) continue;
		if(addr < h.StartAddr || addr > h.EndAddr) continue;
		if(h.ValueMatchEnabled) {
			if((value & h.MatchValueMask) != (h.MatchValue & h.MatchValueMask)) continue;
		}
		if(h.XMask != 0 && (xValue & h.XMask) != (h.XValue & h.XMask)) continue;
		_matchCount.fetch_add(1, std::memory_order_relaxed);

		// Drop oldest if queue full. Slow-client backpressure belongs on
		// the client, not on the emulator thread.
		while(_events.size() >= MaxQueuedEvents) {
			_events.pop_front();
			_droppedEvents++;
		}

		McpHookEvent evt;
		evt.Handle = h.Handle;
		evt.Address = addr;
		evt.Value = value;
		evt.FrameNumber = frameNumber;
		evt.Kind = (uint8_t)kind;
		evt.Cpu = (uint8_t)cpu;
		evt.Padding[0] = 0;
		evt.Padding[1] = 0;
		evt.Snapshot = snapshot;
		_events.push_back(evt);
	}
}

size_t McpHookManager::DrainEvents(McpHookEvent* out, size_t maxCount)
{
	std::lock_guard<std::mutex> lock(_mutex);
	size_t n = std::min(_events.size(), maxCount);
	for(size_t i = 0; i < n; i++) {
		out[i] = _events.front();
		_events.pop_front();
	}
	return n;
}

void McpHookManager::Reset()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_hooks.clear();
	_events.clear();
	_droppedEvents = 0;
	_callCount.store(0, std::memory_order_relaxed);
	_matchCount.store(0, std::memory_order_relaxed);
	UpdateHasAnyFlag();
}

void McpHookManager::UpdateHasAnyFlag()
{
	// Caller must hold _mutex.
	_hasAny.store(!_hooks.empty(), std::memory_order_relaxed);
}
