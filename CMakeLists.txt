cmake_minimum_required(VERSION 3.16)
project(FastSafeStrings
    VERSION 2.3.0
    DESCRIPTION "High-Performance, Zero-Allocation, Memory-Safe Fixed-Capacity String Library and Variable-Block I/O Engine"
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(FSS_BUILD_TESTS "Build FastSafeStrings unit test suite" ON)
option(FSS_BUILD_BENCHMARKS "Build FastSafeStrings benchmark suite" ON)
option(FSS_ENABLE_NATIVE_TUNING "Enable compiler architecture-specific optimizations (AVX2 / native)" ON)
option(FSS_STRICT_WARNINGS "Treat compiler warnings as errors where appropriate" OFF)

# ---------------------------------------------------------------------------
# C / C++ Standards Configuration
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Compiler Flags & Optimization
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    add_compile_options(/W4 /permissive-)
    if(FSS_STRICT_WARNINGS)
        add_compile_options(/WX)
    endif()
    if(FSS_ENABLE_NATIVE_TUNING)
        # Enable intrinsic expansions and standard optimization
        add_compile_options(/Oi /O2)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            add_compile_options(/arch:AVX2)
        endif()
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(FSS_STRICT_WARNINGS)
        add_compile_options(-Werror)
    endif()
    if(FSS_ENABLE_NATIVE_TUNING)
        add_compile_options(-O3)
        include(CheckCCompilerFlag)
        check_c_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
        if(COMPILER_SUPPORTS_MARCH_NATIVE)
            add_compile_options(-march=native)
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# Library Targets
# ---------------------------------------------------------------------------

# 1. C Core String Engine (Header-Only INTERFACE Library)
add_library(faststr INTERFACE)
target_include_directories(faststr INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(faststr INTERFACE c_std_99)

# 2. C++ Modern String Engine (Header-Only INTERFACE Library)
add_library(fast_string_cpp INTERFACE)
target_include_directories(fast_string_cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(fast_string_cpp INTERFACE cxx_std_17)
target_link_libraries(fast_string_cpp INTERFACE faststr)

# 3. Variable-Block Record I/O Engine (Static Library)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/vbx_io.c")
    set(VBX_IO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/vbx_io.c")
else()
    set(VBX_IO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/vbx_io.c")
endif()

add_library(vbx_io STATIC ${VBX_IO_SOURCE})
target_link_libraries(vbx_io PUBLIC faststr)
target_include_directories(vbx_io PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------------------
# Unit Tests
# ---------------------------------------------------------------------------
if(FSS_BUILD_TESTS)
    enable_testing()

    # C Unit Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_faststr.c")
        add_executable(test_faststr tests/test_faststr.c)
        target_link_libraries(test_faststr PRIVATE faststr vbx_io)
        add_test(NAME test_faststr COMMAND test_faststr)
    endif()

    # C++ Unit Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_fast_string.cpp")
        add_executable(test_fast_string tests/test_fast_string.cpp)
        target_link_libraries(test_fast_string PRIVATE fast_string_cpp faststr)
        add_test(NAME test_fast_string COMMAND test_fast_string)
    endif()

    # VBX Record I/O Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_vbx_io.c")
        add_executable(test_vbx_io tests/test_vbx_io.c)
        target_link_libraries(test_vbx_io PRIVATE vbx_io faststr)
        add_test(NAME test_vbx_io COMMAND test_vbx_io)
    endif()
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
if(FSS_BUILD_BENCHMARKS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_c.c")
        add_executable(bench_c bench/bench_c.c)
        target_link_libraries(bench_c PRIVATE faststr vbx_io)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_cpp.cpp")
        add_executable(bench_cpp bench/bench_cpp.cpp)
        target_link_libraries(bench_cpp PRIVATE fast_string_cpp faststr)
    endif()
endif()

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS faststr fast_string_cpp vbx_io
    EXPORT FastSafeStringsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
