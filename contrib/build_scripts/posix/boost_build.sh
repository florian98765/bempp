#!/usr/bin/sh
set -e
. ../../options.cfg

# mkdir $Main_prefix/bempp/contrib/boost
mkdir -p $Main_prefix/bempp/contrib
mkdir -p build

cd build

cmake \
 -D CMAKE_CXX_COMPILER:STRING:STRING=$Main_cxx \
 -D CMAKE_C_COMPILER:STRING=$Main_cc \
 -D CMAKE_INSTALL_PREFIX:PATH=$Main_prefix/bempp/contrib/boost \
 -D CMAKE_CXX_FLAGS:STRING=$Main_cxxflags \
 -D CMAKE_C_FLAGS:STRING=$Main_cflags \
 -D BUILD_PROJECTS:STRING=test  \
 -D CMAKE_BUILD_TYPE:STRING=Release \
 ..
make
make install
