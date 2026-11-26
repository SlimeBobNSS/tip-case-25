#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cpack --config build/CPackConfig.cmake -G TGZ -B build

printf '%s\n' 'Пакет .tar.gz создан в каталоге build.'
