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

# Define the abstract path variable for the generated code target
set(GENERATED_LEXER_SRC "${CMAKE_CURRENT_BINARY_DIR}/generated_lexer/MyLexer.cpp")

# Bind the AOT compiler generation step directly into the build graph dependency tree
add_custom_command(
    OUTPUT ${GENERATED_LEXER_SRC}
    COMMAND metalyzer_app ${CMAKE_CURRENT_SOURCE_DIR}/tests/specs/calculator.mz
    DEPENDS metalyzer_app
    COMMENT "Automated AOT Generation: Processing grammar spec into user_code::MyLexer"
)

# Compile the generated source executable target under your preferred name
add_executable(run_lexer ${GENERATED_LEXER_SRC})

# Bind include paths for header lookups
target_include_directories(run_lexer PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/generated_lexer
)

# =========================================================================
# AUTOMATED TESTING SUITE & INTEGRATION PASSES
# =========================================================================
include(CTest)
enable_testing()

if(BUILD_TESTING)
    # Assert that the generation binary runs and outputs the profile JSON
    add_test(NAME ProfileJsonExists
        COMMAND metalyzer_app ${CMAKE_CURRENT_SOURCE_DIR}/tests/specs/calculator.mz
    )

    # Cross-Platform Validation Script: Native check pointing directly to your cmake/ folder
    add_test(NAME AssertProfileCacheTier
        COMMAND ${CMAKE_COMMAND} -DPROFILE_DIR=${CMAKE_CURRENT_BINARY_DIR}/generated_lexer -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyProfile.cmake"
    )

    # Strict Ordering Constraint: Ensure the validation pass never evaluates before the file has been emitted
    set_tests_properties(AssertProfileCacheTier PROPERTIES 
        DEPENDS ProfileJsonExists
    )

    # Process localized unit tests directory tree
    add_subdirectory(tests)
endif()

# =========================================================================
# MODULAR BENCHMARKING ENGINE & TEST AUTOMATION
# =========================================================================

# Define a CMake library target wrapping the generated source code outputs
add_library(generated_lexer_lib STATIC 
    ${GENERATED_LEXER_SRC}
)

# Prevent duplicate main definition by renaming it during library compilation
target_compile_definitions(generated_lexer_lib PRIVATE main=generated_lexer_main)

if(NOT MSVC)
    target_compile_options(generated_lexer_lib PRIVATE -O3 -march=native)
endif()

# Bind lookup directories for library compilation and client include passes
target_include_directories(generated_lexer_lib PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}/generated_lexer
)

# Register the throughput benchmark binary executable
add_executable(metalyzer_bench 
    benchmarks/bench_throughput.cpp
)

# Link cleanly against the modular generated library target module
target_link_libraries(metalyzer_bench PRIVATE 
    generated_lexer_lib
)

# Maximize compilation runtime execution optimization settings
if(NOT MSVC)
    target_compile_options(metalyzer_bench PRIVATE -O3 -march=native)
endif()

# Register the automated tracking validation test under the CTest profile harness
if(BUILD_TESTING)
    add_test(NAME ThroughputBenchmark
        COMMAND metalyzer_bench
    )
endif()

# =========================================================================
# INDUSTRY BASELINE TARGET: FLEX COMPARISON SANDBOX
# =========================================================================
find_package(FLEX REQUIRED)

if(FLEX_FOUND)
    set(FLEX_COMPARE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/flex_comparison")
    set(FLEX_OUTPUT_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated_flex/lex.yy.cc")
    
    # Custom step to command Flex to generate the C++ state machine code array
    add_custom_command(
        OUTPUT ${FLEX_OUTPUT_CPP}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated_flex"
        COMMAND ${FLEX_EXECUTABLE} -o ${FLEX_OUTPUT_CPP} ${FLEX_COMPARE_DIR}/lex_calculator.l
        DEPENDS ${FLEX_COMPARE_DIR}/lex_calculator.l
        COMMENT "Generating industry-standard Flex scanner implementation source..."
    )
    
    # Compile the standalone Flex high-resolution hardware execution clock binary
    add_executable(bench_flex 
        benchmarks/flex_comparison/bench_flex.cpp
        ${FLEX_OUTPUT_CPP}
    )
    
    target_include_directories(bench_flex PRIVATE 
        ${FLEX_COMPARE_DIR}
    )
    
    if(NOT MSVC)
        target_compile_options(bench_flex PRIVATE -O3 -march=native)
    endif()
endif()
