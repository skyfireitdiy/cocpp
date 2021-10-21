#!/bin/bash
set -e

cd 3rd
mkdir -p gtest_build
cd gtest_build
cmake ../googletest -DCMAKE_INSTALL_PREFIX=../gtest_install
make -j
make install
cd ..
rm -rf gtest_build

cd mockcpp
./build_install4gtest.sh $(pwd)/../gtest_install ../mockcpp_install
