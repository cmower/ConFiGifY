#!/bin/sh
set -eu
project_dir=$(CDPATH='' cd "$(dirname "$0")/../.." && pwd)
cd "$project_dir"
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
cmake -S . -B "$test_dir" -DBUILD_TESTING=ON
cmake --build "$test_dir" --target cfgy-test
(cd "$test_dir" && ctest --output-on-failure)
