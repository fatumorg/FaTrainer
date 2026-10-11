#pragma once
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "../src/version.h"

namespace logic {

inline std::string sha256_hex(const std::string& data) {
    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be,
        0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa,
        0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85,
        0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
        0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::string m = data;
    uint64_t bits = (uint64_t)data.size() * 8;
    m.push_back((char)0x80);
    while (m.size() % 64 != 56) m.push_back(0);
    for (int i = 7; i >= 0; i--) m.push_back((char)(bits >> (i * 8)));
    auto rotr = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
    for (size_t block = 0; block < m.size(); block += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            const unsigned char* p = (const unsigned char*)m.data() + block + i * 4;
            w[i] = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], k = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = k + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
            uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            k = g, g = f, f = e, e = d + t1, d = c, c = b, b = a, a = t1 + t2;
        }
        h[0] += a, h[1] += b, h[2] += c, h[3] += d, h[4] += e, h[5] += f, h[6] += g, h[7] += k;
    }
    static const char* HEX = "0123456789abcdef";
    std::string out;
    for (uint32_t v : h)
        for (int i = 28; i >= 0; i -= 4) out.push_back(HEX[(v >> i) & 0xf]);
    return out;
}

inline std::vector<std::string> vdf_values(const std::string& text, const std::string& key) {
    std::vector<std::string> out;
    const std::string quoted = "\"" + key + "\"";
    for (size_t at = text.find(quoted); at != std::string::npos; at = text.find(quoted, at + 1)) {
        size_t open = at + quoted.size();
        while (open < text.size() && (text[open] == ' ' || text[open] == '\t')) open++;
        if (open >= text.size() || text[open] != '"') continue;
        std::string value;
        size_t i = open + 1;
        for (; i < text.size() && text[i] != '"'; i++) {
            if (text[i] == '\\' && i + 1 < text.size()) i++;
            value.push_back(text[i]);
        }
        if (i < text.size()) out.push_back(value);
    }
    return out;
}

const std::string LEGACY_VERSION = "1.x";

inline std::string trainer_version_in(const std::string& dll) {
    const std::string TAG = "FaTrainer-version:";
    size_t at = dll.find(TAG);
    if (at != std::string::npos) {
        size_t start = at + TAG.size(), end = start;
        while (end < dll.size() && (isdigit((unsigned char)dll[end]) || dll[end] == '.')) end++;
        if (end + 2 < dll.size() && dll.compare(end, 2, " b") == 0 && isdigit((unsigned char)dll[end + 2]))
            for (end += 2; end < dll.size() && isdigit((unsigned char)dll[end]);) end++;
        if (end > start) return dll.substr(start, end - start);
    }
    return dll.find("FaTrainer | Dying Light") != std::string::npos ? LEGACY_VERSION : "";
}

struct ChangelogEntry { std::string version, name; std::vector<std::string> lines; };

inline std::vector<ChangelogEntry> parse_changelog(const std::string& md) {
    std::vector<ChangelogEntry> out;
    size_t at = 0;
    while (at < md.size()) {
        size_t end = md.find('\n', at);
        std::string line = md.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? md.size() : end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.rfind("## ", 0) == 0) out.push_back({line.substr(3), "", {}});
        else if (!line.empty() && line[0] != '-' && line[0] != ' ' && !out.empty() && out.back().lines.empty() && out.back().name.empty()) out.back().name = line;
        else if (line.rfind("- ", 0) == 0 && !out.empty()) out.back().lines.push_back(line.substr(2));
        else if (!line.empty() && !out.empty() && !out.back().lines.empty() && line[0] == ' ') out.back().lines.back() += " " + line.substr(line.find_first_not_of(' '));
    }
    return out;
}

inline bool needs_install(const std::string& installed, const std::string& latest, bool ready, bool beta) {
    if (!ready || installed == latest) return false;
    return !(beta && newer_version(installed, latest));
}

inline bool write_launch_file(const std::filesystem::path& game, long long now) {
    std::ofstream out(game / LAUNCH_FILE, std::ios::trunc);
    out << now;
    out.close();
    return !out.fail();
}

inline std::string ini_value(const std::string& text, const std::string& key) {
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? text.size() : end + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind(key + "=", 0) == 0) return line.substr(key.size() + 1);
    }
    return "";
}

inline std::string with_ini_value(const std::string& text, const std::string& key, const std::string& value) {
    std::string out;
    bool written = false;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? text.size() : end + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind(key + "=", 0) == 0) {
            if (written) continue;
            line = key + "=" + value, written = true;
        }
        out += line + "\n";
    }
    if (!written) out += key + "=" + value + "\n";
    return out;
}

struct Announcement { std::string id, title, text; };

inline Announcement parse_announcement(std::string file) {
    file.erase(std::remove(file.begin(), file.end(), '\r'), file.end());
    size_t split = file.find("\n\n");
    std::string head = file.substr(0, split), text = split == std::string::npos ? "" : file.substr(split + 2);
    while (!text.empty() && (text.back() == '\n' || text.back() == ' ')) text.pop_back();
    Announcement a{ini_value(head, "id"), ini_value(head, "title"), text};
    if (a.id.empty() || a.title.empty()) return {};
    return a;
}

struct Area { int x = 0, y = 0, w = 0, h = 0; };

inline Area hyprland_focused_monitor(const std::string& text) {
    Area found, current;
    int left = 0, top = 0, right = 0, bottom = 0, width = 0, height = 0;
    float scale = 1;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? text.size() : end + 1;
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        line = line.substr(first);
        if (line.rfind("Monitor ", 0) == 0) left = top = right = bottom = width = height = 0, scale = 1, current = {};
        else if (sscanf(line.c_str(), "%dx%d@%*f at %dx%d", &width, &height, &current.x, &current.y) == 4) {}
        else if (sscanf(line.c_str(), "reserved: %d %d %d %d", &left, &top, &right, &bottom) == 4) {}
        else if (sscanf(line.c_str(), "scale: %f", &scale) == 1) {}
        else if (line == "focused: yes" && width > 0 && scale > 0) {
            found.x = current.x + left, found.y = current.y + top;
            found.w = (int)(width / scale) - left - right, found.h = (int)(height / scale) - top - bottom;
        }
    }
    return found;
}

}
