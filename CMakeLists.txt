cmake_minimum_required(VERSION 3.10)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

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
    if(POLICY CMP0091)
        set_property(TARGET target_exec PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        target_compile_options(target_exec PRIVATE "$<$<CONFIG:Debug>:/MTd>" "$<$<NOT:$<CONFIG:Debug>>:/MT>")
    endif()
elseif(MINGW)
    target_link_libraries(target_exec PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(MSVC)
    target_compile_options(target_exec PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(target_exec PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(target_exec_tests tests/test_main.cpp ${TIME_TRACKER_SOURCES})
    target_include_directories(target_exec_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_features(target_exec_tests PRIVATE cxx_std_17)
    if(MSVC)
        if(POLICY CMP0091)
            set_property(TARGET target_exec_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        else()
            target_compile_options(target_exec_tests PRIVATE "$<$<CONFIG:Debug>:/MTd>" "$<$<NOT:$<CONFIG:Debug>>:/MT>")
        endif()
    elseif(MINGW)
        target_link_libraries(target_exec_tests PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    if(MSVC)
        target_compile_options(target_exec_tests PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(target_exec_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME time_tracker_tests COMMAND target_exec_tests)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME time_tracker_cli COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.py $<TARGET_FILE:target_exec>)
    endif()
endif()

install(TARGETS target_exec RUNTIME DESTINATION bin)
install(FILES README.md DESTINATION .)
install(FILES
    docs/ИНСТРУКЦИЯ_ДЛЯ_ПРЕПОДАВАТЕЛЯ.md
    docs/ПЛАН_ТЕСТИРОВАНИЯ.md
    docs/ТЗ_Кейс_25_Тайм-трекер.docx
    docs/ТЗ_Кейс_25_Тайм-трекер.pdf
    DESTINATION docs
)

set(CPACK_PACKAGE_NAME "time-tracker-case25")
set(CPACK_PACKAGE_VENDOR "RTU MIREA")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Консольный тайм-трекер на C++17")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)
