#include "spoof.h"

DWORD64 spoofer::seed = 0;

NTSTATUS spoofer::SpoofAll(DWORD64 _seed)
{
	bool bSuccessful = true;
	spoofer::seed = _seed;
	bSuccessful &= wmi::SpoofMonitor(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed monitors");
		return STATUS_FAILED_MONITOR_SPOOF;
	}
	bSuccessful &= disks::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed disk");
		return STATUS_FAILED_DISKS_SPOOF;
	}
	bSuccessful &= gpu::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed gpu");
		return STATUS_FAILED_GPU_SPOOF;
	}
	return STATUS_SUCCESS;
}
