$ErrorActionPreference = "Stop"
Set-Location (Split-Path -Parent $PSScriptRoot)

cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cpack --config build/CPackConfig.cmake -C Release -G ZIP

Write-Host "Пакет .zip создан в каталоге build."
