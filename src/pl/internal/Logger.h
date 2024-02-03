#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "fmt/chrono.h" // IWYU pragma: keep
#include "fmt/color.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/os.h" // IWYU pragma: keep

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#include "pl/internal/StringUtils.h"
#include "pl/internal/WindowsUtils.h"

namespace fs = std::filesystem;

inline bool shouldLogColor = true;

inline void loadLoggerConfig() {
    try {
        std::ifstream  file(fs::path{pl::utils::sv2u8sv("plugins/LeviLamina/config.json")});
        nlohmann::json json;
        file >> json;
        file.close();
        shouldLogColor = json["logger"]["colorLog"];
    } catch (...) {}
}

#define COLOR_TIME         fmt::color::light_blue
#define COLOR_INFO_PREFIX  fmt::color::light_sea_green
#define COLOR_INFO_TEXT    fmt::terminal_color::white
#define COLOR_WARN_PREFIX  fmt::terminal_color::bright_yellow
#define COLOR_WARN_TEXT    fmt::terminal_color::yellow
#define COLOR_ERROR_PREFIX fmt::terminal_color::bright_red
#define COLOR_ERROR_TEXT   fmt::terminal_color::red

#define LOG_PREFIX(prefix, color1, color2)                                                                             \
    auto [time, ms] = ::pl::utils::getLocalTime();                                                                     \
    fmt::print(shouldLogColor ? fmt::fg(color1) : fmt::text_style(), fmt::format("{:%H:%M:%S}.{:0>3}", time, ms));     \
    fmt::print(shouldLogColor ? fmt::fg(color2) : fmt::text_style(), prefix);

#define LOG(color1, color2, prefix)                                                                                    \
    LOG_PREFIX(prefix, color1, color2);                                                                                \
    std::string str  = "[PreLoader] ";                                                                                 \
    str             += fmt::vformat(__fmt.get(), fmt::make_format_args(__args...));                                    \
    str.append(1, '\n');

template <typename... Args>
void inline Info(fmt::format_string<Args...> __fmt, Args&&... __args) {
    LOG(COLOR_TIME, COLOR_INFO_PREFIX, " INFO ");
    fmt::print(fmt::fg(COLOR_INFO_TEXT), str);
}

template <typename... Args>
void inline Warn(fmt::format_string<Args...> __fmt, Args&&... __args) {
    LOG(COLOR_TIME, COLOR_WARN_PREFIX, " WARN ");
    fmt::print(fmt::fg(COLOR_WARN_TEXT) | fmt::emphasis::bold, str);
}

template <typename... Args>
void inline Error(fmt::format_string<Args...> __fmt, Args&&... __args) {
    LOG(COLOR_TIME, COLOR_ERROR_PREFIX, " ERROR ");
    fmt::print(fmt::fg(COLOR_ERROR_TEXT) | fmt::emphasis::bold, str);
}
