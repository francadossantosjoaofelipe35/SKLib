#include "gpuspoof.h"
#include <chrono>
#include <random>
#include <iostream>

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
    DWORD64 gpuDevice{};

    while (true) {
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

bool gpu::Spoof(DWORD64 seed) {
    if (seed == 0) {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
    }

    std::mt19937_64 rnd(seed);

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
    ZydisDecoder decoder;
    ZyanStatus zstatus = ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
    if (!ZYAN_SUCCESS(zstatus)) {
        DbgMsg("[ZYDIS] Failed creating decoder: 0x%x", zstatus);
        return false;
    }

    ZydisDecodedInstruction instruction;
    GpuMgrGetGpuFromId = reinterpret_cast<UINT64(*)(int)>(*(int*)(Addr + 1) + 5 + Addr);

    Addr += AddrOffset;
    Addr += *(int*)(Addr + 1) + 5;

    UINT32 UuidValidOffset = 0;
    for (int InstructionCount = 0; ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(&decoder, (ZyanU8*)Addr + instrLen, PAGE_SIZE - instrLen, &instruction)), InstructionCount < 0x50; InstructionCount++) {
        UINT32 Opcode = *(UINT32*)Addr & 0xFFFFFF;
        if (Opcode == 0x818D4C) {
            UuidValidOffset = *(UINT32*)(Addr + 3) - 1;
            break;
        }

        Addr += instruction.length;
    }

    if (!UuidValidOffset) {
        DbgMsg("[GPU] Failed to find uuid offset");
        return false;
    }

    static UUID* origGUIDs[32] = { nullptr };

    int spoofedGPUs = 0;
    for (int i = 0; i < 32; i++) {
        UINT64 ProbedGPU = GpuMgrGetGpuFromId(i);

        if (!ProbedGPU) continue;

        if (!*(bool*)(ProbedGPU + UuidValidOffset)) continue;

        if (!origGUIDs[i]) {
            origGUIDs[i] = new UUID;
            *origGUIDs[i] = *(UUID*)(ProbedGPU + UuidValidOffset + 1);
        } else {
            *(UUID*)(ProbedGPU + UuidValidOffset + 1) = *origGUIDs[i];
        }

        UUID newUUID;
        newUUID.Data1 = rnd() & 0xFFFFFFFF;
        newUUID.Data2 = rnd() & 0xFFFF;
        newUUID.Data3 = rnd() & 0xFFFF;
        rnd.bytes((char*)newUUID.Data4, 8);

        _disable();
        CPU::DisableWriteProtection();
        *(UUID*)(ProbedGPU + UuidValidOffset + 1) = newUUID;
        CPU::EnableWriteProtection();
        _enable();

        DbgMsg("[GPU] Spoofed GPU %d", i);
        spoofedGPUs++;
    }

    return spoofedGPUs > 0;
}
