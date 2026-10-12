#pragma once
#include <cstdlib>
#include <algorithm>
#include <climits>
#include <string>
#include <vector>

#define FATRAINER_VERSION "2.5 b4"
#define FATRAINER_NAME "Survivor Update"
#define INSTALLER_VERSION "2.2"
#define FATRAINER_EDITION "Fatum Version"
#define FATRAINER_EDITION_CAPS "FATUM VERSION"
inline const char VERSION_TAG[] = "FaTrainer-version:" FATRAINER_VERSION;
inline const char* VERSION = VERSION_TAG + sizeof "FaTrainer-version:" - 1;
inline const char* RELEASE_DOWNLOADS = "https://github.com/ifatum/DyingLightFaTrainer/releases/latest/download/";
inline const char* RELEASE_TAG_DOWNLOADS = "https://github.com/ifatum/DyingLightFaTrainer/releases/download/";
inline const char* RELEASES_API = "https://api.github.com/repos/ifatum/DyingLightFaTrainer/releases?per_page=100";
inline const char* BETA_DOWNLOADS = "https://github.com/ifatum/DyingLightFaTrainer/releases/download/beta/";
inline const char* INSTALLER_DOWNLOADS = "https://github.com/ifatum/DyingLightFaTrainer/releases/download/installer/";
inline const char* ANNOUNCEMENT_URL = "https://raw.githubusercontent.com/ifatum/DyingLightFaTrainer/main/announcement.txt";
inline const char* LAUNCH_FILE = "fatrainer.launch";
inline const char* FIRST_ARMED_VERSION = "2.3";
const long long LAUNCH_MAX_AGE = 180, LAUNCH_MAX_AHEAD = 30;

inline bool launch_armed(long long written, long long now) {
    return written > 0 && written <= now + LAUNCH_MAX_AHEAD && now - written <= LAUNCH_MAX_AGE;
}

const size_t VERSION_NUMBERS = 4;
const long FINAL_BUILD = LONG_MAX;

inline std::vector<long> version_parts(const std::string& v) {
    size_t at = v.find(" b");
    std::string numbers = v.substr(0, at);
    std::vector<long> parts;
    for (const char* p = numbers.c_str(); *p;) {
        char* end;
        parts.push_back(strtol(p, &end, 10));
        if (end == p || (*end && *end != '.')) return {};
        p = *end ? end + 1 : end;
    }
    if (parts.empty() || parts.size() > VERSION_NUMBERS) return {};
    long build = FINAL_BUILD;
    if (at != std::string::npos) {
        const char* p = v.c_str() + at + 2;
        char* end;
        build = strtol(p, &end, 10);
        if (end == p || *end || build < 1) return {};
    }
    parts.resize(VERSION_NUMBERS);
    parts.push_back(build);
    return parts;
}

inline bool newer_version(const std::string& candidate, const std::string& current) {
    std::vector<long> a = version_parts(candidate), b = version_parts(current);
    return !a.empty() && !b.empty() && a > b;
}

inline bool same_version(const std::string& a, const std::string& b) {
    std::vector<long> x = version_parts(a), y = version_parts(b);
    return !x.empty() && !y.empty() && std::equal(x.begin(), x.begin() + VERSION_NUMBERS, y.begin());
}

inline std::string version_slug(std::string v) {
    std::replace(v.begin(), v.end(), ' ', '-');
    return v;
}

struct ReleaseInfo { std::string version, sha256; unsigned long long size = 0; };

inline ReleaseInfo parse_release_info(const std::string& text) {
    ReleaseInfo r;
    std::string build, release;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        at = end == std::string::npos ? text.size() : end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.rfind("version ", 0) == 0) r.version = line.substr(8);
        else if (line.rfind("sha256 ", 0) == 0) r.sha256 = line.substr(7);
        else if (line.rfind("build ", 0) == 0) build = line.substr(6);
        else if (line.rfind("release ", 0) == 0) release = line.substr(8);
        else if (line.rfind("size ", 0) == 0) r.size = strtoull(line.c_str() + 5, nullptr, 10);
    }
    if (!build.empty()) r.version = build;
    if (!release.empty()) r.version = release;
    return r;
}
