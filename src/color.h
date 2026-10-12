#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace color {

inline int to_byte(float v) { return (int)std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f); }

inline std::string to_hex(const float rgb[3]) {
    char b[8];
    snprintf(b, sizeof b, "#%02X%02X%02X", to_byte(rgb[0]), to_byte(rgb[1]), to_byte(rgb[2]));
    return b;
}

inline bool from_hex(const std::string& text, float rgb[3]) {
    std::string digits = !text.empty() && text[0] == '#' ? text.substr(1) : text;
    if (digits.size() != 6 || digits.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) return false;
    unsigned long v = strtoul(digits.c_str(), nullptr, 16);
    rgb[0] = ((v >> 16) & 0xFF) / 255.0f, rgb[1] = ((v >> 8) & 0xFF) / 255.0f, rgb[2] = (v & 0xFF) / 255.0f;
    return true;
}

inline void rainbow(float rgb[3], double turns) {
    float v = std::max({rgb[0], rgb[1], rgb[2]});
    if (v < 0.05f) v = 1.0f;
    float h = (float)(turns - std::floor(turns)) * 6.0f, f = h - std::floor(h), up = v * f, down = v * (1 - f);
    const float table[6][3] = {{v, up, 0}, {down, v, 0}, {0, v, up}, {0, down, v}, {up, 0, v}, {v, 0, down}};
    const float* c = table[std::clamp((int)h, 0, 5)];
    rgb[0] = c[0], rgb[1] = c[1], rgb[2] = c[2];
}

}
