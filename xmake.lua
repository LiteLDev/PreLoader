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

add_requires("detours v4.0.1")
add_requires("demangler v1.0.0")

target("PreLoader")
    set_kind("shared")
    set_languages("c++17")
    add_headerfiles("include/**.h")
    add_includedirs("./include")
    add_defines("PRELOADER_EXPORT", "UNICODE")
    add_cxxflags("/UTF-8", "/GL")
    if is_mode("debug") then
        add_defines("DEBUG")
        add_cxxflags("/MDd")
        add_ldflags("/DEBUG")
    else
        add_defines("NDEBUG")
        add_cxxflags("/MD")
        add_ldflags("/OPT:REF", "/OPT:ICF")
    end
    add_ldflags("/LTCG")
    add_files("src/**.cpp")
    add_packages("raw_pdb", "nlohmann_json", "parallel-hashmap", "demangler", "detours", "fmt")
