cmake_minimum_required(VERSION 3.16)

project(qt-kiosk-hmi-demo VERSION 1.0.0 LANGUAGES CXX)

option(HMI_BUILD_TESTS "Build the unit tests (needs Qt Test)" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt 5.12 or newer. A Qt 6 path exists below but has not been tested.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
if(QT_VERSION_MAJOR EQUAL 5 AND QT_VERSION VERSION_LESS 5.12)
    message(FATAL_ERROR "Qt 5.12 or newer is required, found ${QT_VERSION}")
endif()
set(QT Qt${QT_VERSION_MAJOR})
find_package(${QT} REQUIRED COMPONENTS Core Gui Qml Quick LinguistTools)
message(STATUS "Using Qt ${QT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Core library: data layer shared by the application and the tests -------
add_library(hmi_core STATIC
    src/alarmmodel.cpp src/alarmmodel.h
    src/languagemanager.cpp src/languagemanager.h
    src/machinebackend.cpp src/machinebackend.h
    src/processvalue.cpp src/processvalue.h
    src/simulatedbackend.cpp src/simulatedbackend.h
)
target_include_directories(hmi_core PUBLIC src)
target_link_libraries(hmi_core PUBLIC ${QT}::Core ${QT}::Qml)

# --- Application -------------------------------------------------------------
add_executable(hmi-demo src/main.cpp qml/qml.qrc)
target_link_libraries(hmi-demo PRIVATE hmi_core ${QT}::Gui ${QT}::Quick)
target_compile_definitions(hmi-demo PRIVATE HMI_VERSION="${PROJECT_VERSION}")

# Translations are compiled with lrelease and embedded as :/i18n/hmi_<lang>.qm
set(HMI_TS_FILES i18n/hmi_tr.ts)
if(QT_VERSION_MAJOR EQUAL 5)
    qt5_add_translation(HMI_QM_FILES ${HMI_TS_FILES})
    set(HMI_I18N_QRC "${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc")
    set(HMI_I18N_QRC_CONTENT "<RCC>\n  <qresource prefix=\"/i18n\">\n")
    foreach(qm ${HMI_QM_FILES})
        get_filename_component(qm_name "${qm}" NAME)
        string(APPEND HMI_I18N_QRC_CONTENT "    <file>${qm_name}</file>\n")
    endforeach()
    string(APPEND HMI_I18N_QRC_CONTENT "  </qresource>\n</RCC>\n")
    # Written at configure time (qt5_add_resources reads it to find the .qm
    # dependencies) and only when the content changes, to avoid rebuilds.
    set(HMI_I18N_QRC_OLD "")
    if(EXISTS "${HMI_I18N_QRC}")
        file(READ "${HMI_I18N_QRC}" HMI_I18N_QRC_OLD)
    endif()
    if(NOT HMI_I18N_QRC_OLD STREQUAL HMI_I18N_QRC_CONTENT)
        file(WRITE "${HMI_I18N_QRC}" "${HMI_I18N_QRC_CONTENT}")
    endif()
    qt5_add_resources(HMI_I18N_SOURCES "${HMI_I18N_QRC}")
    target_sources(hmi-demo PRIVATE ${HMI_QM_FILES} ${HMI_I18N_SOURCES})
else()
    qt_add_translations(hmi-demo TS_FILES ${HMI_TS_FILES} RESOURCE_PREFIX /i18n)
endif()

# `cmake --build build --target update_translations` refreshes the .ts files
# from the sources; translate with Qt Linguist afterwards.
add_custom_target(update_translations
    COMMAND ${QT}::lupdate ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/qml
            -locations none -ts ${HMI_TS_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Updating translation sources"
    VERBATIM)

include(GNUInstallDirs)
install(TARGETS hmi-demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

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
