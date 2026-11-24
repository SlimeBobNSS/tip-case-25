# Тайм-трекер — кейс №25

Консольное приложение на C++17 для учёта времени по проектам и задачам. Поддерживает создание проектов и задач, запуск и остановку таймера, просмотр истории, формирование отчёта за период и экспорт в CSV.

## Структура проекта

```text
include/                 заголовочные файлы модулей
src/                     исходный код приложения
tests/                   автоматические проверки
docs/                    ТЗ, инструкция преподавателю и план тестирования
scripts/                 команды сборки релизного пакета
.github/workflows/       автоматическая сборка и тестирование
CMakeLists.txt           сборка, тестирование и упаковка
README.md                описание проекта и инструкции
```

Каждый модуль, кроме `main.cpp`, разделён на заголовочный файл в `include/` и реализацию в `src/`. Внешние библиотеки не используются.

## Требования

- CMake 3.10 или новее;
- компилятор C++17: MSVC, GCC или Clang.

## Сборка и запуск

Выполняйте команды из корневой папки репозитория.

macOS:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/target_exec ./build/data
```

Linux:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/target_exec ./build/data
```

Windows с генератором Visual Studio:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\target_exec.exe .\build\data
```

Программа создаёт CSV-файлы в каталоге данных, переданном первым аргументом. Если аргумент не указан, используется каталог `data` рядом с текущим рабочим каталогом.

## Тестирование

Автоматические тесты собраны через CTest. После сборки запустите:

```bash
ctest --test-dir build --output-on-failure
```

В Windows используйте `ctest --test-dir build -C Release --output-on-failure`. Тесты проверяют обработку дат, правила сессий, расчёт пересечения отчётного периода, чтение и запись CSV и работу контроллера таймера. План и связь с критериями приёмки приведены в [плане тестирования](docs/ПЛАН_ТЕСТИРОВАНИЯ.md). GitHub Actions автоматически собирает проект и запускает CTest на Windows, macOS и Ubuntu при обновлении ветки `main` и при создании Pull Request.

## Упаковка и публикация

CPack собирает приложение и инструкции в архив. Для Windows запустите `.\scripts\package.ps1` в PowerShell; для macOS — `sh scripts/package.sh`. Готовый архив появится в `build/`. На macOS и Linux создаётся `.tar.gz`, на Windows — `.zip`. При отправке тега вида `v1.0.0` GitHub Actions собирает пакеты для Windows, macOS и Linux и прикрепляет их к GitHub Release. На Mac распакуйте `.tar.gz`, откройте Terminal в папке распаковки и запустите `./bin/target_exec ./data`.

Пользовательские инструкции: [инструкция преподавателю](docs/ИНСТРУКЦИЯ_ДЛЯ_ПРЕПОДАВАТЕЛЯ.md). Полные требования: [ТЗ кейса 25](docs/ТЗ_Кейс_25_Тайм-трекер.docx).
