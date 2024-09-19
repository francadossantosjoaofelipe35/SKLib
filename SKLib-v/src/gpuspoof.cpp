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
        return 0;
    }

    gpuSys += gpuSysOffset2;
    DWORD64 gpuDevice = 0;

    while (1) {
        DWORD32 foundInstance = *(DWORD32*)(gpuSys + 0x8);

        if (foundInstance == gpuInstance) {
            DWORD64 device = *(DWORD64*)gpuSys;

            if (device != 0)
                gpuDevice = device;

            break;
        }

        gpuSys += 0x10;
    }

    return gpuDevice;
}

DWORD64 nextGpu(DWORD32 deviceMask, DWORD32* startIndex) {
    if (*startIndex >= NV_MAX_DEVICES) {
        DbgMsg("[GPU] Start index too big: %d", *startIndex);
        return 0;
    }

    for (DWORD32 i = *startIndex; i < NV_MAX_DEVICES; ++i) {
        if (deviceMask & (1U << i)) {
            *startIndex = i + 1;
            return gpuData(i);
        }
    }

    *startIndex = NV_MAX_DEVICES;
    DbgMsg("[GPU] All devices have been handled");
    return 0;
}

UINT64 (*GpuMgrGetGpuFromId)(int gpuId);

// Função auxiliar para obter um valor pseudo-aleatório mais robusto
UINT64 generatePseudoRandom() {
    UINT64 timeValue = __rdtsc();  // Valor baseado no timestamp da CPU
    UINT64 stackAddress = (UINT64)&timeValue;  // Endereço atual da pilha
    UINT64 memoryValue = *(UINT64*)stackAddress;  // Valor na memória naquele endereço

    // Combine diferentes bits para gerar um valor mais único
    return (timeValue ^ (memoryValue << 13) ^ (memoryValue >> 7)) * 31;
}

bool gpu::Spoof(DWORD64 seed) {
    PVOID pBase = Memory::GetKernelAddress((PCHAR)"nvlddmkm.sys");
    if (!pBase) {
        DbgMsg("[GPU] Failed getting NVIDIA driver object");
        return false;
    }

    BOOLEAN status = FALSE;

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

    ZyanUSize instrLen = 0;
    ZydisDecoder* pDecoder = (ZydisDecoder*)cpp::kMalloc(sizeof(*pDecoder), PAGE_READWRITE);
    ZyanStatus zstatus = ZydisDecoderInit(pDecoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
    if (!ZYAN_SUCCESS(zstatus)) {
        DbgMsg("[ZYDIS] Failed creating decoder: 0x%x", zstatus);
        return false;
    }

    const ZyanUSize length = PAGE_SIZE;
    ZydisDecodedInstruction* instruction = (ZydisDecodedInstruction*)cpp::kMalloc(sizeof(*instruction), PAGE_READWRITE);

    GpuMgrGetGpuFromId = decltype(GpuMgrGetGpuFromId)(*(int*)(Addr + 1) + 5 + Addr);

    Addr += AddrOffset;
    Addr += *(int*)(Addr + 1) + 5;

    UINT32 UuidValidOffset = 0;
    for (int InstructionCount = 0; ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(pDecoder, (ZyanU8*)Addr + instrLen, length - instrLen, instruction)), InstructionCount < 0x50; InstructionCount++) {
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

    static UUID* origGUIDs[32] = { 0 };

    int spoofedGPUs = 0;
    for (int i = 0; i < 32; i++) {
        UINT64 ProbedGPU = GpuMgrGetGpuFromId(i);

        if (!ProbedGPU) continue;
        if (!*(bool*)(ProbedGPU + UuidValidOffset)) continue;

        if (!origGUIDs[i]) {
            origGUIDs[i] = (UUID*)cpp::kMalloc(sizeof(UUID));
            *origGUIDs[i] = *(UUID*)(ProbedGPU + UuidValidOffset + 1);
        } else {
            *(UUID*)(ProbedGPU + UuidValidOffset + 1) = *origGUIDs[i];
        }

        // Gerar um UUID pseudo-aleatório
        UINT64 randomValue = generatePseudoRandom();

        // Dividir o valor pseudo-aleatório para definir o UUID
        _disable();
        UUID* pGuid = (UUID*)(ProbedGPU + UuidValidOffset + 1);
        pGuid->Data1 = (randomValue & 0xFFFFFFFF);
        pGuid->Data2 = (randomValue >> 32) & 0xFFFF;
        pGuid->Data3 = (randomValue >> 48) & 0xFFFF;

        // Preencher os 8 bytes de Data4
        for (int j = 0; j < 8; j++) {
            pGuid->Data4[j] = (randomValue >> (j * 8)) & 0xFF;
        }
        _enable();

        DbgMsg("[GPU] Spoofed GPU %d with randomized UUID", i);
        spoofedGPUs++;
    }

    return spoofedGPUs > 0;
}
