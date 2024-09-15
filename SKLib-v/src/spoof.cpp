#include "spoof.h"

DWORD64 spoofer::seed = 0;

NTSTATUS spoofer::SpoofAll(DWORD64 _seed)
{
	bool bSuccessful = true;
	spoofer::seed = _seed;
	bSuccessful &= gpu::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed gpu");
		return STATUS_FAILED_GPU_SPOOF;
	}
	return STATUS_SUCCESS;
	}
	bSuccessful &= disks::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed disk");
		return STATUS_FAILED_DISKS_SPOOF;
	}
	bSuccessful &= smbios::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed smbios");
		return STATUS_FAILED_SMBIOS_SPOOF;
	}
	bSuccessful &= efi::Spoof(_seed);
	if (!bSuccessful) {
		DbgMsg("[SPOOFER] Failed efi");
		return STATUS_FAILED_EFI_SPOOF;
	}
