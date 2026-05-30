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

# 1. Define the abstract path variable for the generated code target
set(GENERATED_LEXER_SRC "${CMAKE_CURRENT_BINARY_DIR}/generated_lexer/MyLexer.cpp")

# 2. Bind the AOT compiler generation step directly into the build graph dependency tree
add_custom_command(
    OUTPUT ${GENERATED_LEXER_SRC}
    COMMAND metalyzer_app ${CMAKE_CURRENT_SOURCE_DIR}/tests/specs/calculator.mz
    DEPENDS metalyzer_app
    COMMENT "Automated AOT Generation: Processing grammar spec into user_code::MyLexer"
)

# 3. Compile the generated source executable target under your preferred name
add_executable(run_lexer ${GENERATED_LEXER_SRC})

# 4. Bind include paths for header lookups
target_include_directories(run_lexer PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/generated_lexer
)

# =========================================================================
# AUTOMATED TESTING SUITE & INTEGRATION PASSES
# =========================================================================
include(CTest)
enable_testing()

if(BUILD_TESTING)
    # Block D Integration Test: Assert that the generation binary runs and outputs the profile JSON
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
# PERFORMANCE BENCHMARKING ENGINE LABS
# =========================================================================

# Target A: Register the custom Metalyzer benchmark target
add_executable(bench_calculator 
    benchmarks/bench_calculator.cpp
)

target_include_directories(bench_calculator PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/generated_lexer
)

if(NOT MSVC)
    target_compile_options(bench_calculator PRIVATE -O3 -march=native)
endif()

# Target B: Locate and bind the Industry-Standard Flex baseline target
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
