#include "RandEx.h"

namespace random {

Random rnd;

ULONG Random::getRandom() {
    // Geração de número pseudo-aleatório baseada em uma fórmula mais robusta
    _seed = (_seed * 1664525 + 1013904223) % ULONG_MAX;
    return _seed;
}

Random::Random(SecurityLevel eSecLevel) {
    _eSecLevel = eSecLevel;

    if (_eSecLevel == SecurityLevel::PREDICTABLE) {
        _seed = 987654321;  // Seed fixo para previsibilidade
    } else {
        _seed = (ULONG)(__rdtsc() ^ (ULONG)&_seed);  // Seed inicial baseado em timestamp e endereço de memória
    }

    _xorKey = this->Next(1ull, MAXULONG64);
}

Random::Random(ULONG seed) {
    _eSecLevel = SecurityLevel::PREDICTABLE;
    _seed = seed;
    _xorKey = this->Next(1ull, MAXULONG64);
}

void Random::setSeed(ULONG seed) {
    _seed = seed;
}

void Random::setSecLevel(SecurityLevel eSecLevel) {
    _eSecLevel = eSecLevel;
}

size_t Random::Next(size_t begin, size_t end) {
    size_t ret = getRandom();
    ret <<= 32; 
    ret |= getRandom();
    ret %= end;

    if (ret < begin) {
        ret += begin;
    }

    return ret;
}

int Random::Next(int begin, int end) {
    return static_cast<int>(Next(static_cast<size_t>(begin), static_cast<size_t>(end)));
}

size_t Random::NextPredictable(size_t begin, size_t end) {
    size_t ret = (begin ^ (size_t)_AddressOfReturnAddress());
    ret <<= 32;
    ret |= begin ^ _xorKey;
    ret %= end;

    if (ret < begin) {
        ret += begin;
    }

    return ret;
}

int Random::NextPredictable(int begin, int end) {
    return static_cast<int>(NextPredictable(static_cast<size_t>(begin), static_cast<size_t>(end)));
}

size_t Random::XorPredictable(size_t number) {
    if (_xorKey == 0) {
        _xorKey = 987654321;  // Valor fixo inicial para XOR
    }
    _xorKey = (_xorKey * 1664525 + 1013904223) % ULONG_MAX;  // Atualizando XOR key
    return number ^ _xorKey;
}

int Random::XorPredictable(int number) {
    return static_cast<int>(XorPredictable(static_cast<size_t>(number)));
}

string Random::String(size_t sz) {
    char pBuf[RND_STRING_MAX_LEN] = { 0 };

    if (sz < RND_STRING_MAX_LEN - 1) {
        c_str(pBuf, sz);
    } else {
        DbgMsg("[RANDOM] Warning: attempted to generate %lld characters, current max is %d", sz, RND_STRING_MAX_LEN - 1);
    }

    return pBuf;
}

void Random::bytes(char* pOut, size_t sz) {
    for (size_t n = 0; n < sz; ++n) {
        pOut[n] = static_cast<char>(getRandom());
    }
}

void Random::c_str(char* pOut, size_t sz) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = 0;
}

void Random::w_str(wchar_t* pOut, size_t sz) {
    static const wchar_t charset[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) / sizeof(wchar_t) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = 0;
}

void Random::c_str_upper(char* pOut, size_t sz) {
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = 0;
}

void Random::w_str_upper(wchar_t* pOut, size_t sz) {
    static const wchar_t charset[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) / sizeof(wchar_t) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = 0;
}

void Random::c_str_hex(char* pOut, size_t sz) {
    static const char charset[] = "ABCDEF0123456789";

    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = 0;
}

void Random::w_str_hex(wchar_t* pOut, size_t sz) {
    static const wchar_t charset[] = L"ABCDEF0123456789";

    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) / sizeof(wchar_t) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = 0;
}

void Regenerate(SecurityLevel eSecLevl) {
    Random r(eSecLevl);
    rnd = r;
}

size_t Next(size_t begin, size_t end) {
    return rnd.Next(begin, end);
}

int Next32(int begin, int end) {
    return rnd.Next(begin, end);
}

string String(size_t sz) {
    return rnd.String(sz);
}

void c_str(char* pOut, size_t sz) {
    rnd.c_str(pOut, sz);
}

void w_str(wchar_t* pOut, size_t sz) {
    rnd.w_str(pOut, sz);
}

} // namespace random
