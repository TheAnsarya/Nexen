# Issue #2332 Plan - NotificationManager Benchmark Coverage

## Scope

Add focused benchmark coverage for `NotificationManager::SendNotification` dispatch throughput across representative listener-count and expired-listener scenarios.

## Acceptance Criteria

- Add benchmark source under `Core.Benchmarks/Shared/`.
- Include benchmark in `Core.Benchmarks.vcxproj`.
- Cover at least:
	- Single-listener dispatch
	- Medium listener count dispatch
	- Higher listener count dispatch
	- Expired-listener churn scenario
- Ensure benchmark project builds and benchmark filter runs.
