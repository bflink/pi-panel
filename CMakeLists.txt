cmake_minimum_required(VERSION 3.20)
project(pi_panel VERSION 1.0 LANGUAGES CXX)

include(CTest)
option(PI_PANEL_BUILD_GUI "Build the Qt application and hardware tests" ON)
include(cmake/Telemetry.cmake)

if(PI_PANEL_BUILD_GUI)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_library(pi_telemetry_qt ViewModels/TelemetryViewModel.h ViewModels/TelemetryViewModel.cpp)
set_target_properties(pi_telemetry_qt PROPERTIES AUTOMOC ON)
target_link_libraries(pi_telemetry_qt PUBLIC Qt6::Core pi_telemetry)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiodcxx)

add_library(pi_panel_core
    Controllers/Ads1015.cpp
    Controllers/ButtonDebouncher.cpp
    Controllers/GpiodService.cpp
    Controllers/LedController.cpp
    Controllers/Thermistor.cpp
)
target_link_libraries(pi_panel_core PUBLIC PkgConfig::GPIOD)
target_compile_features(pi_panel_core PUBLIC cxx_std_20)
set_target_properties(pi_panel_core PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(pi_panel_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_include_directories(pi_panel_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(pi_panel main.cpp)

target_sources(pi_panel PRIVATE
    ViewModels/LedViewModel.h
    ViewModels/LedViewModel.cpp
)

set_target_properties(pi_panel PROPERTIES AUTOMOC ON)

qt_add_qml_module(pi_panel
    URI PiPanel
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES Main.qml TelemetryPanel.qml
)

target_link_libraries(pi_panel PRIVATE
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)
target_link_libraries(pi_panel PRIVATE Qt6::Core)
target_link_libraries(pi_panel PRIVATE pi_panel_core)
target_link_libraries(pi_panel PRIVATE pi_telemetry_qt)
target_compile_features(pi_panel PRIVATE cxx_std_20)
set_target_properties(pi_panel PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(pi_panel PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Unit Testing with GoogleTest
if(BUILD_TESTING)
add_executable(telemetry_qml_tests tests/TelemetryQmlTests.cpp)
set_target_properties(telemetry_qml_tests PROPERTIES AUTOMOC ON)
target_compile_definitions(telemetry_qml_tests PRIVATE PI_PANEL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(telemetry_qml_tests PRIVATE pi_telemetry_qt Qt6::Gui Qt6::Qml Qt6::Quick)
add_test(NAME telemetry_qml COMMAND telemetry_qml_tests)
set_tests_properties(telemetry_qml PROPERTIES TIMEOUT 15
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding parent project compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_executable(pi_panel_tests
    tests/LedControllerTests.cpp
    tests/ButtonDebouncerTests.cpp
    tests/ThermistorTests.cpp
)
target_link_libraries(pi_panel_tests PRIVATE
    pi_panel_core
    GTest::gtest_main
)
target_compile_features(pi_panel_tests PRIVATE cxx_std_20)
set_target_properties(pi_panel_tests PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(pi_panel_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

include(GoogleTest)
gtest_discover_tests(pi_panel_tests)
endif()
endif()
