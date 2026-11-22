#!/bin/sh
set -eu
project_dir=$(CDPATH='' cd "$(dirname "$0")/../.." && pwd)
cd "$project_dir"
build_dir=${1:-build}
compiler=${CXX:-c++}
mkdir -p "$build_dir"
"$compiler" -std=c++17 -Isrc/cpp -c src/cpp/cfgy.cpp -o "$build_dir/cfgy.o"
"$compiler" -std=c++17 -Isrc/cpp src/cpp/tools/generate.cpp "$build_dir/cfgy.o" -o "$build_dir/cfgy-gen"
"$build_dir/cfgy-gen" --fields ExampleConfig c.myel1 myfloat2 > "$build_dir/example_config.hpp"
"$compiler" -std=c++17 -Isrc/cpp -I"$build_dir" examples/cpp/example.cpp "$build_dir/cfgy.o" -o "$build_dir/cfgy-example"
