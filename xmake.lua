set_project("breeze-js")
set_policy("compatibility.version", "3.0")

set_languages("cxx23", "c89")
set_warnings("all") 
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
add_rules("mode.releasedbg")

add_requires("cxxopts", "ctre", "concurrentqueue")

includes("deps/yalantinglibs.lua")
add_requires("yalantinglibs", {
    configs = {
        ssl = true
    }
})

includes("bindgen.lua")


target("breeze-quickjs-ng")
    set_kind("static")
    set_languages("c11", "c++23")
    add_defines("BREEZE_CUSTOM_JOB_QUEUE")
    if is_plat("linux", "bsd", "cross") then
        add_syslinks("m", "pthread")
        add_defines("_POSIX_C_SOURCE=199309L")
        add_defines("_GNU_SOURCE")
    end
    add_files("src/quickjs/*.c")
    add_headerfiles("src/quickjs/headers/(**.h)")
    add_includedirs("src/quickjs/headers/breeze-js/")
    add_includedirs("src/quickjs/headers/", { public = true })
target("breeze-js-runtime")
    set_kind("static")
    add_deps("breeze-quickjs-ng", { public = true })
    add_packages("yalantinglibs", { public = true })
    add_packages("ctre")
    add_packages("concurrentqueue", { public = true })
    add_rules("breezejs.bindgen", {
        name_filter = "breeze::js",
        ts_module_name = "breeze",
    })
    add_files("src/breeze-js/*.cc", "src/breeze-js/**/*.cc")
    add_headerfiles("src/breeze-js/headers/(**.h)", "src/breeze-js/headers/(**.hpp)")
    add_includedirs("src/breeze-js/headers/", {public = true})

    if is_plat("linux", "bsd", "cross") then
        add_syslinks("m", "pthread")
    elseif is_plat("windows") then
        add_syslinks("ws2_32", "user32", "shell32")
    elseif is_plat("macosx") then
        add_frameworks("CoreFoundation", "CoreServices")
    end

target("cli")
    set_kind("binary")
    add_deps("breeze-js-runtime")
    add_packages("cxxopts")
    add_files("src/breeze-js-cli/*.cc")
    -- On Linux, quickjs.a references JS_EnqueueJob which is defined in
    -- breeze-js-runtime.a, creating a circular static lib dependency.
    -- add_linkgroups wraps them in --start-group/--end-group so ld re-scans.
    if is_plat("linux", "bsd", "cross") then
        add_linkgroups("breeze-js-runtime", "breeze-quickjs-ng", {group = true})
    end

    if is_plat("windows") then
        add_syslinks("ws2_32", "user32", "shell32")
    end