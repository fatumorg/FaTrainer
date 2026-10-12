#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace pak {

inline uint32_t crc32(const void* data, size_t n) {
    static uint32_t table[256] = {};
    if (!table[1])
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
    uint32_t c = ~0u;
    for (size_t i = 0; i < n; i++) c = table[(c ^ ((const uint8_t*)data)[i]) & 0xFF] ^ (c >> 8);
    return ~c;
}

struct Bits {
    const uint8_t* data;
    size_t size, pos = 0;
    uint32_t held = 0;
    int count = 0;
    bool broken = false;
    int take(int n) {
        while (count < n) {
            if (pos >= size) {
                broken = true;
                return 0;
            }
            held |= (uint32_t)data[pos++] << count;
            count += 8;
        }
        int v = (int)(held & ((1u << n) - 1));
        held >>= n, count -= n;
        return v;
    }
};

struct Huffman { uint16_t count[16] = {}, symbol[320] = {}; };

inline void build(Huffman& h, const uint8_t* lengths, int n) {
    memset(h.count, 0, sizeof h.count);
    for (int i = 0; i < n; i++) h.count[lengths[i]]++;
    h.count[0] = 0;
    uint16_t offset[16] = {};
    for (int len = 1; len < 16; len++) offset[len] = offset[len - 1] + h.count[len - 1];
    for (int i = 0; i < n; i++)
        if (lengths[i]) h.symbol[offset[lengths[i]]++] = (uint16_t)i;
}

inline int decode(Bits& b, const Huffman& h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len < 16; len++) {
        code |= b.take(1);
        int count = h.count[len];
        if (code - count < first) return h.symbol[index + (code - first)];
        index += count, first += count, first <<= 1, code <<= 1;
        if (b.broken) return -1;
    }
    return -1;
}

