cmake_minimum_required(VERSION 3.15)
project(PunkDither)

set(CMAKE_CXX_STANDARD 17)

# Set After Effects SDK Path
set(AE_SDK_PATH ${CMAKE_SOURCE_DIR})  # Ensure correct root path

# Add include directories
include_directories(
    ${AE_SDK_PATH}/May2023_AfterEffectsSDK_Win/Examples/Headers
)

# Add source files
add_executable(PunkDither
    PunkDither.cpp
    PunkDither.h
    PunkDither_Strings.cpp
    PunkDither_Strings.h
)

# Link AE SDK libraries (macOS only)
if (APPLE)
    target_link_libraries(PunkDither "-framework Carbon")
endif()
