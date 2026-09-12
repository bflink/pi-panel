cmake_minimum_required(VERSION 3.20)
project(pi_panel VERSION 1.0 LANGUAGES CXX)

add_executable(pi_panel main.cpp Controllers/LedController.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiodcxx)

target_link_libraries(pi_panel PRIVATE PkgConfig::GPIOD)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

target_compile_features(pi_panel PRIVATE cxx_std_20)
set_target_properties(pi_panel PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(pi_panel PRIVATE -Wall -Wextra -Wpedantic -Werror)