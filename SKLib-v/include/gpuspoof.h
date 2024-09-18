#pragma once

#include "cpp.h"

namespace gpuspoof {
    // Função que realiza o spoof do UUID da GPU utilizando a seed para gerar valores aleatórios
    NTSTATUS SpoofGPU(DWORD64 seed);
}
