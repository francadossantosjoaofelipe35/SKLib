#include "spoof.h"

DWORD64 spoofer::seed = 0;

// Função para aplicar spoofing a todos os dispositivos
NTSTATUS spoofer::SpoofAll(DWORD64 _seed)
{
    // Define a seed global
    spoofer::seed = _seed;

    // Flag para verificar se todas as operações foram bem-sucedidas
    bool bSuccessful = true;

    // Aplica o spoofing na GPU
    if (!gpu::Spoof(_seed))
    {
        DbgMsg("[SPOOFER] Failed to spoof GPU");
        bSuccessful = false;
    }

    // Adicione outras funções de spoofing conforme necessário, por exemplo:
    // bSuccessful &= nicspoof::SpoofNIC(_seed);
    // if (!bSuccessful) {
    //     DbgMsg("[SPOOFER] Failed to spoof NIC");
    // }

    // Retorna o status baseado no sucesso das operações de spoofing
    return bSuccessful ? STATUS_SUCCESS : STATUS_FAILED_SPOOF;
}
