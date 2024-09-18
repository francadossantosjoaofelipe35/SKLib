#include "gpuspoof.h"
#include "random"
#include <array>
#include <random>
#include <uuid/uuid.h> // Inclua esta biblioteca para manipulação de UUID

// Função para gerar um UUID aleatório
UUID GenerateRandomUUID() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<uint32_t> distribution(0, 0xFFFFFFFF);

    UUID uuid;
    std::array<uint32_t, 4> parts;

    for (auto &part : parts) {
        part = distribution(generator);
    }

    // Monta o UUID
    uuid.time_low = parts[0];
    uuid.time_mid = parts[1] & 0xFFFF;
    uuid.time_hi_and_version = (parts[2] & 0x0FFF) | 0x4000;
    uuid.clock_seq_hi_and_reserved = ((parts[2] >> 16) & 0x3F) | 0x80;
    uuid.clock_seq_low = parts[3] & 0xFF;

    return uuid;
}

// Função para converter UUID para um array de bytes
void UUIDToBytes(const UUID& uuid, uint8_t* buffer) {
    buffer[0] = uuid.time_low & 0xFF;
    buffer[1] = (uuid.time_low >> 8) & 0xFF;
    buffer[2] = (uuid.time_low >> 16) & 0xFF;
    buffer[3] = (uuid.time_low >> 24) & 0xFF;

    buffer[4] = uuid.time_mid & 0xFF;
    buffer[5] = (uuid.time_mid >> 8) & 0xFF;

    buffer[6] = uuid.time_hi_and_version & 0xFF;
    buffer[7] = (uuid.time_hi_and_version >> 8) & 0xFF;

    buffer[8] = uuid.clock_seq_hi_and_reserved;
    buffer[9] = uuid.clock_seq_low;

    buffer[10] = uuid.node & 0xFF;
    buffer[11] = (uuid.node >> 8) & 0xFF;
    buffer[12] = (uuid.node >> 16) & 0xFF;
    buffer[13] = (uuid.node >> 24) & 0xFF;
    buffer[14] = (uuid.node >> 32) & 0xFF;
    buffer[15] = (uuid.node >> 40) & 0xFF;
}

bool gpu::Spoof(DWORD64 seed) {
    std::mt19937 rnd(seed);

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

        UUID newUuid = GenerateRandomUUID();
        uint8_t uuidBytes[16];
        UUIDToBytes(newUuid, uuidBytes);

        _disable();
        memcpy((void*)(ProbedGPU + UuidValidOffset + 1), uuidBytes, sizeof(UUID));
        _enable();

        DbgMsg("[GPU] Spoofed GPU %d", i);
        spoofedGPUs++;
    }

    return spoofedGPUs > 0;
}
