cmake_minimum_required(VERSION 3.14)
project(nysys VERSION 0.5.0 LANGUAGES CXX C)

include(FetchContent)

# Fetch nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Config
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
if(MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus /utf-8 /wd4996 /GS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Runtime flags
if(MSVC)
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /GL")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -ffunction-sections -fdata-sections")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")
endif()

# Source
set(SOURCES
    src/nysys.cpp
    src/helper/json_structure.cpp
    src/helper/wmi_helper.cpp
    src/main/gpu_info.cpp
    src/main/motherboard_info.cpp
    src/main/cpu_info.cpp
    src/main/memory_info.cpp
    src/main/storage_info.cpp
    src/main/network_info.cpp
    src/main/audio_info.cpp
    src/main/battery_info.cpp
    src/main/monitor_info.cpp
    src/nysys.rc
    src/nysys.def
)

# Library target
if(MSVC)
    set(DEF_FILE "${CMAKE_SOURCE_DIR}/src/nysys.def")
    add_library(nysys SHARED ${SOURCES} ${DEF_FILE})
else()
    add_library(nysys SHARED ${SOURCES})
endif()

target_compile_definitions(nysys PRIVATE NYSYS_EXPORTS)

# Include
target_include_directories(nysys
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include/nysys
)

# Build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(nysys PRIVATE NDEBUG)
endif()

# Target
set_target_properties( PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

if(MSVC)
    set_target_properties(nysys PROPERTIES
        LINK_SEARCH_START_STATIC ON
        LINK_FLAGS "/DEF:${DEF_FILE}"
    )
endif()

# Lib
target_link_libraries(nysys
    dxgi
    d3d11
    wbemuuid
    oleaut32
    ole32
    pdh
    iphlpapi
    setupapi
    nlohmann_json::nlohmann_json
)

# Copy header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/nysys/nysys.h
    ${CMAKE_BINARY_DIR}/include/nysys.h
    COPYONLY
)

# Example (C++)
add_executable(example_cpp examples/example_cpp.cpp)
target_link_libraries(example_cpp nysys)
set_target_properties(example_cpp PROPERTIES CXX_STANDARD 17)

# Example (C)
add_executable(example_c examples/example_c.c)
target_link_libraries(example_c nysys)

# Copy DLL
foreach(APP example_cpp example_c)
    add_custom_command(TARGET ${APP} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:nysys>
        $<TARGET_FILE_DIR:${APP}>/$<TARGET_FILE_NAME:nysys>
    )
endforeach()

# Installation
install(TARGETS nysys example_cpp example_c
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES
    include/nysys/nysys.hpp
    ${CMAKE_BINARY_DIR}/include/nysys/nysys.h
    DESTINATION include
)

install(FILES
    src/nysys.def
    DESTINATION lib/nysys
)
