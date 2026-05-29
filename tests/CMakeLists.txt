cmake_minimum_required(VERSION 3.15)

# Expose our developer update toggle switch
set(UPDATE_SNAPSHOTS OFF CACHE BOOL "Overwrites `.snap` files with live test log structures when enabled.")

# Define the testing macro loop
macro(register_metalyzer_snapshot_test TEST_NAME BASE_FILENAME)
    add_test(
        NAME metalyzer_${TEST_NAME}_snapshot
        COMMAND "${CMAKE_COMMAND}"
            -DENGINE_EXE=$<TARGET_FILE:metalyzer_app>
            -DSPEC_FILE=${CMAKE_CURRENT_SOURCE_DIR}/specs/${BASE_FILENAME}.mz
            -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/inputs/${BASE_FILENAME}.in
            -DSNAPSHOT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/snapshots/${BASE_FILENAME}.snap
            -DUPDATE_SNAPSHOTS=${UPDATE_SNAPSHOTS}
            -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/sandbox_${TEST_NAME}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -P ${CMAKE_SOURCE_DIR}/cmake/RunSnapshotTest.cmake
    )
endmacro()

# Connect the spec matrix files
register_metalyzer_snapshot_test(calculator calculator)
register_metalyzer_snapshot_test(json_core json_core)
