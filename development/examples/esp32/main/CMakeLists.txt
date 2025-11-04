# Flexible main component for different TLE92466ED app types
# Determine source file based on APP_TYPE

# Get app type from parent
if(NOT DEFINED APP_TYPE)
    set(APP_TYPE "basic_usage")
endif()

# Debug information
message(STATUS "=== TLE92466ED CMake Debug Information ===")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "APP_TYPE: ${APP_TYPE}")
message(STATUS "==========================================")

# Get source file from centralized configuration
execute_process(
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/get_app_info.py" validate "${APP_TYPE}"
    RESULT_VARIABLE VALIDATE_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)

if(NOT VALIDATE_RESULT EQUAL 0)
    message(FATAL_ERROR "Unknown APP_TYPE: ${APP_TYPE}")
endif()

execute_process(
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/get_app_info.py" source_file "${APP_TYPE}"
    OUTPUT_VARIABLE MAIN_SOURCE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE SOURCE_RESULT
)

if(NOT SOURCE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to get source file for APP_TYPE: ${APP_TYPE}")
endif()

message(STATUS "Using source file from configuration: ${MAIN_SOURCE}")

# Check if source file exists
set(SOURCE_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${MAIN_SOURCE}")
message(STATUS "Looking for source file: ${SOURCE_FILE_PATH}")

# List files in current directory for debugging
file(GLOB DEBUG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*")
message(STATUS "Files in source directory:")
foreach(DEBUG_FILE ${DEBUG_FILES})
    message(STATUS "  ${DEBUG_FILE}")
endforeach()

if(NOT EXISTS "${SOURCE_FILE_PATH}")
    # Try alternative paths for CI environment
    set(SOURCE_FILE_PATH "${CMAKE_CURRENT_LIST_DIR}/${MAIN_SOURCE}")
    message(STATUS "Trying alternative path: ${SOURCE_FILE_PATH}")
    if(NOT EXISTS "${SOURCE_FILE_PATH}")
        message(FATAL_ERROR "Source file not found: ${MAIN_SOURCE}\n"
                            "Tried paths:\n"
                            "  ${CMAKE_CURRENT_SOURCE_DIR}/${MAIN_SOURCE}\n"
                            "  ${CMAKE_CURRENT_LIST_DIR}/${MAIN_SOURCE}\n"
                            "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}\n"
                            "CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
    endif()
endif()

message(STATUS "Using source file: ${SOURCE_FILE_PATH}")

# Determine include paths based on environment
# In CI, include is copied to ../include, in development it's at ../../include or ../../../include
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../include")
    # CI environment - include is at project level
    set(INC_BASE_PATH "../include")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
    # Development environment - include is at examples level
    set(INC_BASE_PATH "../../include")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../include")
    # Development environment - include is at driver root
    set(INC_BASE_PATH "../../../include")
else()
    message(FATAL_ERROR "Cannot find include directory. Checked ../include, ../../include, and ../../../include")
endif()

message(STATUS "Using include base path: ${INC_BASE_PATH}")

idf_component_register(
    SRCS "${MAIN_SOURCE}" "ESP32C6_HAL.cpp"
    INCLUDE_DIRS "." "${INC_BASE_PATH}"
    REQUIRES driver esp_timer freertos spi_flash tle92466ed-espidf
)

# Set C++ standard (TLE92466ED requires C++23)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_23)

# Set compiler flags based on build type
if(BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O0
        -g3
        -DDEBUG
        -DTLE92466ED_DEBUG
    )
else()
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O2
        -g
        -DNDEBUG
        -DTLE92466ED_RELEASE
    )
endif()

# Add compile definitions for each example type
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    "EXAMPLE_TYPE_${APP_TYPE}=1"
    "TLE92466ED_ESP32_C6=1"
)

# ESP32-C6 specific optimizations
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    "CONFIG_IDF_TARGET_ESP32C6=1"
)
