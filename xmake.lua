add_rules("mode.debug", "mode.release")

set_languages("c++20")

if is_mode("debug") then
    add_links("gcov")
    add_cxxflags("-fprofile-arcs", "-ftest-coverage", "-ggdb")
end
add_cxxflags("-Wall", "-Werror")

target("cocpp")
set_kind("shared")

add_includedirs("comm/include")
add_includedirs("core/include")
add_includedirs("exception/include")
add_includedirs("interface/include")
add_includedirs("scheduler/include")
add_includedirs("sync/include")
add_includedirs("utils/include")

add_files("comm/source/*.cpp")
add_files("core/source/*.cpp")
add_files("exception/source/*.cpp")
add_files("interface/source/*.cpp")
add_files("scheduler/source/*.cpp")
add_files("sync/source/*.cpp")
add_files("utils/source/*.cpp")

add_links("pthread")
target_end()

target("test")
set_kind("binary")
add_includedirs("comm/include")
add_includedirs("core/include")
add_includedirs("exception/include")
add_includedirs("interface/include")
add_includedirs("scheduler/include")
add_includedirs("sync/include")
add_includedirs("utils/include")
add_files("test/*.cpp")
add_links("gtest", "pthread")
add_deps("cocpp")
if is_mode("debug") and is_plat("linux") then
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
    -- os.exec(exe .. "/test --gtest_shuffle --gtest_repeat=10")
    os.exec(exe .. "/test --gtest_shuffle")
    -- os.exec(exe .. "/test --gtest_filter='*shared_stack*'")
    os.exec("lcov -c -d ./ -o cover.info")
    os.exec(
        "lcov --remove cover.info '*/usr/include/*' '*/usr/lib/*' '*/usr/lib64/*' '*/usr/local/include/*' '*/usr/local/lib/*' '*/usr/local/lib64/*' -o final.info")
    os.exec(
        "genhtml -o cover_report --legend --title 'lcov'  --prefix=./ final.info")
end)
task_end()
