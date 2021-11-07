add_rules("mode.debug", "mode.release")

set_languages("c++20")

add_includedirs("3rd/gtest_install/include", "3rd/mockcpp_install/include")
add_linkdirs("3rd/gtest_install/lib", "3rd/mockcpp_install/lib")

add_includedirs("comm/include")
add_includedirs("core/include")
add_includedirs("exception/include")
add_includedirs("interface/include")
add_includedirs("scheduler/include")
add_includedirs("sync/include")
add_includedirs("utils/include")
add_includedirs("mem/include")

if is_mode("debug") then
    add_links("gcov")
    add_cxxflags("-fprofile-arcs", "-ftest-coverage", "-ggdb")
end
add_cxxflags("-Wall", "-Werror")

target("cocpp")
set_kind("shared")
add_files("comm/source/*.cpp")
add_files("core/source/*.cpp")
add_files("exception/source/*.cpp")
add_files("interface/source/*.cpp")
add_files("scheduler/source/*.cpp")
add_files("sync/source/*.cpp")
add_files("utils/source/*.cpp")
add_files("mem/source/*.cpp")
add_links("pthread")

add_headerfiles("comm/include/*.h", {prefixdir = "cocpp"})
add_headerfiles("core/include/*.h", {prefixdir = "cocpp"})
add_headerfiles("exception/include/*.h", {prefixdir = "cocpp"})
add_headerfiles("interface/include/*.h", {prefixdir = "cocpp"})
add_headerfiles("scheduler/include/*.h", {prefixdir = "cocpp"})
add_headerfiles("sync/include/*.h", {prefixdir = "cocpp"})
add_headerfiles("utils/include/*.h", {prefixdir = "cocpp"})
add_headerfiles("mem/include/*.h", {prefixdir = "cocpp"})

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
add_includedirs("mem/include")
add_files("test/*.cpp")
add_links("gtest", "pthread")
add_deps("cocpp")
if is_mode("debug") and is_plat("linux") then
    after_build(function()
        import("core.project.task")
        task.run("test_cov")
    end)
end
on_install(function(target) print("ignore install test") end)
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
        "lcov --remove cover.info '*/usr/include/*' '*/usr/lib/*' '*/usr/lib64/*' '*/usr/local/include/' '*/usr/local/lib/*' '*/usr/local/lib64/*' '*/3rd/*'  -o final.info")
    os.exec(
        "genhtml -o cover_report --legend --title 'lcov'  --prefix=./ final.info")
end)
task_end()
