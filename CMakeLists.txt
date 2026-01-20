cmake_minimum_required(VERSION 3.15)
project(Metalyzer VERSION 1.0.0 LANGUAGES CXX)

# 1. Standard C++ Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 2. Define the Library (The Engine)
# This includes the ThompsonConstructor and NFA logic
add_library(metalyzer_lexer
    src/lexer/ThompsonConstructor.cpp
)

# 3. Handle Include Directories
# This allows you to use #include <metalyzer/lexer/NFA.hpp>
target_include_directories(metalyzer_lexer PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 4. Define the Executable (The Test/Main app)
add_executable(metalyzer_app main.cpp)

# 5. Link the Library to the Executable
target_link_libraries(metalyzer_app PRIVATE metalyzer_lexer)

# 6. Compiler Warnings (Helpful for debugging)
if(MSVC)
    target_compile_options(metalyzer_lexer PRIVATE /W4)
else()
    target_compile_options(metalyzer_lexer PRIVATE -Wall -Wextra -Wpedantic)
endif()
