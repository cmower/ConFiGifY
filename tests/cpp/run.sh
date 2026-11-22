#!/bin/sh
set -eu
project_dir=$(CDPATH='' cd "$(dirname "$0")/../.." && pwd)
cd "$project_dir"
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
./examples/cpp/build.sh "$test_dir"
"$test_dir/cfgy-gen" example.cfgy TestConfig > "$test_dir/test_config.hpp"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Wpedantic -Isrc/cpp -I"$test_dir" \
    tests/cpp/test_configify.cpp "$test_dir/cfgy.o" -o "$test_dir/test"
"$test_dir/test"
