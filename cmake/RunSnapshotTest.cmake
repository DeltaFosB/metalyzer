# cmake/RunSnapshotTest.cmake
cmake_minimum_required(VERSION 3.15)

# 1. Sanity Check on Incoming Variables
if(NOT ENGINE_EXE OR NOT SPEC_FILE OR NOT INPUT_FILE OR NOT SNAPSHOT_FILE OR NOT BUILD_DIR)
    message(FATAL_ERROR "Missing vital toolchain parameters for the snapshot test driver.")
endif()

# Ensure our clean sandbox directory exists for this specific test case execution
file(MAKE_DIRECTORY "${BUILD_DIR}")

# 2. Phase 1: Code Generation Pass
message(STATUS "Phase 1: Compiling regular expressions from spec: ${SPEC_FILE}")
execute_process(
    COMMAND "${ENGINE_EXE}" "${SPEC_FILE}"
    WORKING_DIRECTORY "${BUILD_DIR}"
    RESULT_VARIABLE gen_result
)

if(NOT gen_result EQUAL 0)
    message(FATAL_ERROR "Automata generation failed for specification ${SPEC_FILE}")
endif()

# 3. Phase 2: Toolchain Compilation Pass
message(STATUS "Phase 2: Invoking host compiler to emit binary harness...")
# We dynamically locate the host compiler used to build the main project
execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" "-std=c++17" "-O3" "generated_lexer/MyLexer.cpp" -I "generated_lexer" 
    WORKING_DIRECTORY "${BUILD_DIR}"
    RESULT_VARIABLE compile_result
)

if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "Host compiler failed to compile the dynamically generated C++ code target.")
endif()

# Determine binary naming convention based on platform architecture
if(WIN32)
    set(TEST_EXEC "${BUILD_DIR}/a.exe")
else()
    set(TEST_EXEC "${BUILD_DIR}/a.out")
endif()

# 4. Phase 3: Input Piping & Stream Execution Pass
message(STATUS "Phase 3: Processing input streams via standard input redirection...")
execute_process(
    COMMAND "${TEST_EXEC}"
    INPUT_FILE "${INPUT_FILE}"
    OUTPUT_FILE "${BUILD_DIR}/actual_output.log"
    RESULT_VARIABLE exec_result
)

if(NOT exec_result EQUAL 0)
    message(FATAL_ERROR "The integration binary execution crashed or emitted an unexpected exit vector.")
endif()

# 5. Phase 4: Validation or Update Pass
if(UPDATE_SNAPSHOTS)
    message(STATUS "Regeneration Mode: Overwriting baseline snapshot file on disk...")
    file(COPY "${BUILD_DIR}/actual_output.log" DESTINATION "${BUILD_DIR}/tmp_snap_dir")
    file(RENAME "${BUILD_DIR}/tmp_snap_dir/actual_output.log" "${SNAPSHOT_FILE}")
    message(STATUS "Successfully updated ground truth snapshot baseline: ${SNAPSHOT_FILE}")
else()
    message(STATUS "Validation Mode: Executing byte-for-byte stream comparison matrix...")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${BUILD_DIR}/actual_output.log" "${SNAPSHOT_FILE}"
        RESULT_VARIABLE diff_result
    )
    
    if(NOT diff_result EQUAL 0)
        message(FATAL_ERROR "Snapshot Divergence Detected! The generated lexer output has regressed from the baseline .snap master template.")
    endif()
    message(STATUS "Test passed successfully.")
endif()
