#include "gpuspoof.h"
#include <random>

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

// Gera um seed aleatório baseado no tempo atual
DWORD64 gpu::generateRandomSeed() {
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

bool gpu::Spoof(DWORD64 seed) {
    if (seed == 0) {
        seed = generateRandomSeed();  // Usa seed baseado no tempo atual se nenhum for fornecido
    }

    rnd.setSecLevel(random::SecurityLevel::PREDICTABLE);
    rnd.setSeed(seed);  // Define o seed para o gerador de números aleatórios

    PVOID pBase = Memory::GetKernelAddress((PCHAR)"nvlddmkm.sys");
    if (!pBase) {
        DbgMsg("[GPU] Failed getting NVIDIA driver object");
        return true; // Pode ocorrer se o PC não tiver uma GPU
    }

    UINT64 Addr = (UINT64)Memory::FindPatternImage(pBase,
        (PCHAR)"\xE8\xCC\xCC\xCC\xCC\x48\x8B\xD8\x48\x85\xC0\x0F\x84\xCC\xCC\xCC\xCC\x44\x8B\x80\xCC\xCC\xCC\xCC\x48\x8D\x15",
        (PCHAR)"x????xxxxxxxx????xxx????xxx");

    if (!Addr) {
        DbgMsg("[GPU] Could not find GpuMgrGetGpuFromId pattern");
        return false;
    }

    UINT64 AddrOffset = 0x3B;
    ZyanUSize instrLen = 0;
    ZydisDecoder* pDecoder = (ZydisDecoder*)cpp::kMalloc(sizeof(*pDecoder), PAGE_READWRITE);
    ZyanStatus zstatus = ZydisDecoderInit(pDecoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
    if (!ZYAN_SUCCESS(zstatus)) {
        DbgMsg("[ZYDIS] Failed creating decoder: 0x%x", zstatus);
        return false;
    }

    GpuMgrGetGpuFromId = decltype(GpuMgrGetGpuFromId)(*(int*)(Addr + 1) + 5 + Addr);

    Addr += AddrOffset;
    Addr += *(int*)(Addr + 1) + 5;

    UINT32 UuidValidOffset = 0;
    for (int InstructionCount = 0; ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(pDecoder, (ZyanU8*)Addr + instrLen, PAGE_SIZE - instrLen, instruction)), InstructionCount < 0x50; InstructionCount++) {
        UINT32 Opcode = *(UINT32*)Addr & 0xFFFFFF;
        if (Opcode == 0x818D4C) {
            UuidValidOffset = *(UINT32*)(Addr + 3) - 1;
            break;
        }
        Addr += instruction->length;
    }

    if (!UuidValidOffset) {
        DbgMsg("[GPU] Failed to find uuid offset");
        return false;
    }

    static UUID* origGUIDs[NV_MAX_DEVICES] = { 0 };
    int spoofedGPUs = 0;

    for (int i = 0; i < NV_MAX_DEVICES; i++) {
        UINT64 ProbedGPU = GpuMgrGetGpuFromId(i);

        if (!ProbedGPU) continue;
        if (!*(bool*)(ProbedGPU + UuidValidOffset)) continue;

        if (!origGUIDs[i]) {
            origGUIDs[i] = (UUID*)cpp::kMalloc(sizeof(UUID));
            *origGUIDs[i] = *(UUID*)(ProbedGPU + UuidValidOffset + 1);
        } else {
            *(UUID*)(ProbedGPU + UuidValidOffset + 1) = *origGUIDs[i];
        }

        rnd.setSeed(seed);
        _disable();
        rnd.bytes((char*)(ProbedGPU + UuidValidOffset + 1), sizeof(UUID));
        _enable();

        DbgMsg("[GPU] Spoofed GPU %d", i);
        spoofedGPUs++;
    }

    return spoofedGPUs > 0;
}
