add_rules("mode.debug", "mode.release")

set_languages("c++20")

target("cocpp")
set_kind("binary")
add_includedirs("include")
add_files("source/*.cpp")
add_files("test/test1.cpp")
add_links("pthread")
