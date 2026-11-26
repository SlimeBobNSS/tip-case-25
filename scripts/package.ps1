$ErrorActionPreference = "Stop"
Push-Location (Split-Path -Parent $PSScriptRoot)
try {
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw "Ошибка конфигурации CMake." }
    cmake --build build --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "Ошибка сборки." }
    ctest --test-dir build -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Тесты не пройдены." }
    cpack --config build/CPackConfig.cmake -C Release -G ZIP -B build
    if ($LASTEXITCODE -ne 0) { throw "Ошибка упаковки." }
    Write-Host "Пакет .zip создан в каталоге build."
} finally {
    Pop-Location
}
