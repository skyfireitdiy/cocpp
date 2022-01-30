#!/bin/bash

find ./ -name "*.cpp" -type f | grep -v "3rd/" | xargs clang-format  -i
find ./ -name "*.h" -type f | grep -v "3rd/" | xargs clang-format  -i

