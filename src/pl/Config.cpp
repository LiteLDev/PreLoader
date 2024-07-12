#include "pl/Config.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "pl/internal/WindowsUtils.h"

#include "nlohmann/json.hpp"

namespace pl {
char const* pl_mod_manager_name = "preload-native";
char        pl_color_log        = utils::isStdoutSupportAnsi();
int         pl_log_level        = 4;
char const* pl_log_path         = "./logs/server-latest.log";
char const* pl_mods_path        = "./plugins/"; // TODO: change to mods

nlohmann::json config;

void loadConfig() try {
    auto configPath = std::filesystem::path{u8"PreloaderConfig.json"};

    try {
        if (std::filesystem::exists("./mods/")) { pl_mods_path = "./mods/"; } // TODO: remove when release
        if (std::filesystem::exists(configPath)) {
            std::ifstream{configPath} >> config;
            if (config["version"] == 1) {
                pl_color_log = (bool)config["colorLog"];
                pl_log_level = config["logLevel"];
                pl_log_path  = config["logPath"].get_ref<std::string const&>().c_str();
                pl_mods_path = config["modsPath"].get_ref<std::string const&>().c_str();
                return;
            }
        }
    } catch (...) {}
    config.clear();
    config["version"]  = 1;
    config["colorLog"] = pl_color_log;
    config["logLevel"] = pl_log_level;
    config["logPath"]  = pl_log_path;
    config["modsPath"] = pl_mods_path;
    std::ofstream{configPath} << config.dump(4);
} catch (...) {}

} // namespace pl
