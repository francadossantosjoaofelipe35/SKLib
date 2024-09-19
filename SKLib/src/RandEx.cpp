#include "RandEx.h"
#include <windows.h>  // Para KeQueryTimeIncrement e KeQueryInterruptTime

#pragma warning(disable : 4244)  // Suprimir warning de conversão

random::Random random::rnd;

ULONG random::Random::getRandom() {
#ifndef ENABLE_PREDICTABLE_RANDOM
    if (_eSecLevel == SecurityLevel::PREDICTABLE)
        _eSecLevel = SecurityLevel::SECURE;
#endif

    // Se o nível de segurança for PREDICTABLE, usa um algoritmo linear congruente
    if (_eSecLevel == SecurityLevel::PREDICTABLE) {
        if (_seed == 0) {
            _seed = 987654321;  // Seed padrão para previsibilidade
        }
        _seed = 8253729 * _seed + 2396403; // Algoritmo LCG simples
    }
    // Se o nível de segurança for PSEUDO ou SECURE, usa uma entropia baseada no tempo
    else if (_eSecLevel == SecurityLevel::PSEUDO || _eSecLevel == SecurityLevel::SECURE) {
        // Combina o tempo atual com uma multiplicação para misturar bits
        _seed = KeQueryTimeIncrement() ^ (KeQueryInterruptTime() * 8253729);
    }

    return _seed;
}

random::Random::Random(SecurityLevel eSecLevel) {
    _eSecLevel = eSecLevel;

    // Usa um seed pseudo-aleatório baseado no tempo do sistema
    _seed = KeQueryTimeIncrement() ^ (KeQueryInterruptTime() * 8253729);

    _xorKey = this->Next(1ull, MAXULONG64); // Gera uma chave XOR para operações futuras
}

random::Random::Random(ULONG seed) {
    _eSecLevel = SecurityLevel::PREDICTABLE;
    _seed = seed;
    _xorKey = this->Next(1ull, MAXULONG64);
}

void random::Random::setSeed(ULONG seed) {
    _eSecLevel = SecurityLevel::PREDICTABLE;
    _seed = seed;
}

void random::Random::setSecLevel(SecurityLevel eSecLevel) {
    _eSecLevel = eSecLevel;
}

size_t random::Random::Next(size_t begin, size_t end) {
    size_t ret = 0;
    ret = getRandom();
    ret <<= 32;
    ret |= getRandom();
    ret %= end;
    if (ret < begin)
        ret += begin;
    return ret;
}

int random::Random::Next(int begin, int end) {
    return (int)Next((size_t)begin, (size_t)end);
}

size_t random::Random::NextPredictable(size_t begin, size_t end) {
    if (!end) {
        return 1;
    }
    size_t ret = 0;
    ret = begin ^ (size_t)_AddressOfReturnAddress();
    ret <<= 32;
    ret |= begin ^ _xorKey;
    ret %= end;
    if (ret < begin)
        ret += begin;
    return ret;
}

int random::Random::NextPredictable(int begin, int end) {
    return (int)NextPredictable((size_t)begin, (size_t)end);
}

size_t random::Random::XorPredictable(size_t number) {
    if (_xorKey == 0) {
        _xorKey = 987654321;
    }
    _xorKey = 8253729 * _xorKey + 2396403;
    return number ^ _xorKey;
}

int random::Random::XorPredictable(int number) {
    return (int)XorPredictable((size_t)number);
}

std::string random::Random::String(size_t sz) {
    char pBuf[RND_STRING_MAX_LEN] = { 0 };

    if (sz < RND_STRING_MAX_LEN - 1) {
        c_str(pBuf, sz);
    } else {
        // Substitua por um log apropriado se necessário
    }

    return pBuf;
}

void random::Random::bytes(char* pOut, size_t sz) {
    for (size_t n = 0; n < sz; n++) {
        pOut[n] = (char)getRandom();
    }
}

void random::Random::c_str(char* pOut, size_t sz) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; n++) {
        int key = getRandom() % (int)(sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = 0;
}

void random::Random::w_str(wchar_t* pOut, size_t sz) {
    static wchar_t charset[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; n++) {
        int key = getRandom() % (int)((sizeof(charset) / sizeof(wchar_t)) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = L'\0';
}

void random::Regenerate(SecurityLevel eSecLevel) {
    Random r(eSecLevel);
    rnd = r;
}

size_t random::Next(size_t begin, size_t end) {
    return rnd.Next(begin, end);
}
