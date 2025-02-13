cmake_minimum_required(VERSION 3.15)
project(PunkDither)

set(CMAKE_CXX_STANDARD 17)

set(AE_SDK_PATH ${CMAKE_SOURCE_DIR}/May2023_AfterEffectsSDK_Win)

# Add include directories
include_directories(${AE_SDK_PATH}/Headers)

# Add source files
add_executable(PunkDither
    PunkDither.cpp
    PunkDither.h
    PunkDither_Strings.cpp
    PunkDither_Strings.h
)

# Link AE SDK libraries (Modify this if needed)
target_link_libraries(PunkDither
    "-framework Carbon"  # Required for After Effects SDK
)



