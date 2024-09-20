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
	bSuccessful &= nics::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed nics");
		return STATUS_FAILED_NICS_SPOOF;
	}
	bSuccessful &= usb::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed usb");
		return STATUS_FAILED_USB_SPOOF;
	}
	bSuccessful &= gpu::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed gpu");
		return STATUS_FAILED_GPU_SPOOF;
	}
	return STATUS_SUCCESS;
}
