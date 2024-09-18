#pragma once

#include "cpp.h"
#include "nicspoof.h"
#include "diskspoof.h"
#include "gpuspoof.h"
#include "wmispoof.h"
#include "usbspoof.h"

// Namespace para todas as funções de spoofing
namespace spoofer {
    // Seed global usada para randomizar os valores
    extern DWORD64 seed;

    // Função para aplicar o spoof em todos os dispositivos
    NTSTATUS SpoofAll(DWORD64 customSeed = TEST_SEED) {
        // Se um seed customizado for passado, ele será utilizado
        seed = customSeed;

        // Chamar as funções de spoofing de cada dispositivo, passando a seed
        NTSTATUS status = STATUS_SUCCESS;

        status = nicspoof::SpoofNIC(seed);
        if (!NT_SUCCESS(status)) return status;

        status = diskspoof::SpoofDisk(seed);
        if (!NT_SUCCESS(status)) return status;

        status = gpuspoof::SpoofGPU(seed);  // Passa a seed para randomizar o UUID da GPU
        if (!NT_SUCCESS(status)) return status;

        status = wmispoof::SpoofWMI(seed);
        if (!NT_SUCCESS(status)) return status;

        status = usbspoof::SpoofUSB(seed);
        if (!NT_SUCCESS(status)) return status;

        return status;
    }
    
    // Seed global que pode ser alterada para gerar resultados diferentes
    DWORD64 seed = 0;
}
