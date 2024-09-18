bool gpu::Spoof(DWORD64 seed)
{
    rnd.setSecLevel(random::SecurityLevel::PREDICTABLE);
    rnd.setSeed(seed);

    static UUID* origGUIDs[32] = { 0 };

    // Max number of GPUs supported is 32.
    int spoofedGPUs = 0;
    for (int i = 0; i < 32; i++)
    {
        UINT64 ProbedGPU = GpuMgrGetGpuFromId(i);

        // Does not exist?
        if (!ProbedGPU) continue;

        // Is GPU UUID not initialized?
        if (!*(bool*)(ProbedGPU + UuidValidOffset)) continue;

        if (!origGUIDs[i]) {
            origGUIDs[i] = (UUID*)cpp::kMalloc(sizeof(UUID));
            *origGUIDs[i] = *(UUID*)(ProbedGPU + UuidValidOffset + 1);
        }
        else {
            *(UUID*)(ProbedGPU + UuidValidOffset + 1) = *origGUIDs[i];
        }

        // Generate the random UUID using rnd
        UUID* uuid = (UUID*)(ProbedGPU + UuidValidOffset + 1);

        uuid->Data1 = (DWORD32)rnd.Next(0, 0xFFFFFFFF); // 8 hexadecimal digits
        uuid->Data2 = (UINT16)rnd.Next(0, 0xFFFF);      // 4 hexadecimal digits
        uuid->Data3 = (UINT16)rnd.Next(0, 0xFFFF);      // 4 hexadecimal digits

        // For Data4 (2 groups: 4 hex and 12 hex digits), randomize it directly as bytes
        rnd.bytes((char*)uuid->Data4, sizeof(uuid->Data4));

        DbgMsg("[GPU] Spoofed GPU %d, New UUID: GPU-%08x-%04x-%04x-%04x%012llx", 
                i, uuid->Data1, uuid->Data2, uuid->Data3, 
                *(UINT16*)uuid->Data4, *(UINT64*)(uuid->Data4 + 2));

        spoofedGPUs++;
    }

    return spoofedGPUs > 0;
}
