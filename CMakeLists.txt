cmake_minimum_required(VERSION 3.15)
project(PunkDither)

set(CMAKE_CXX_STANDARD 17)

# Enable OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found!")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Set After Effects SDK Path
set(AE_SDK_PATH ${CMAKE_SOURCE_DIR})  # Ensure correct root path

# Add include directories
include_directories(
    ${AE_SDK_PATH}/AfterEffectsSDK/Examples/Headers
    ${AE_SDK_PATH}/AfterEffectsSDK/Examples/Util
    ${AE_SDK_PATH}/AfterEffectsSDK/Examples/Headers/SP
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
