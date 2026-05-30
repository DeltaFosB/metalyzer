# Native CMake Metadata Assertion Wrapper 
set(PROFILE_PATH "${PROFILE_DIR}/calculator_profile.json")

if(NOT EXISTS "${PROFILE_PATH}")
    message(FATAL_ERROR "Verification Error: Expected compile profile artifact was not found at target: ${PROFILE_PATH}")
endif()

# Ingest serialized raw file payload
file(READ "${PROFILE_PATH}" FILE_CONTENT)

# Execute non-null validation check tracking the generated cache assignment metric
if(FILE_CONTENT MATCHES "\"cache_tier\": *\"(L1|L2)\"")
    message(STATUS "Validation Success: Recognized matching structural cache tier indicator: '${CMAKE_MATCH_1}'")
else()
    message(FATAL_ERROR "Assertion Fault: The required metric property 'cache_tier' is absent or corrupted within the generated JSON.")
endif()