const uint16_t LENGTH_BASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const uint8_t LENGTH_EXTRA[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const uint16_t DISTANCE_BASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const uint8_t DISTANCE_EXTRA[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const uint8_t LENGTH_ORDER[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

inline bool codes(Bits& b, std::string& out, const Huffman& lit, const Huffman& dist) {
    for (;;) {
        int s = decode(b, lit);
        if (s < 0 || b.broken) return false;
        if (s < 256) out += (char)s;
        else if (s == 256) return true;
        else {
            s -= 257;
            if (s >= 29) return false;
            size_t len = LENGTH_BASE[s] + b.take(LENGTH_EXTRA[s]);
            int d = decode(b, dist);
            if (d < 0 || d >= 30) return false;
            size_t back = DISTANCE_BASE[d] + b.take(DISTANCE_EXTRA[d]);
            if (back > out.size() || b.broken) return false;
            for (size_t i = 0, from = out.size() - back; i < len; i++) out += out[from + i];
        }
    }
}

inline bool inflate(const uint8_t* data, size_t size, std::string& out) {
    Bits b{data, size};
    int last;
    do {
        last = b.take(1);
        int type = b.take(2);
        if (b.broken) return false;
        if (type == 0) {
            b.held = 0, b.count = 0;
            if (b.pos + 4 > size) return false;
            uint16_t len = (uint16_t)(data[b.pos] | data[b.pos + 1] << 8), nlen = (uint16_t)(data[b.pos + 2] | data[b.pos + 3] << 8);
            b.pos += 4;
            if ((uint16_t)~nlen != len || b.pos + len > size) return false;
            out.append((const char*)data + b.pos, len);
            b.pos += len;
        } else if (type == 1) {
            uint8_t lengths[320];
            for (int i = 0; i < 288; i++) lengths[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
            for (int i = 0; i < 30; i++) lengths[288 + i] = 5;
            Huffman lit, dist;
            build(lit, lengths, 288);
            build(dist, lengths + 288, 30);
            if (!codes(b, out, lit, dist)) return false;
        } else if (type == 2) {
            int nlit = b.take(5) + 257, ndist = b.take(5) + 1, ncode = b.take(4) + 4;
            if (nlit > 286 || ndist > 30) return false;
            uint8_t lengths[320] = {};
            for (int i = 0; i < ncode; i++) lengths[LENGTH_ORDER[i]] = (uint8_t)b.take(3);
            Huffman lencode;
            build(lencode, lengths, 19);
            int total = nlit + ndist;
            uint8_t all[320] = {};
            for (int i = 0; i < total;) {
                int s = decode(b, lencode);
                if (s < 0 || b.broken) return false;
                if (s < 16) {
                    all[i++] = (uint8_t)s;
                    continue;
                }
                int repeat = s == 16 ? 3 + b.take(2) : s == 17 ? 3 + b.take(3) : 11 + b.take(7);
                uint8_t value = s == 16 && i ? all[i - 1] : 0;
                if ((s == 16 && !i) || i + repeat > total) return false;
                while (repeat--) all[i++] = value;
            }
            Huffman lit, dist;
            build(lit, all, nlit);
            build(dist, all + nlit, ndist);
            if (!codes(b, out, lit, dist)) return false;
        } else {
            return false;
        }
    } while (!last);
    return true;
}

inline uint32_t u32(const std::string& s, size_t at) { return at + 4 <= s.size() ? (uint8_t)s[at] | (uint8_t)s[at + 1] << 8 | (uint8_t)s[at + 2] << 16 | (uint32_t)(uint8_t)s[at + 3] << 24 : 0; }
inline uint16_t u16(const std::string& s, size_t at) { return at + 2 <= s.size() ? (uint16_t)((uint8_t)s[at] | (uint8_t)s[at + 1] << 8) : 0; }

inline bool read_file(const std::string& path, std::string& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? (size_t)n : 0);
    bool ok = n >= 0 && fread(out.data(), 1, out.size(), f) == out.size();
    fclose(f);
    return ok;
}

const uint32_t END_RECORD = 0x06054b50, CENTRAL_RECORD = 0x02014b50, LOCAL_RECORD = 0x04034b50;
const size_t END_SIZE = 22, CENTRAL_SIZE = 46, LOCAL_SIZE = 30;

struct Entry { std::string name; uint16_t method; uint32_t crc, packed, size, local; };

inline std::vector<Entry> entries(const std::string& zip) {
    std::vector<Entry> out;
    if (zip.size() < END_SIZE) return out;
    size_t end = zip.size() - END_SIZE;
    while (end > 0 && u32(zip, end) != END_RECORD && zip.size() - end < END_SIZE + 0x10000) end--;
    if (u32(zip, end) != END_RECORD) return out;
    size_t at = u32(zip, end + 16);
    for (int i = 0, n = u16(zip, end + 10); i < n && u32(zip, at) == CENTRAL_RECORD; i++) {
        uint16_t name = u16(zip, at + 28), extra = u16(zip, at + 30), comment = u16(zip, at + 32);
        if (at + CENTRAL_SIZE + name > zip.size()) break;
        out.push_back({zip.substr(at + CENTRAL_SIZE, name), u16(zip, at + 10), u32(zip, at + 16), u32(zip, at + 20), u32(zip, at + 24), u32(zip, at + 42)});
        at += CENTRAL_SIZE + name + extra + comment;
    }
    return out;
}

inline bool extract(const std::string& zip, const Entry& e, std::string& out) {
    out.clear();
    if (u32(zip, e.local) != LOCAL_RECORD) return false;
    size_t data = e.local + LOCAL_SIZE + u16(zip, e.local + 26) + u16(zip, e.local + 28);
    if (data + e.packed > zip.size()) return false;
    const uint8_t* p = (const uint8_t*)zip.data() + data;
    out.reserve(e.size);
    if (e.method == 0) out.assign((const char*)p, e.packed);
    else if (e.method != 8 || !inflate(p, e.packed, out)) return false;
    return out.size() == e.size && crc32(out.data(), out.size()) == e.crc;
}

}
