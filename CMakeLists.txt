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

include(CTest)
enable_testing()
add_subdirectory(tests)
