#pragma once

#include "cpp.h"
#include "gpuspoof.h"
#include "wmispoof.h"
#include "diskspoof.h"
#include "usbspoof.h"

// Namespace para as funções de spoofing
namespace spoofer {
    // Valor padrão para a semente de aleatorização
    constexpr DWORD64 DEFAULT_SEED = 0x123456789ABCDEF0;

    // Função para realizar o spoofing e aleatorização de UUIDs
    NTSTATUS SpoofAll(DWORD64 seed = DEFAULT_SEED);

    // Variável global para armazenar a semente usada para aleatorização
    extern DWORD64 seed;
}
