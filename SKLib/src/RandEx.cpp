#include <random>
#include <cstdio>
#include <cstring>
#include <string>

namespace random {

enum class SecurityLevel {
    PREDICTABLE,
    PSEUDO,
    SECURE
};

class Random {
public:
    Random(SecurityLevel eSecLevl);
    Random(ULONG seed);

    void setSeed(ULONG seed);
    void setSecLevel(SecurityLevel eSecLevel);

    size_t Next(size_t begin, size_t end);
    int Next(int begin, int end);
    size_t NextPredictable(size_t begin, size_t end);
    int NextPredictable(int begin, int end);

    size_t XorPredictable(size_t number);
    int XorPredictable(int number);

    std::string String(size_t sz);
    void bytes(char* pOut, size_t sz);
    void c_str(char* pOut, size_t sz);
    void w_str(wchar_t* pOut, size_t sz);
    void c_str_upper(char* pOut, size_t sz);
    void w_str_upper(wchar_t* pOut, size_t sz);
    void c_str_hex(char* pOut, size_t sz);
    void w_str_hex(wchar_t* pOut, size_t sz);

    void Regenerate(SecurityLevel eSecLevl);

private:
    ULONG getRandom();
    size_t Next(size_t begin, size_t end, ULONG seed);

    SecurityLevel _eSecLevel;
    ULONG _seed;
    ULONG _xorKey;

    static constexpr size_t RND_STRING_MAX_LEN = 256;
    static std::mt19937 _engine;
    static std::uniform_int_distribution<ULONG> _dist;
};

// Static members initialization
std::mt19937 Random::_engine(std::random_device{}());
std::uniform_int_distribution<ULONG> Random::_dist(0, ULONG_MAX);

// Random Number Generation
ULONG Random::getRandom() {
    return _dist(_engine);
}

// Constructors
Random::Random(SecurityLevel eSecLevl)
    : _eSecLevel(eSecLevl), _xorKey(0) {
    if (_eSecLevel == SecurityLevel::PSEUDO) {
        _seed = static_cast<ULONG>(std::chrono::steady_clock::now().time_since_epoch().count());
    } else if (_eSecLevel == SecurityLevel::PREDICTABLE) {
        _seed = 0;
    } else {
        // Secure Random
        _seed = _dist(_engine);
    }
    _xorKey = Next(1ull, ULONG_MAX);
}

Random::Random(ULONG seed)
    : _eSecLevel(SecurityLevel::PREDICTABLE), _seed(seed), _xorKey(Next(1ull, ULONG_MAX)) {}

// Setters
void Random::setSeed(ULONG seed) {
    _eSecLevel = SecurityLevel::PREDICTABLE;
    _seed = seed;
}

void Random::setSecLevel(SecurityLevel eSecLevel) {
    _eSecLevel = eSecLevel;
}

// Random Number Generation within Range
size_t Random::Next(size_t begin, size_t end) {
    size_t ret = getRandom();
    ret <<= 32;
    ret |= getRandom();
    ret %= end;
    if (ret < begin) ret += begin;
    return ret;
}

int Random::Next(int begin, int end) {
    return static_cast<int>(Next(static_cast<size_t>(begin), static_cast<size_t>(end)));
}

// Predictable Random Number Generation
size_t Random::NextPredictable(size_t begin, size_t end) {
    if (end == 0) return 1;
    size_t ret = begin ^ static_cast<size_t>(_AddressOfReturnAddress());
    ret <<= 32;
    ret |= begin ^ _xorKey;
    ret %= end;
    if (ret < begin) ret += begin;
    return ret;
}

int Random::NextPredictable(int begin, int end) {
    return static_cast<int>(NextPredictable(static_cast<size_t>(begin), static_cast<size_t>(end)));
}

// XOR Operation for Predictable Randomness
size_t Random::XorPredictable(size_t number) {
    if (_xorKey == 0) _xorKey = 987654321;
    _xorKey = 8253729 * _xorKey + 2396403;
    return number ^ _xorKey;
}

int Random::XorPredictable(int number) {
    return static_cast<int>(XorPredictable(static_cast<size_t>(number)));
}

// Generate String
std::string Random::String(size_t sz) {
    char pBuf[RND_STRING_MAX_LEN] = { 0 };
    if (sz < RND_STRING_MAX_LEN - 1) {
        c_str(pBuf, sz);
    } else {
        printf("[RANDOM] Warning: attempted to generate %lld characters, current max is %d", sz, RND_STRING_MAX_LEN - 1);
    }
    return pBuf;
}

// Generate Bytes
void Random::bytes(char* pOut, size_t sz) {
    for (size_t n = 0; n < sz; ++n) {
        pOut[n] = static_cast<char>(getRandom() % 256);
    }
}

// Generate Character Strings
void Random::c_str(char* pOut, size_t sz) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = '\0';
}

void Random::w_str(wchar_t* pOut, size_t sz) {
    static const wchar_t charset[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) / sizeof(wchar_t) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = L'\0';
}

void Random::c_str_upper(char* pOut, size_t sz) {
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = '\0';
}

void Random::w_str_upper(wchar_t* pOut, size_t sz) {
    static const wchar_t charset[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) / sizeof(wchar_t) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = L'\0';
}

// Generate Hexadecimal Strings
void Random::c_str_hex(char* pOut, size_t sz) {
    static const char charset[] = "ABCDEF0123456789";
    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = '\0';
}

void Random::w_str_hex(wchar_t* pOut, size_t sz) {
    static const wchar_t charset[] = L"ABCDEF0123456789";
    for (size_t n = 0; n < sz; ++n) {
        int key = getRandom() % (sizeof(charset) / sizeof(wchar_t) - 1);
        pOut[n] = charset[key];
    }
    pOut[sz] = L'\0';
}

// Regenerate Random Instance
void Random::Regenerate(SecurityLevel eSecLevl) {
    Random r(eSecLevl);
    _engine = std::mt19937(std::random_device{}());
    _dist = std::uniform_int_distribution<ULONG>(0, ULONG_MAX);
}

// Global Random Functions
size_t Next(size_t begin, size_t end) {
    return Random::Next(begin, end);
}

size_t NextHardware(size_t begin, size_t end) {
    ULONGLONG seed;
    _rdrand64_step(&seed);
    seed %= end;
    if (seed < begin) seed += begin;
    return seed;
}

int Next32(int begin, int end) {
    return Random::Next(begin, end);
}

std::string String(size_t sz) {
    return Random::String(sz);
}

void c_str(char* pOut, size_t sz) {
    Random::c_str(pOut, sz);
}

void w_str(wchar_t* pOut, size_t sz) {
    Random::w_str(pOut, sz);
}

} // namespace random
