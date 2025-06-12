rule("breezejs.bindgen")
set_extensions(".breezejs.cc", ".breezejs.cpp", ".breezejs.cxx")
on_load(function(target)
    import("lib.detect.find_tool")
    local bindgen = {
        program = target:is_plat("windows") and "npx.cmd" or "npx",
        argv = { "breeze-bindgen@latest" }
    }
    target:data_set("breeze_bindgen.tool", bindgen)
    local clang = find_tool("clang++") or find_tool("clang")
    if clang then
        target:data_set("breeze.bindgen.clang", clang.program)
    else
        print("${yellow}clang++ not found, breeze-bindgen may not work properly")
    end
end)
on_build_files(function(target, jobgraph, sourcebatch, opt)
    for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
        local group_name = path.join(target:fullname(), "obj", sourcefile)
        jobgraph:group(group_name, function()
            local bindgen = target:data("breeze_bindgen.tool")
            if not bindgen then
                return
            end

            local output_dir = target:extraconf("rules", "breezejs.bindgen", "output_dir") or path.directory(sourcefile)
            local cpp_output_file = target:extraconf("rules", "breezejs.bindgen", "cpp_output_file")
            local ts_output_file = target:extraconf("rules", "breezejs.bindgen", "ts_output_file")
            local ts_module_name = target:extraconf("rules", "breezejs.bindgen", "ts_module_name")
            local ext_types_path = target:extraconf("rules", "breezejs.bindgen", "ext_types_path")
            local name_filter = target:extraconf("rules", "breezejs.bindgen", "name_filter")
            local flags = target:extraconf("rules", "breezejs.bindgen", "flags") or {}

            local filename = path.basename(sourcefile)
            local base_output = path.join(output_dir, filename)
            local default_ts_output = base_output .. ".d.ts"
            local default_cpp_output = base_output .. ".qjs.h"

            local argv = {}
            if bindgen.argv then
                table.join2(argv, bindgen.argv)
            end

            table.insert(argv, "-i")
            table.insert(argv, sourcefile)
            table.insert(argv, "-o")
            table.insert(argv, output_dir)
            table.insert(argv, "--cppBindingOutputFile")
            table.insert(argv, cpp_output_file or path.filename(default_cpp_output))
            table.insert(argv, "--tsDefinitionOutputFile")
            table.insert(argv, ts_output_file or path.filename(default_ts_output))

            if ts_module_name then
                table.insert(argv, "--tsModuleName")
                table.insert(argv, ts_module_name)
            end
            if ext_types_path then
                table.insert(argv, "--extTypesPath")
                table.insert(argv, ext_types_path)
            end
            if name_filter then
                table.insert(argv, "--nameFilter")
                table.insert(argv, name_filter)
            end

            for _, flag in ipairs(flags) do
                table.insert(argv, flag)
            end

            local clang = target:data("breeze.bindgen.clang")
            if clang then
                table.insert(argv, "--clang")
                table.insert(argv, clang)
            end

            local bindgen_tool = target:data("breeze_bindgen.tool")
            if not bindgen_tool then
                return
            end
            print(target:dependfile(sourcefile))
            jobgraph:add(path.join(group_name, sourcefile), function (index, total, opt)
                import("core.project.depend")
                import("utils.progress")
                depend.on_changed(function()
                    progress.show(opt.progress or 0, "${color.build.object}compiling.g4 %s", sourcefile)
                    os.vrunv(bindgen_tool.program, argv)
                end, {
                    files = sourcefile,
                    dependfile = target:dependfile(sourcefile),
                    changed = target:is_rebuilt()
                })
            end)
        end)
    end
end, { jobgraph = true, distcc = true })
