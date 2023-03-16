add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

if is_mode("debug") then
    set_runtimes("MDd")
else
    set_runtimes("MD")
end

add_requires("fmt")
add_requires("raw_pdb")
add_requires("nlohmann_json")
add_requires("parallel-hashmap")

add_requires("detours v4.0.1-xmake.0")
add_requires("demangler v1.0.1")

target("PreLoader")
    set_kind("shared")
    set_languages("c++17")
    set_symbols("debug")
    add_headerfiles("include/**.h")
    add_includedirs("./include")
    add_defines("PRELOADER_EXPORT", "UNICODE")
    add_cxxflags("/UTF-8")
    add_files("src/**.cpp")
    add_packages("raw_pdb", "nlohmann_json", "parallel-hashmap", "demangler", "detours", "fmt")
