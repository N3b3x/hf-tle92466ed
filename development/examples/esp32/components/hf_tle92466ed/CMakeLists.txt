cmake_minimum_required(VERSION 3.16)

# Get the absolute path to the project root
# In CI, the files are copied to the build directory, so we can use relative paths from CMAKE_SOURCE_DIR
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)

# Check if we're in the CI environment (files copied to build directory)
if(EXISTS "${PROJECT_ROOT}/src" AND EXISTS "${PROJECT_ROOT}/inc")
    # We're in CI or local build with copied files
    set(SRC_ROOT "${PROJECT_ROOT}")
else()
    # We're in development with original structure
    get_filename_component(SRC_ROOT "${CMAKE_SOURCE_DIR}/../../" ABSOLUTE)
endif()

message(STATUS "TLE92466ED Driver - ESP32 Component Configuration")
message(STATUS "Project Root: ${PROJECT_ROOT}")
message(STATUS "Source Root: ${SRC_ROOT}")

# List all TLE92466ED driver source files
set(TLE92466ED_SRCS
    "${SRC_ROOT}/src/TLE92466ED.cpp"
)

# Check which source files actually exist and add them
set(EXISTING_SRCS "")
foreach(src_file ${TLE92466ED_SRCS})
    if(EXISTS "${src_file}")
        list(APPEND EXISTING_SRCS "${src_file}")
        message(STATUS "Found source: ${src_file}")
    else()
        message(WARNING "Source file not found: ${src_file}")
    endif()
endforeach()

idf_component_register(
    SRCS 
        ${EXISTING_SRCS}
        
    INCLUDE_DIRS 
        "${SRC_ROOT}/inc"
        
    REQUIRES 
        # ESP-IDF core components
        driver              # SPI drivers
        esp_timer           # Timing functions
        freertos            # RTOS support
        esp_system          # System utilities
        esp_hw_support      # Hardware support
        hal                 # Hardware abstraction
        soc                 # SoC definitions
        log                 # Logging
        
        # Driver components
        esp_driver_gpio
        esp_driver_spi
        
        # Additional components
        esp_common          # Common ESP32 utilities
        spi_flash           # SPI flash support
)

# Set C++ standard (TLE92466ED requires C++23)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_23)

# Add ESP32 family definitions based on target
if(CONFIG_IDF_TARGET_ESP32)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        TLE92466ED_TARGET_ESP32=1
        TLE92466ED_MCU_FAMILY_ESP32=1
        ESP32=1
    )
elseif(CONFIG_IDF_TARGET_ESP32C6)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        TLE92466ED_TARGET_ESP32C6=1
        TLE92466ED_MCU_FAMILY_ESP32=1
        ESP32C6=1
    )
elseif(CONFIG_IDF_TARGET_ESP32S3)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        TLE92466ED_TARGET_ESP32S3=1
        TLE92466ED_MCU_FAMILY_ESP32=1
        ESP32S3=1
    )
elseif(CONFIG_IDF_TARGET_ESP32C3)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        TLE92466ED_TARGET_ESP32C3=1
        TLE92466ED_MCU_FAMILY_ESP32=1
        ESP32C3=1
    )
else()
    # Default to ESP32
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        TLE92466ED_TARGET_ESP32=1
        TLE92466ED_MCU_FAMILY_ESP32=1
        ESP32=1
    )
endif()

# Compiler flags for optimization
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
    -g
    -ffunction-sections
    -fdata-sections
)

# Add specific flags for C++ files
target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++23>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=pedantic>
)

# Link time optimization for better performance
if(CONFIG_COMPILER_OPTIMIZATION_SIZE OR CONFIG_COMPILER_OPTIMIZATION_PERF)
    target_compile_options(${COMPONENT_LIB} PRIVATE -flto)
    target_link_options(${COMPONENT_LIB} PRIVATE -flto)
endif()

# Final status messages
list(LENGTH EXISTING_SRCS NUM_SOURCES)
message(STATUS "=== TLE92466ED Driver Component Summary ===")
message(STATUS "Component configured successfully")
message(STATUS "Included ${NUM_SOURCES} source files")
if(CONFIG_IDF_TARGET_ESP32)
    message(STATUS "Target MCU: ESP32")
elseif(CONFIG_IDF_TARGET_ESP32C6)
    message(STATUS "Target MCU: ESP32-C6")
elseif(CONFIG_IDF_TARGET_ESP32S3)
    message(STATUS "Target MCU: ESP32-S3")
elseif(CONFIG_IDF_TARGET_ESP32C3)
    message(STATUS "Target MCU: ESP32-C3")
else()
    message(STATUS "Target MCU: ESP32 (default)")
endif()
message(STATUS "C++ Standard: C++23")
message(STATUS "Driver Version: 2.0.0")
message(STATUS "Optimization: LTO enabled (if configured)")
message(STATUS "==========================================")

