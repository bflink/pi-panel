# Prefer vcpkg's config on Windows; Debian ships CMake's FindProtobuf workflow.
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()
find_package(gRPC CONFIG QUIET)
if(NOT gRPC_FOUND)
    # Some Debian/Ubuntu packages expose pkg-config files but no CMake config.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GRPC REQUIRED IMPORTED_TARGET grpc++)
    add_library(gRPC::grpc++ ALIAS PkgConfig::GRPC)
endif()
if(NOT TARGET gRPC::grpc_cpp_plugin)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
    add_executable(gRPC::grpc_cpp_plugin IMPORTED)
    set_target_properties(gRPC::grpc_cpp_plugin PROPERTIES IMPORTED_LOCATION "${GRPC_CPP_PLUGIN}")
endif()
find_package(Threads REQUIRED)

set(TELEMETRY_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(TELEMETRY_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${TELEMETRY_GENERATED_DIR}")
add_custom_command(
    OUTPUT "${TELEMETRY_GENERATED_DIR}/telemetry.pb.cc"
           "${TELEMETRY_GENERATED_DIR}/telemetry.pb.h"
           "${TELEMETRY_GENERATED_DIR}/telemetry.grpc.pb.cc"
           "${TELEMETRY_GENERATED_DIR}/telemetry.grpc.pb.h"
    COMMAND protobuf::protoc
    ARGS "--proto_path=${TELEMETRY_PROTO_DIR}"
         "--cpp_out=${TELEMETRY_GENERATED_DIR}"
         "--grpc_out=${TELEMETRY_GENERATED_DIR}"
         "--plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
         "${TELEMETRY_PROTO_DIR}/telemetry.proto"
    DEPENDS "${TELEMETRY_PROTO_DIR}/telemetry.proto" protobuf::protoc gRPC::grpc_cpp_plugin
    VERBATIM)
add_library(pi_telemetry_proto
    "${TELEMETRY_GENERATED_DIR}/telemetry.pb.cc"
    "${TELEMETRY_GENERATED_DIR}/telemetry.grpc.pb.cc")
target_include_directories(pi_telemetry_proto SYSTEM PUBLIC "${TELEMETRY_GENERATED_DIR}")
target_link_libraries(pi_telemetry_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)
target_compile_features(pi_telemetry_proto PUBLIC cxx_std_20)

add_library(pi_telemetry Networking/TelemetryClient.cpp)
target_include_directories(pi_telemetry PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(pi_telemetry PUBLIC pi_telemetry_proto Threads::Threads)
target_compile_features(pi_telemetry PUBLIC cxx_std_20)
set_target_properties(pi_telemetry PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(pi_telemetry PRIVATE /W4 /permissive-)
else()
    target_compile_options(pi_telemetry PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(telemetry_probe tools/TelemetryProbe.cpp)
target_link_libraries(telemetry_probe PRIVATE pi_telemetry)

if(BUILD_TESTING)
    add_executable(telemetry_client_tests tests/TelemetryClientTests.cpp)
    target_link_libraries(telemetry_client_tests PRIVATE pi_telemetry)
    add_test(NAME telemetry_client COMMAND telemetry_client_tests)
    set_tests_properties(telemetry_client PROPERTIES TIMEOUT 30)
endif()
