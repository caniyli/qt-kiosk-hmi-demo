cmake_minimum_required(VERSION 3.16)

project(qt-kiosk-hmi-demo VERSION 1.0.0 LANGUAGES CXX)

option(HMI_BUILD_TESTS "Build the unit tests (needs Qt Test)" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

# Qt 5.12 or newer. A Qt 6 path exists below but has not been tested.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
if(QT_VERSION_MAJOR EQUAL 5 AND QT_VERSION VERSION_LESS 5.12)
    message(FATAL_ERROR "Qt 5.12 or newer is required, found ${QT_VERSION}")
endif()
set(QT Qt${QT_VERSION_MAJOR})
find_package(${QT} REQUIRED COMPONENTS Core)
message(STATUS "Using Qt ${QT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Core library: data layer shared by the application and the tests -------
add_library(hmi_core STATIC
    src/alarmmodel.cpp src/alarmmodel.h
    src/machinebackend.cpp src/machinebackend.h
    src/processvalue.cpp src/processvalue.h
    src/simulatedbackend.cpp src/simulatedbackend.h
)
target_include_directories(hmi_core PUBLIC src)
target_link_libraries(hmi_core PUBLIC ${QT}::Core)

# --- Tests -------------------------------------------------------------------
if(HMI_BUILD_TESTS)
    find_package(${QT} QUIET COMPONENTS Test)
    if(${QT}Test_FOUND)
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "Qt Test not found, unit tests are skipped")
    endif()
endif()
