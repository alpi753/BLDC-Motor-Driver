cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME demo_stm32f411)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

file(GLOB BLDC_SOURCES CONFIGURE_DEPENDS
    Core/Src/bldc/*.c
)

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    Core/Src/*.c
    Core/Src/bldc/*.c
)

set(core_compile_options
    -O3
    -Wall
    -Wextra
    # -Wpedantic
    -Wformat=2
    -Wno-unused-parameter
    -Wshadow
    -Wwrite-strings
    -Wold-style-definition
    -Wmissing-include-dirs
    # -Wstrict-prototypes
    # -Wredundant-decls
    # -Wnested-externs
)

set_source_files_properties(${CORE_SOURCES} PROPERTIES
    COMPILE_OPTIONS "${core_compile_options}"
)

# Add sources to executable
file(GLOB DSDL_SOURCES CONFIGURE_DEPENDS
    Middlewares/Third_Party/dsdl_generated/src/*.c
)

set(NON_CORE_SOURCES
    ${DSDL_SOURCES}
		Core/Src/syscalls.c
    Middlewares/Third_Party/NanoCBOR/src/decoder.c
    Middlewares/Third_Party/NanoCBOR/src/encoder.c
    Middlewares/Third_Party/libcanard/canard.c
)

set_source_files_properties(${NON_CORE_SOURCES} PROPERTIES
    COMPILE_OPTIONS -w
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
   ${BLDC_SOURCES}
   ${DSDL_SOURCES}
    Middlewares/Third_Party/NanoCBOR/src/decoder.c
    Middlewares/Third_Party/NanoCBOR/src/encoder.c
    Middlewares/Third_Party/libcanard/canard.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Middlewares/Third_Party/NanoCBOR/include
    Middlewares/Third_Party/libcanard
    Middlewares/Third_Party/dsdl_generated/include
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
