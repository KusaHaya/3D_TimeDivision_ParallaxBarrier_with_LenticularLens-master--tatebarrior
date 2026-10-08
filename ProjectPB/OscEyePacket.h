#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>

namespace eyeosc {
// OSC文字列は終端を含めて4バイト境界までパディングされる。
inline bool ReadString(const unsigned char* data, size_t size, size_t& offset, std::string& value) {
    if (offset >= size) return false;
    const size_t begin = offset;
    while (offset < size && data[offset]) ++offset;
    if (offset == size) return false;
    value.assign(reinterpret_cast<const char*>(data + begin), offset - begin);
    offset = (offset + 4) & ~size_t(3);
    return offset <= size;
}
inline uint32_t ReadU32(const unsigned char* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
template<class Sink>
bool Parse(const unsigned char* data, size_t size, Sink&& sink, unsigned nesting = 0) {
    if (nesting > 8) return false;
    if (size >= 8 && std::memcmp(data, "#bundle\0", 8) == 0) {
        if (size < 16) return false;
        size_t offset = 16;
        while (offset < size) {
            if (size - offset < 4) return false;
            const uint32_t length = ReadU32(data + offset); offset += 4;
            if (!length || length > size - offset) return false;
            if (!Parse(data + offset, length, sink, nesting + 1)) return false;
            offset += length;
        }
        return true;
    }
    size_t offset = 0;
    std::string address, tags;
    if (!ReadString(data, size, offset, address) || !ReadString(data, size, offset, tags)) return false;
    if (tags != ",fff" || size - offset != 12) return false;
    std::array<float, 3> xyz;
    for (int k = 0; k < 3; ++k) {
        const uint32_t bits = ReadU32(data + offset + 4 * k);
        std::memcpy(&xyz[k], &bits, sizeof(float));
        if (!std::isfinite(xyz[k])) return false;
    }
    sink(address, xyz);
    return true;
}
}
