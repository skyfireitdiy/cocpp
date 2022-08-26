add_rules("mode.debug", "mode.release")

if is_mode("debug") then
    -- add_links("gcov")
    -- add_cxxflags("-fprofile-arcs", "-ftest-coverage")
    add_cxxflags("-ggdb")
    add_ldflags("-no-pie")
end

set_languages("c++20")
add_cxxflags("-Wall", "-Werror")
add_cxxflags("-fconcepts")

add_includedirs("include")

target("cocpp")
set_kind("static")

add_files("source/**/*.cpp")
add_headerfiles("./include/(**/*.h)")
target_end()

-- 以下为test

add_links("pthread")

target("test")
set_kind("binary")
add_includedirs("3rd/gtest_install/include")
add_linkdirs("3rd/gtest_install/lib")
add_includedirs("include")
add_files("test/*.cpp")
add_links("gtest")
add_deps("cocpp")
if is_mode("debug") then
    after_build(function()
        -- import("core.project.task")
        -- task.run("test_cov")
    end)
end

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
on_install(function(target) print("ignore install example") end)
target_end()

target("different_entry")
set_kind("binary")
add_files("example/different_entry.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("bind_env")
set_kind("binary")
add_files("example/bind_env.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("dead_loop")
set_kind("binary")
add_files("example/dead_loop.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("mutex")
set_kind("binary")
add_files("example/mutex.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("fixed_size_channel")
set_kind("binary")
add_files("example/fixed_size_channel.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("nobuf_channel")
set_kind("binary")
add_files("example/nobuf_channel.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("no_limited_channel")
set_kind("binary")
add_files("example/no_limited_channel.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("co_local")
set_kind("binary")
add_files("example/co_local.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("timer")
set_kind("binary")
add_files("example/timer.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()

target("pipeline")
set_kind("binary")
add_files("example/pipeline.cpp")
add_deps("cocpp")
on_install(function(target) print("ignore install example") end)
target_end()
