set_project("breeze-js")
set_policy("compatibility.version", "3.0")

set_languages("c++2b")
set_warnings("all") 
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
add_rules("mode.releasedbg")

add_requires("cxxopts")
add_requires("yalantinglibs", {
    configs = {
        ssl = true
    }
})

set_runtimes("MT")

includes("bindgen.lua")


target("breeze-quickjs-ng")
    set_kind("static")
    set_languages("c89", "c++20")
    if is_plat("linux", "bsd", "cross") then
        add_syslinks("m", "pthread")
    end
    add_files("src/quickjs/*.c")
    add_headerfiles("src/quickjs/*.h")
    add_includedirs("src/quickjs", {public = true})

target("breeze-js-runtime")
    set_kind("static")
    add_deps("breeze-quickjs-ng")
    add_packages("yalantinglibs", { public = true })
    add_rules("breezejs.bindgen", {
        name_filter = "breeze::js",
        ts_module_name = "breeze",
    })
    add_files("src/breeze-js/*.cc", "src/breeze-js/**/*.cc")
    add_headerfiles("src/breeze-js/*.h")
    add_includedirs("src/breeze-js", {public = true})

target("breeze-js-cli")
    set_kind("binary")
    add_deps("breeze-js-runtime")
    add_packages("cxxopts")
    add_files("src/breeze-js-cli/*.cc")
    
    if is_plat("windows") then
        add_syslinks("ws2_32", "user32", "shell32")
    end