add_rules("mode.debug", "mode.release")

set_languages("c++20")

target("cocpp")
set_kind("shared")
add_includedirs("include")
add_files("source/*.cpp")
add_links("pthread")
if is_mode("debug") then
    add_links("gcov")
    add_cxxflags("-fprofile-arcs", "-ftest-coverage")
end
target_end()

target("test")
set_kind("binary")
add_includedirs("include")
add_files("test/*.cpp")
add_links("gtest", "pthread")
add_deps("cocpp")
if is_mode("debug") then
    add_links("gcov")
    add_cxxflags("-fprofile-arcs", "-ftest-coverage")
    after_build(function()
        import("core.project.task")
        task.run("test_cov")
    end)
end
target_end()

task("test_cov")
on_run(function()
    local exe = "$(buildir)/$(plat)/$(arch)/$(mode)"
    os.exec("lcov -d ./ -z")
    os.exec(exe .. "/test")
    os.exec("lcov -c -d ./ -o cover.info")
    os.exec(
        "lcov --remove cover.info '*/usr/include/*' '*/usr/lib/*' '*/usr/lib64/*' '*/usr/local/include/*' '*/usr/local/lib/*' '*/usr/local/lib64/*' -o final.info")
    os.exec(
        "genhtml -o cover_report --legend --title 'lcov'  --prefix=./ final.info")
end)
task_end()
