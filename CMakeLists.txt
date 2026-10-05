cmake_minimum_required(VERSION 3.20)
project(example_device_plugin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(ENGINE_DIR "${PROJECT_SOURCE_DIR}/engine")
set(PLUGIN_MANIFEST_FILE "${PROJECT_SOURCE_DIR}/plugin.json")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${PLUGIN_MANIFEST_FILE}")

file(READ "${PLUGIN_MANIFEST_FILE}" PLUGIN_MANIFEST_JSON)
string(JSON ENGINE_PLUGIN_ID GET "${PLUGIN_MANIFEST_JSON}" id)
string(JSON MIN_ENGINE_VERSION GET "${PLUGIN_MANIFEST_JSON}" minEngineVersion)
string(JSON CONTAINS_ENGINE_PLUGIN GET "${PLUGIN_MANIFEST_JSON}" contains_engine_plugin)
if("${MIN_ENGINE_VERSION}" STREQUAL "")
    message(FATAL_ERROR "plugin.json must set minEngineVersion.")
endif()
if(NOT CONTAINS_ENGINE_PLUGIN)
    message(FATAL_ERROR "plugin.json must set contains_engine_plugin to true.")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" NORMALIZED_BUILD_TYPE)
set(ENGINE_PLUGIN_VARIANT "engine")
if(NORMALIZED_BUILD_TYPE STREQUAL "debug")
    set(ENGINE_PLUGIN_VARIANT "engine-debug")
endif()

if(CMAKE_CONFIGURATION_TYPES)
    set(ENGINE_PLUGIN_VARIANT "$<IF:$<CONFIG:Debug>,engine-debug,engine>")
endif()
set(ENGINE_PLUGIN_DIR "${PROJECT_SOURCE_DIR}/plugin/${ENGINE_PLUGIN_VARIANT}/${ENGINE_PLUGIN_ID}")

file(GLOB_RECURSE EXAMPLE_ENGINE_SOURCES CONFIGURE_DEPENDS "${ENGINE_DIR}/src/*.cpp")
add_library(example_device_plugin_engine SHARED ${EXAMPLE_ENGINE_SOURCES})

# Public, locked dependency snapshots: no private checkout is required.
if(NOT TARGET TEMPEST::Messaging)
    add_subdirectory(vendor/messaging)
endif()
if(NOT TARGET TEMPEST::Peer)
    add_subdirectory(vendor/peer)
endif()
if(NOT TARGET DARTWIC::EngineProtocol)
    add_subdirectory(vendor/engine-protocol)
endif()

file(GLOB ENGINE_INCLUDE_DIRS
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES true
        "${ENGINE_DIR}/include/*")

target_include_directories(example_device_plugin_engine PRIVATE
        "${ENGINE_DIR}/include"
        ${ENGINE_INCLUDE_DIRS})

find_package(nlohmann_json CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

target_link_libraries(example_device_plugin_engine PRIVATE
        nlohmann_json::nlohmann_json)
target_link_libraries(example_device_plugin_engine PRIVATE
        TEMPEST::Peer
        cppzmq
        DARTWIC::EngineProtocol)

set_target_properties(example_device_plugin_engine PROPERTIES
        OUTPUT_NAME "${ENGINE_PLUGIN_ID}"
        RUNTIME_OUTPUT_DIRECTORY "${ENGINE_PLUGIN_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${ENGINE_PLUGIN_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${ENGINE_PLUGIN_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${ENGINE_PLUGIN_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${ENGINE_PLUGIN_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${ENGINE_PLUGIN_DIR}/bin"
        PDB_OUTPUT_DIRECTORY "${ENGINE_PLUGIN_DIR}/bin"
        PDB_OUTPUT_DIRECTORY_DEBUG "${ENGINE_PLUGIN_DIR}/bin"
        PDB_OUTPUT_DIRECTORY_RELEASE "${ENGINE_PLUGIN_DIR}/bin")

set(ENGINE_PLUGIN_COPY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ENGINE_PLUGIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PLUGIN_MANIFEST_FILE}"
        "${ENGINE_PLUGIN_DIR}/plugin.json"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ENGINE_DIR}/src/module_config.json"
        "${ENGINE_PLUGIN_DIR}/module_config.json"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ENGINE_DIR}/src/default_parameters.json"
        "${ENGINE_PLUGIN_DIR}/default_parameters.json"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ENGINE_DIR}/src/peer_config.json"
        "${ENGINE_PLUGIN_DIR}/peer_config.json")

add_custom_target(example_device_plugin ALL
        ${ENGINE_PLUGIN_COPY_COMMANDS}
        DEPENDS example_device_plugin_engine
        COMMENT "Copying engine plugin files")

add_custom_target(copy_engine_plugin
        DEPENDS example_device_plugin)

message(STATUS "Example engine plugin dir: ${ENGINE_PLUGIN_DIR}")
message(STATUS "Example engine plugin variant: ${ENGINE_PLUGIN_VARIANT}")
message(STATUS "Using the bundled Engine SDK snapshot from engine/include/sdk.")

add_executable(rocket-flight-peer examples/flight-peer/main.cpp engine/src/example_transport.cpp)
target_include_directories(rocket-flight-peer PRIVATE engine/include)
target_link_libraries(rocket-flight-peer PRIVATE TEMPEST::Peer DARTWIC::EngineProtocol cppzmq nlohmann_json::nlohmann_json)
