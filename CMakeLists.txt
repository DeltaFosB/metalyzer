cmake_minimum_required(VERSION 3.15)
project(Metalyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# User-controlled switch to toggle compile-time introspection metrics
option(ENABLE_METALYZER_PROFILING "Activate toolchain static profiling and instrumentation telemetry" ON)

file(GLOB LEXER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/*.cpp")

add_library(metalyzer_lexer ${LEXER_SOURCES})

target_include_directories(metalyzer_lexer PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Conditional compilation driver assignment based on optimization profiling state
if(ENABLE_METALYZER_PROFILING)
    target_compile_definitions(metalyzer_lexer PRIVATE ENABLE_PROFILING)
endif()

add_executable(metalyzer_app main.cpp)

target_link_libraries(metalyzer_app PRIVATE metalyzer_lexer)

if(MSVC)
    target_compile_options(metalyzer_lexer PRIVATE /W4)
    target_compile_options(metalyzer_app PRIVATE /W4)
else()
    target_compile_options(metalyzer_lexer PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(metalyzer_app PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =========================================================================
# AUTOMATED AOT MULTI-GRAMMAR CODE GENERATION MATRIX
# =========================================================================
set(GEN_ROOT "${CMAKE_CURRENT_BINARY_DIR}/generated_grammars")

# Target the precise generated_lexer subfolder layout dropped by your app
set(CALC_GEN_SRC    "${GEN_ROOT}/calculator/generated_lexer/MyLexer.cpp")
set(JSON_GEN_SRC    "${GEN_ROOT}/json_core/generated_lexer/MyLexer.cpp")
set(CSUB_GEN_SRC    "${GEN_ROOT}/c_subset/generated_lexer/MyLexer.cpp")

# 1. Calculator AOT generation path
add_custom_command(
    OUTPUT ${CALC_GEN_SRC} "${GEN_ROOT}/calculator/generated_lexer/MyLexer.hpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_ROOT}/calculator"
    COMMAND cd "${GEN_ROOT}/calculator" && ${CMAKE_CURRENT_BINARY_DIR}/metalyzer_app ${CMAKE_CURRENT_SOURCE_DIR}/tests/specs/calculator.mz
    DEPENDS metalyzer_app
    COMMENT "Automated AOT Generation: Processing calculator grammar spec"
)

# 2. JSON Core AOT generation path
add_custom_command(
    OUTPUT ${JSON_GEN_SRC} "${GEN_ROOT}/json_core/generated_lexer/MyLexer.hpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_ROOT}/json_core"
    COMMAND cd "${GEN_ROOT}/json_core" && ${CMAKE_CURRENT_BINARY_DIR}/metalyzer_app ${CMAKE_CURRENT_SOURCE_DIR}/tests/specs/json_core.mz
    DEPENDS metalyzer_app
    COMMENT "Automated AOT Generation: Processing JSON Core grammar spec"
)

# 3. C Subset AOT generation path
add_custom_command(
    OUTPUT ${CSUB_GEN_SRC} "${GEN_ROOT}/c_subset/generated_lexer/MyLexer.hpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_ROOT}/c_subset"
    COMMAND cd "${GEN_ROOT}/c_subset" && ${CMAKE_CURRENT_BINARY_DIR}/metalyzer_app ${CMAKE_CURRENT_SOURCE_DIR}/tests/specs/c_subset.mz
    DEPENDS metalyzer_app
    COMMENT "Automated AOT Generation: Processing C Subset grammar spec"
)

# =========================================================================
# MODULAR GENERATED OBJECT ARCHIVE LIBRARIES (Isolating namespaces)
# =========================================================================
# Enforced critical cross-target dependencies to serialize parallel graph steps
add_library(gen_lexer_calc_lib_bench STATIC ${CALC_GEN_SRC})
add_dependencies(gen_lexer_calc_lib_bench metalyzer_app)
target_compile_definitions(gen_lexer_calc_lib_bench PRIVATE main=calc_lexer_main)
target_include_directories(gen_lexer_calc_lib_bench PUBLIC "${GEN_ROOT}/calculator/generated_lexer")

add_library(gen_lexer_json_lib_bench STATIC ${JSON_GEN_SRC})
add_dependencies(gen_lexer_json_lib_bench metalyzer_app)
target_compile_definitions(gen_lexer_json_lib_bench PRIVATE main=json_lexer_main)
target_include_directories(gen_lexer_json_lib_bench PUBLIC "${GEN_ROOT}/json_core/generated_lexer")

add_library(gen_lexer_csub_lib_bench STATIC ${CSUB_GEN_SRC})
add_dependencies(gen_lexer_csub_lib_bench metalyzer_app)
target_compile_definitions(gen_lexer_csub_lib_bench PRIVATE main=csub_lexer_main)
target_include_directories(gen_lexer_csub_lib_bench PUBLIC "${GEN_ROOT}/c_subset/generated_lexer")

if(NOT MSVC)
    target_compile_options(gen_lexer_calc_lib_bench PRIVATE -O3 -march=native)
    target_compile_options(gen_lexer_json_lib_bench PRIVATE -O3 -march=native)
    target_compile_options(gen_lexer_csub_lib_bench PRIVATE -O3 -march=native)
endif()

# =========================================================================
# DECOUPLED BENCHMARKING ENGINE: 9-POINT CONCURRENT SUITE
# =========================================================================
add_executable(metalyzer_bench 
    benchmarks/metalyzer_bench_throughput.cpp
    benchmarks/metalyzer_bench_calculator.cpp
    benchmarks/metalyzer_bench_json.cpp
    benchmarks/metalyzer_bench_c_subset.cpp
)

# Link cleanly against all 3 isolated unique benchmark library targets
target_link_libraries(metalyzer_bench PRIVATE 
    gen_lexer_calc_lib_bench
    gen_lexer_json_lib_bench
    gen_lexer_csub_lib_bench
)

# Bind individual target translation units to their respective header folder paths
set_source_files_properties(benchmarks/metalyzer_bench_calculator.cpp PROPERTIES
    COMPILE_FLAGS "-I${GEN_ROOT}/calculator/generated_lexer"
)
set_source_files_properties(benchmarks/metalyzer_bench_json.cpp PROPERTIES
    COMPILE_FLAGS "-I${GEN_ROOT}/json_core/generated_lexer"
)
set_source_files_properties(benchmarks/metalyzer_bench_c_subset.cpp PROPERTIES
    COMPILE_FLAGS "-I${GEN_ROOT}/c_subset/generated_lexer"
)

target_include_directories(metalyzer_bench PRIVATE benchmarks)

if(NOT MSVC)
    target_compile_options(metalyzer_bench PRIVATE -O3 -march=native)
endif()

# =========================================================================
# AUTOMATED TESTING SUITE & INTEGRATION PASSES
# =========================================================================
include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_test(NAME ProfileJsonExists
        COMMAND metalyzer_app ${CMAKE_CURRENT_SOURCE_DIR}/tests/specs/calculator.mz
    )

    add_test(NAME AssertProfileCacheTier
        COMMAND ${CMAKE_COMMAND} -DPROFILE_DIR=${GEN_ROOT}/calculator -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyProfile.cmake"
    )

    set_tests_properties(AssertProfileCacheTier PROPERTIES 
        DEPENDS ProfileJsonExists
    )

    add_subdirectory(tests)

    add_test(NAME 9PointThroughputBenchmark
        COMMAND metalyzer_bench
    )
endif()

# =========================================================================
# INDUSTRY BASELINE TARGET: FLEX MULTI-GRAMMAR COMPARISON SANDBOX
# =========================================================================
find_package(FLEX REQUIRED)

if(FLEX_FOUND)
    set(FLEX_COMPARE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/flex_comparison")
    set(FLEX_OUT_ROOT   "${CMAKE_CURRENT_BINARY_DIR}/generated_flex")
    
    set(FLEX_CALC_CPP "${FLEX_OUT_ROOT}/calculator/lex.yy.cc")
    set(FLEX_JSON_CPP "${FLEX_OUT_ROOT}/json_core/lex.yy.cc")
    set(FLEX_CSUB_CPP "${FLEX_OUT_ROOT}/c_subset/lex.yy.cc")
    
    # 1. Flex Calculator engine compilation step (Using CalcPrefix)
    add_custom_command(
        OUTPUT ${FLEX_CALC_CPP}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${FLEX_OUT_ROOT}/calculator"
        COMMAND ${FLEX_EXECUTABLE} -PCalcSpec -o ${FLEX_CALC_CPP} ${FLEX_COMPARE_DIR}/calculator.l
        DEPENDS ${FLEX_COMPARE_DIR}/calculator.l
        COMMENT "Generating industry-standard Flex calculator scanner..."
    )
    
    # 2. Flex JSON Core engine compilation step (Using JsonPrefix)
    add_custom_command(
        OUTPUT ${FLEX_JSON_CPP}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${FLEX_OUT_ROOT}/json_core"
        COMMAND ${FLEX_EXECUTABLE} -PJsonSpec -o ${FLEX_JSON_CPP} ${FLEX_COMPARE_DIR}/json_core.l
        DEPENDS ${FLEX_COMPARE_DIR}/json_core.l
        COMMENT "Generating industry-standard Flex JSON scanner..."
    )

    # 3. Flex C Subset engine compilation step (Using CSubPrefix)
    add_custom_command(
        OUTPUT ${FLEX_CSUB_CPP}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${FLEX_OUT_ROOT}/c_subset"
        COMMAND ${FLEX_EXECUTABLE} -PCSubSpec -o ${FLEX_CSUB_CPP} ${FLEX_COMPARE_DIR}/c_subset.l
        DEPENDS ${FLEX_COMPARE_DIR}/c_subset.l
        COMMENT "Generating industry-standard Flex C Subset scanner..."
    )
    
    # Register the split multi-translation framework baseline binary
    add_executable(flex_bench
        benchmarks/flex_comparison/flex_bench_throughput.cpp
        benchmarks/flex_comparison/flex_bench_calculator.cpp
        benchmarks/flex_comparison/flex_bench_json.cpp
        benchmarks/flex_comparison/flex_bench_c_subset.cpp
        ${FLEX_CALC_CPP}
        ${FLEX_JSON_CPP}
        ${FLEX_CSUB_CPP}
    )
    
    # Bind translation unit lookups securely to prevent conflicting macro injections
    set_source_files_properties(benchmarks/flex_comparison/flex_bench_calculator.cpp PROPERTIES COMPILE_FLAGS "-I${FLEX_OUT_ROOT}/calculator")
    set_source_files_properties(benchmarks/flex_comparison/flex_bench_json.cpp PROPERTIES COMPILE_FLAGS "-I${FLEX_OUT_ROOT}/json_core")
    set_source_files_properties(benchmarks/flex_comparison/flex_bench_c_subset.cpp PROPERTIES COMPILE_FLAGS "-I${FLEX_OUT_ROOT}/c_subset")

    target_include_directories(flex_bench PRIVATE 
        benchmarks 
        ${FLEX_COMPARE_DIR}
    )
    
    if(NOT MSVC)
        target_compile_options(flex_bench PRIVATE -O3 -march=native)
    endif()

    if(BUILD_TESTING)
        add_test(NAME FlexIndustryBaselineBenchmark
            COMMAND flex_bench
        )
    endif()
endif()
