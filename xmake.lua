add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

add_requires("fmt")
add_requires("raw_pdb 2022.10.17")
add_requires("nlohmann_json")
add_requires("parallel-hashmap 1.35")
add_requires("pe_bliss")

add_requires("detours v4.0.1-xmake.1")
add_requires("demangler ~17")

target("PreLoader")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")   
     set_exceptions("none")
    add_headerfiles("src/(**.h)")
    add_includedirs("./src")
    add_defines("PRELOADER_EXPORT", "UNICODE")
    add_cxflags("/utf-8", "/EHa")
    add_files("src/**.cpp")
    add_packages("raw_pdb", "nlohmann_json", "parallel-hashmap", "demangler", "detours", "fmt", "pe_bliss")
