cmake_minimum_required(VERSION 3.15)
project(PunkDither)

set(CMAKE_CXX_STANDARD 17)

# Ensure AE_SDK_PATH is set
if (NOT DEFINED ENV{AE_SDK_PATH})
    message(FATAL_ERROR "AE_SDK_PATH is not set. Make sure the After Effects SDK is downloaded.")
endif()

set(AE_SDK_PATH $ENV{AE_SDK_PATH})

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



