add_rules("mode.debug", "mode.release")

set_languages("c++20")

target("cocpp")
set_kind("binary")
add_includedirs("include")
add_files("source/*.cpp")
add_files("test/test1.cpp")
if is_plat("linux") then
    add_links("pthread")
else
    add_files("source/*.s")
end
