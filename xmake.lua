add_rules("mode.debug", "mode.release")

if is_mode("release") then set_optimize("none") end

if is_mode("debug") then
    -- add_links("gcov")
    -- add_cxxflags("-fprofile-arcs", "-ftest-coverage")
    add_cxxflags("-ggdb")
    add_ldflags("-no-pie")
end

set_languages("c++20")
add_cxxflags("-Wall", "-Werror", "-fconcepts")

add_includedirs("include")

target("cocpp")
set_kind("static")
if is_mode("release") then
    add_cxxflags("-fauto-inc-dec")
    add_cxxflags("-fbranch-count-reg")
    add_cxxflags("-fcombine-stack-adjustments")
    add_cxxflags("-fcompare-elim")
    add_cxxflags("-fcprop-registers")
    add_cxxflags("-fdce")
    add_cxxflags("-fdefer-pop")
    add_cxxflags("-fdse")
    add_cxxflags("-fforward-propagate")
    add_cxxflags("-fguess-branch-probability")
    add_cxxflags("-fif-conversion")
    add_cxxflags("-fif-conversion2")
    add_cxxflags("-finline-functions-called-once")
    add_cxxflags("-fipa-modref")
    add_cxxflags("-fipa-profile")
    add_cxxflags("-fipa-pure-const")
    add_cxxflags("-fipa-reference")
    add_cxxflags("-fipa-reference-addressable")
    add_cxxflags("-fmerge-constants")
    add_cxxflags("-fmove-loop-invariants")
    add_cxxflags("-fno-omit-frame-pointer")
    add_cxxflags("-freorder-blocks")
    add_cxxflags("-fshrink-wrap")
    add_cxxflags("-fshrink-wrap-separate")
    add_cxxflags("-fsplit-wide-types")
    add_cxxflags("-fssa-backprop")
    add_cxxflags("-fssa-phiopt")
    add_cxxflags("-ftree-bit-ccp")
    add_cxxflags("-ftree-ccp")
    add_cxxflags("-ftree-ch")
    add_cxxflags("-ftree-coalesce-vars")
    add_cxxflags("-ftree-copy-prop")
    add_cxxflags("-ftree-dce")
    add_cxxflags("-ftree-dominator-opts")
    add_cxxflags("-ftree-dse")
    add_cxxflags("-ftree-forwprop")
    add_cxxflags("-ftree-fre")
    add_cxxflags("-ftree-phiprop")
    add_cxxflags("-ftree-pta")
    add_cxxflags("-ftree-scev-cprop")
    add_cxxflags("-ftree-sink")
    add_cxxflags("-ftree-slsr")
    add_cxxflags("-ftree-sra")
    add_cxxflags("-ftree-ter")
    add_cxxflags("-funit-at-a-time")
    add_cxxflags("-falign-functions")
    add_cxxflags("-falign-jumps")
    add_cxxflags("-falign-labels")
    add_cxxflags("-falign-loops")
    add_cxxflags("-fcaller-saves")
    add_cxxflags("-fcode-hoisting")
    add_cxxflags("-fcrossjumping")
    add_cxxflags("-fcse-follow-jumps")
    add_cxxflags("-fcse-skip-blocks")
    add_cxxflags("-fdelete-null-pointer-checks")
    add_cxxflags("-fdevirtualize")
    add_cxxflags("-fdevirtualize-speculatively")
    add_cxxflags("-fexpensive-optimizations")
    add_cxxflags("-ffinite-loops")
    add_cxxflags("-fgcse")
    add_cxxflags("-fgcse-lm")
    add_cxxflags("-fhoist-adjacent-loads")
    add_cxxflags("-finline-functions")
    add_cxxflags("-finline-small-functions")
    add_cxxflags("-findirect-inlining")
    add_cxxflags("-fipa-bit-cp")
    add_cxxflags("-fipa-cp")
    add_cxxflags("-fipa-icf")
    add_cxxflags("-fipa-ra")
    add_cxxflags("-fipa-sra")
    add_cxxflags("-fipa-vrp")
    add_cxxflags("-fisolate-erroneous-paths-dereference")
    add_cxxflags("-flra-remat")
    add_cxxflags("-foptimize-sibling-calls")
    add_cxxflags("-foptimize-strlen")
    add_cxxflags("-fpartial-inlining")
    add_cxxflags("-fpeephole2")
    add_cxxflags("-freorder-blocks-algorithm=stc")
    add_cxxflags("-freorder-blocks-and-partition")
    add_cxxflags("-freorder-functions")
    add_cxxflags("-frerun-cse-after-loop")
    add_cxxflags("-fschedule-insns")
    add_cxxflags("-fschedule-insns2")
    add_cxxflags("-fsched-interblock")
    add_cxxflags("-fsched-spec")
    add_cxxflags("-fstore-merging")
    add_cxxflags("-fstrict-aliasing")
    add_cxxflags("-fthread-jumps")
    add_cxxflags("-ftree-builtin-call-dce")
    add_cxxflags("-ftree-pre")
    add_cxxflags("-ftree-switch-conversion")
    add_cxxflags("-ftree-tail-merge")
    add_cxxflags("-ftree-vrp")
    add_cxxflags("-fgcse-after-reload")
    add_cxxflags("-fipa-cp-clone")
    add_cxxflags("-floop-interchange")
    add_cxxflags("-floop-unroll-and-jam")
    add_cxxflags("-fpeel-loops")
    add_cxxflags("-fpredictive-commoning")
    add_cxxflags("-fsplit-loops")
    add_cxxflags("-fsplit-paths")
    add_cxxflags("-ftree-loop-distribution")
    add_cxxflags("-ftree-loop-vectorize")
    add_cxxflags("-ftree-partial-pre")
    add_cxxflags("-ftree-slp-vectorize")
    add_cxxflags("-funswitch-loops")
    add_cxxflags("-fvect-cost-model")
    add_cxxflags("-fvect-cost-model=dynamic")
    add_cxxflags("-fversion-loops-for-strides")
end

add_files("source/**/*.cpp")
add_headerfiles("./include/(**/*.h)")
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
