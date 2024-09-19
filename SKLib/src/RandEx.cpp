#include "RandEx.h"
#include <string>
#include <windows.h>  // Para KeQueryTimeIncrement e KeQueryInterruptTime

#pragma warning(disable : 4244)  // Suprimir warning de conversão

random::Random random::rnd;

ULONG random::Random::getRandom() {
    if (_eSecLevel == SecurityLevel::PREDICTABLE) {
        if (_seed == 0) {
            _seed = 987654321;  // Seed padrão para previsibilidade
        }
        _seed = 8253729 * _seed + 2396403; // Algoritmo LCG simples
    } else {
        _seed = KeQueryTimeIncrement() ^ (KeQueryInterruptTime() * 8253729);
    }
    return _seed;
}

random::Random::Random(SecurityLevel eSecLevel) : _eSecLevel(eSecLevel), _seed(KeQueryTimeIncrement() ^ (KeQueryInterruptTime() * 8253729)), _xorKey(Next(1ull, MAXULONG64)) {}

random::Random::Random(ULONG seed) : _eSecLevel(SecurityLevel::PREDICTABLE), _seed(seed), _xorKey(Next(1ull, MAXULONG64)) {}

void random::Random::setSeed(ULONG seed) {
    _eSecLevel = SecurityLevel::PREDICTABLE;
    _seed = seed;
}

void random::Random::setSecLevel(SecurityLevel eSecLevel) {
    _eSecLevel = eSecLevel;
}

size_t random::Random::Next(size_t begin, size_t end) {
    size_t ret = getRandom();
    ret <<= 32;
    ret |= getRandom();
    ret %= end;
    if (ret < begin)
        ret += begin;
    return ret;
}

int random::Random::Next(int begin, int end) {
    return static_cast<int>(Next(static_cast<size_t>(begin), static_cast<size_t>(end)));
}

size_t random::Random::NextPredictable(size_t begin, size_t end) {
    if (end == 0) {
        return 1;
    }
    size_t ret = begin ^ static_cast<size_t>(_AddressOfReturnAddress());
    ret <<= 32;
    ret |= begin ^ _xorKey;
    ret %= end;
    if (ret < begin)
        ret += begin;
    return ret;
}

int random::Random::NextPredictable(int begin, int end) {
    return static_cast<int>(NextPredictable(static_cast<size_t>(begin), static_cast<size_t>(end)));
}

size_t random::Random::XorPredictable(size_t number) {
    if (_xorKey == 0) {
        _xorKey = 987654321;
    }
    _xorKey = 8253729 * _xorKey + 2396403;
    return number ^ _xorKey;
}

int random::Random::XorPredictable(int number) {
    return static_cast<int>(XorPredictable(static_cast<size_t>(number)));
}

std::string random::Random::String(size_t sz) {
    char pBuf[RND_STRING_MAX_LEN] = { 0 };

    if (sz < RND_STRING_MAX_LEN - 1) {
        c_str(pBuf, sz);
    } else {
        // Substitua por um log apropriado se necessário
    }

    return std::string(pBuf);
}

void random::Random::bytes(char* pOut, size_t sz) {
    for (size_t n = 0; n < sz; n++) {
        pOut[n] = static_cast<char>(getRandom());
    }
}

void random::Random::c_str(char* pOut, size_t sz) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; n++) {
        int key = static_cast<int>(getRandom()) % (sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = '\0';
}

void random::Random::w_str(wchar_t* pOut, size_t sz) {
    static const wchar_t charset[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; n++) {
        int key = static_cast<int>(getRandom()) % (sizeof(charset) / sizeof(wchar_t) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = L'\0';
}

void random::Regenerate(SecurityLevel eSecLevel) {
    rnd = Random(eSecLevel);
}

size_t random::Next(size_t begin, size_t end) {
    return rnd.Next(begin, end);
}
