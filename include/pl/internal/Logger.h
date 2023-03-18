#pragma once
#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

inline bool shouldLogColor;

inline void loadConfigFromJson(const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        shouldLogColor = true;
        return;
    }
    nlohmann::json json;
    file >> json;
    file.close();
    if (json.find("ColorLog") != json.end()) {
        shouldLogColor = json["ColorLog"];
    } else {
        shouldLogColor = true;
    }
}

inline void loadLoggerConfig() {
    if (std::filesystem::exists("plugins/LiteLoader/LiteLoader.json")) {
        try {
            loadConfigFromJson("plugins/LiteLoader/LiteLoader.json");
        } catch (std::exception& e) {
            shouldLogColor = true;
        } catch (...) {
            shouldLogColor = true;
        }
    } else {
        shouldLogColor = true;
    }
}

#define COLOR_TIME         fmt::color::light_blue
#define COLOR_INFO_PREFIX  fmt::color::light_sea_green
#define COLOR_INFO_TEXT    fmt::terminal_color::white
#define COLOR_WARN_PREFIX  fmt::terminal_color::bright_yellow
#define COLOR_WARN_TEXT    fmt::terminal_color::yellow
#define COLOR_ERROR_PREFIX fmt::terminal_color::bright_red
#define COLOR_ERROR_TEXT   fmt::terminal_color::red

#define LOG_PREFIX(prefix, color1, color2)                                                                             \
    fmt::print(                                                                                                        \
        shouldLogColor ? fmt::fg(color1) : fmt::text_style(),                                                          \
        fmt::format("{:%H:%M:%S}", fmt::localtime(_time64(0)))                                                         \
    );                                                                                                                 \
    fmt::print(shouldLogColor ? fmt::fg(color2) : fmt::text_style(), fmt::format(prefix, fmt::localtime(_time64(0))));
#define LOG(color1, color2, prefix)                                                                                    \
    LOG_PREFIX(prefix, color1, color2);                                                                                \
    std::string str = fmt::format("[PreLoader] ", fmt::localtime(_time64(0)));                                         \
    str             += fmt::format(fmt::runtime(formatStr), args...);                                                  \
    str.append(1, '\n');

template <typename... Args>
void inline Info(const std::string& formatStr, const Args&... args) {
    LOG(COLOR_TIME, COLOR_INFO_PREFIX, " INFO ");
    fmt::print(fmt::fg(COLOR_INFO_TEXT), str);
}

template <typename... Args>
void inline Warn(const std::string& formatStr, const Args&... args) {
    LOG(COLOR_TIME, COLOR_WARN_PREFIX, " WARN ");
    fmt::print(fmt::fg(COLOR_WARN_TEXT) | fmt::emphasis::bold, str);
}

template <typename... Args>
void inline Error(const std::string& formatStr, const Args&... args) {
    LOG(COLOR_TIME, COLOR_ERROR_PREFIX, " ERROR ");
    fmt::print(fmt::fg(COLOR_ERROR_TEXT) | fmt::emphasis::bold, str);
}
