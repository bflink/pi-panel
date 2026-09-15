cmake_minimum_required(VERSION 3.20)
project(pi_panel VERSION 1.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

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
    Controllers/SystemTemperatureReader.cpp
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
    QML_FILES Main.qml
)

target_link_libraries(pi_panel PRIVATE
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)
target_link_libraries(pi_panel PRIVATE Qt6::Core)
target_link_libraries(pi_panel PRIVATE pi_panel_core)
target_compile_features(pi_panel PRIVATE cxx_std_20)
set_target_properties(pi_panel PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(pi_panel PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Unit Testing with GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding parent project compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(pi_panel_tests
    tests/LedControllerTests.cpp
    tests/ButtonDebouncerTests.cpp
    tests/SystemTemperatureReaderTests.cpp
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