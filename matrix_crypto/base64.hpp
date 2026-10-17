// base64.hpp — стандартный Base64 (RFC 4648) без внешних зависимостей.
// Совместим с Python base64.b64encode/b64decode (алфавит +/, padding '=').
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {

inline std::string b64_encode(const uint8_t *data, size_t n)
{
    static const char TBL[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((n + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 3 <= n; i += 3)
    {
        uint32_t v = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8) | uint32_t(data[i + 2]);
        out.push_back(TBL[(v >> 18) & 63]);
        out.push_back(TBL[(v >> 12) & 63]);
        out.push_back(TBL[(v >> 6) & 63]);
        out.push_back(TBL[v & 63]);
    }
    if (i + 1 == n)
    {
        uint32_t v = uint32_t(data[i]) << 16;
        out.push_back(TBL[(v >> 18) & 63]);
        out.push_back(TBL[(v >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    }
    else if (i + 2 == n)
    {
        uint32_t v = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8);
        out.push_back(TBL[(v >> 18) & 63]);
        out.push_back(TBL[(v >> 12) & 63]);
        out.push_back(TBL[(v >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

inline std::string b64_encode(const std::vector<uint8_t> &v)
{
    return b64_encode(v.data(), v.size());
}

inline std::vector<uint8_t> b64_decode(const std::string &s)
{
    auto val = [](char c) -> int
    {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::vector<uint8_t> out;
    out.reserve(s.size() / 4 * 3 + 3);
    uint32_t buf = 0;
    int bits = 0;
    for (char c : s)
    {
        if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
        int v = val(c);
        if (v < 0)
            throw std::runtime_error("Некорректная base64-строка");
        buf = (buf << 6) | uint32_t(v);
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(uint8_t((buf >> bits) & 0xFF));
        }
    }
    return out;
}

} // namespace mc
