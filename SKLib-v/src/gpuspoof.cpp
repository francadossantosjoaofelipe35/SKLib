#include "gpuspoof.h"

DWORD64 pGpuSystem = 0;
DWORD32 gpuSysOffset = 0;
DWORD32 gpuMgrOffset = 0;
DWORD32 gpuSysOffset2 = 0;
DWORD32 bInitOffset = 0;
DWORD32 uuidOffset = 0;

DWORD64 gpuData(DWORD32 gpuInstance) {
	DWORD64 gpuSys = *(DWORD64*)(pGpuSystem + gpuSysOffset);
	DWORD32 gpuMgr = *(DWORD32*)(gpuSys + gpuMgrOffset);

	if (!gpuMgr) {
		DbgMsg("[GPU] Failed getting gpuMgr");
		return false;
	}

	gpuSys += gpuSysOffset2;
	DWORD64 gpuDevice{};

	while (1) {
		DWORD32 foundInstance = *(DWORD32*)(gpuSys + 0x8);

		if (foundInstance == gpuInstance)
		{
			DWORD64 device = *(DWORD64*)gpuSys;

			if (device != 0)
				gpuDevice = device;

			break;
		}

		gpuSys += 0x10;
	}

	return gpuDevice;
}

DWORD64 nextGpu(DWORD32 deviceMask, DWORD32* startIndex)
{
	if (*startIndex >= NV_MAX_DEVICES)
	{
		DbgMsg("[GPU] Start index too big: %d", *startIndex);
		return 0;

	}

	for (DWORD32 i = *startIndex; i < NV_MAX_DEVICES; ++i)
	{
		if (deviceMask & (1U << i))
		{
			*startIndex = i + 1;
			return gpuData(i);
		}
	}

	*startIndex = NV_MAX_DEVICES;

	DbgMsg("[GPU] All devices have been handled");
	return 0;
}

UINT64 (*GpuMgrGetGpuFromId)(int gpuId);

bool gpu::Spoof(DWORD64 seed)
{
    rnd.setSecLevel(random::SecurityLevel::PREDICTABLE);
    rnd.setSeed(seed);

    PVOID pBase = Memory::GetKernelAddress((PCHAR)"nvlddmkm.sys");
    if (!pBase) {
        DbgMsg("[GPU] Failed getting NVIDIA driver object");
        return true;
    }

    UINT64 Addr = (UINT64)Memory::FindPatternImage(pBase,
        (PCHAR)"\xE8\xCC\xCC\xCC\xCC\x48\x8B\xD8\x48\x85\xC0\x0F\x84\xCC\xCC\xCC\xCC\x44\x8B\x80\xCC\xCC\xCC\xCC\x48\x8D\x15",
        (PCHAR)"x????xxxxxxxx????xxx????xxx");

    UINT64 AddrOffset = 0x3B;
    if (!Addr || *(UINT8*)(Addr + AddrOffset) != 0xE8) {
        AddrOffset++;
        if (*(UINT8*)(Addr + AddrOffset) != 0xE8) {
            DbgMsg("[GPU] Could not find GpuMgrGetGpuFromId pattern");
            return false;
        }
    }

    GpuMgrGetGpuFromId = decltype(GpuMgrGetGpuFromId)(*(int*)(Addr + 1) + 5 + Addr);
    Addr += AddrOffset;
    Addr += *(int*)(Addr + 1) + 5;

    UINT32 UuidValidOffset = 0;
    for (int InstructionCount = 0; InstructionCount < 0x50; InstructionCount++) {
        UINT32 Opcode = *(UINT32*)Addr & 0xFFFFFF;
        if (Opcode == 0x818D4C) {
            UuidValidOffset = *(UINT32*)(Addr + 3) - 1;
            break;
        }
        Addr += sizeof(UINT32);
    }

    if (!UuidValidOffset) {
        DbgMsg("[GPU] Failed to find uuid offset");
        return false;
    }

    static UUID* origGUIDs[32] = { 0 };
    int spoofedGPUs = 0;

    for (int i = 0; i < 32; i++) {
        UINT64 ProbedGPU = GpuMgrGetGpuFromId(i);

        if (!ProbedGPU) continue;
        if (!*(bool*)(ProbedGPU + UuidValidOffset)) continue;

        UUID* uuid = (UUID*)(ProbedGPU + UuidValidOffset + 1);

        if (!origGUIDs[i]) {
            origGUIDs[i] = (UUID*)cpp::kMalloc(sizeof(UUID));
            *origGUIDs[i] = *uuid;
        } else {
            *uuid = *origGUIDs[i];
        }

        // Randomize the UUID
        _disable();
        rnd.setSeed(seed);

        uuid->Data1 = rnd.Next(0, MAXULONG); // Randomize Data1 (4 bytes)
        uuid->Data2 = (UINT16)rnd.Next(0, MAXUSHORT); // Randomize Data2 (2 bytes)
        uuid->Data3 = (UINT16)rnd.Next(0, MAXUSHORT); // Randomize Data3 (2 bytes)
        rnd.bytes((char*)uuid->Data4, 8); // Randomize Data4 (8 bytes)

        _enable();

        DbgMsg("[GPU] Spoofed GPU %d UUID", i);
        spoofedGPUs++;
    }

    return spoofedGPUs > 0;
}
