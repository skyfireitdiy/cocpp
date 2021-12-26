add_rules("mode.debug", "mode.release")

set_languages("c++20")
add_cxxflags("-Wall", "-Werror")

add_includedirs("include")

target("cocpp")
set_kind("static")
add_cxxflags("-O0")
add_files("source/**/*.cpp")
add_headerfiles("./(**/*.h)")
target_end()

-- 以下为test

add_links("pthread")

target("test")
set_kind("binary")
add_includedirs("3rd/gtest_install/include", "3rd/mockcpp_install/include")
add_linkdirs("3rd/gtest_install/lib", "3rd/mockcpp_install/lib")
add_includedirs("include")
add_files("test/*.cpp")
add_links("gtest")
add_links("gcov")
add_cxxflags("-fprofile-arcs", "-ftest-coverage", "-ggdb")
add_deps("cocpp")

after_build(function()
    import("core.project.task")
    task.run("test_cov")
end)

on_install(function(target) print("ignore install test") end)
target_end()

task("test_cov")
on_run(function()
    local exe = "$(buildir)/$(plat)/$(arch)/$(mode)"
    os.exec("lcov -d ./ -z")
    os.exec(exe .. "/test --gtest_shuffle")
    os.exec("lcov -c -d ./ -o cover.info")
    os.exec(
        "lcov --remove cover.info '*/usr/include/*' '*/usr/lib/*' '*/usr/lib64/*' '*/usr/local/include/' '*/usr/local/lib/*' '*/usr/local/lib64/*' '*/3rd/*'  -o final.info")
    os.exec(
        "genhtml -o cover_report --legend --title 'lcov'  --prefix=./ final.info")
end)
task_end()

-- 以下为example

target("helloworld")
set_kind("binary")
add_files("example/helloworld.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install test") end)
target_end()
