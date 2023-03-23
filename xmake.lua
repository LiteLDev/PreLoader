add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

if not has_config("vs_runtime") then
    if is_mode("debug") then
        set_runtimes("MDd")
    else
        set_runtimes("MD")
    end
end

add_requires("fmt 9.1.0")
add_requires("raw_pdb 2022.10.17")
add_requires("nlohmann_json v3.11.2")
add_requires("parallel-hashmap 1.35")

add_requires("detours v4.0.1-xmake.1")
add_requires("demangler v1.0.2")

target("PreLoader")
    set_kind("shared")
    set_languages("c++17")
    set_symbols("debug")
    add_headerfiles("include/(**.h)")
    add_includedirs("./include")
    add_defines("PRELOADER_EXPORT", "UNICODE")
    add_cxflags("/utf-8")
    add_files("src/**.cpp")
    add_packages("raw_pdb", "nlohmann_json", "parallel-hashmap", "demangler", "detours", "fmt")
