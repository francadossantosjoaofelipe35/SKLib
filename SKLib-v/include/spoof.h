#pragma once

#include "cpp.h"
#include "nicspoof.h"
#include "diskspoof.h"
#include "gpuspoof.h"
#include "wmispoof.h"
#include "usbspoof.h"

namespace spoofer {
    // Função para aplicar o spoof em todos os tipos de dispositivos
    NTSTATUS SpoofAll(DWORD64 seed = TEST_SEED) {
        gpu::Spoof(seed);  // Passando a seed para o spoof de GPU
        // Adicione chamadas para outros dispositivos, passando a seed se necessário
        // nic::Spoof(seed);
        // disk::Spoof(seed);
        // usb::Spoof(seed);
        // wmi::Spoof(seed);
        
        return STATUS_SUCCESS;
    }

    // Seed global para spoofing
    extern DWORD64 seed;
}
