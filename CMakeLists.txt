cmake_minimum_required(VERSION 3.10)

project(case25_time_tracker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TIME_TRACKER_SOURCES
    src/DateTime.cpp
    src/Project.cpp
    src/Task.cpp
    src/TimeEntry.cpp
    src/CsvStorage.cpp
    src/TimerController.cpp
    src/ReportService.cpp
    src/ReportCsvExporter.cpp
    src/ProjectTaskService.cpp
    src/HistoryService.cpp
    src/IdGenerator.cpp
)

add_executable(target_exec src/main.cpp ${TIME_TRACKER_SOURCES})
target_include_directories(target_exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(target_exec PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(target_exec PRIVATE /W4 /permissive-)
else()
    target_compile_options(target_exec PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(target_exec_tests tests/test_main.cpp ${TIME_TRACKER_SOURCES})
    target_include_directories(target_exec_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(target_exec_tests PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(target_exec_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(target_exec_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME time_tracker_tests COMMAND target_exec_tests)
endif()

install(TARGETS target_exec RUNTIME DESTINATION bin)
install(FILES README.md DESTINATION .)
install(FILES
    docs/ИНСТРУКЦИЯ_ДЛЯ_ПРЕПОДАВАТЕЛЯ.md
    docs/ПЛАН_ТЕСТИРОВАНИЯ.md
    docs/ТЗ_Кейс_25_Тайм-трекер.docx
    DESTINATION docs
)

set(CPACK_PACKAGE_NAME "time-tracker-case25")
set(CPACK_PACKAGE_VENDOR "RTU MIREA")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Консольный тайм-трекер на C++17")
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)
