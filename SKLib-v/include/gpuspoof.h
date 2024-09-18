#pragma once

#include <Windows.h>
#include <Zydis/Zydis.h>
#include "spoof.h"
#include "Memory.h"
#include "DbgMsg.h"
#include <intrin.h>  // Para _disable() e _enable()

// Definições de constantes para os limites de GPUs
#define NV_MAX_DEVICES 32

// Variáveis globais para offsets e base do driver
extern DWORD64 pGpuSystem;
extern DWORD32 gpuSysOffset;
extern DWORD32 gpuMgrOffset;
extern DWORD32 gpuSysOffset2;
extern DWORD32 bInitOffset;
extern DWORD32 uuidOffset;

// Funções de spoof para GPU
namespace gpu {
    bool Spoof(DWORD64 seed);  // Função que realiza o spoof das GPUs
}

// Funções auxiliares para trabalhar com as GPUs
DWORD64 gpuData(DWORD32 gpuInstance);  // Retorna os dados da GPU
DWORD64 nextGpu(DWORD32 deviceMask, DWORD32* startIndex);  // Itera sobre as GPUs
