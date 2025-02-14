cmake_minimum_required(VERSION 3.15)
project(PunkDither)

set(CMAKE_CXX_STANDARD 17)

# Set After Effects SDK Path
set(AE_SDK_PATH ${CMAKE_SOURCE_DIR}/AfterEffectsSDK)

# Include AE SDK headers
include_directories(
    ${AE_SDK_PATH}/Examples/Headers
    ${AE_SDK_PATH}/Examples/Util
    ${AE_SDK_PATH}/Examples/Headers/SP
)

# Find OpenMP
find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
    message(STATUS "Found OpenMP: ${OpenMP_CXX_FLAGS}")
    add_compile_options(${OpenMP_CXX_FLAGS})
    add_link_options(${OpenMP_CXX_LIBRARIES})
endif()

# Add source files
add_executable(PunkDither
    PunkDither.cpp
    PunkDither.h
    PunkDither_Strings.cpp
    PunkDither_Strings.h
    ${AE_SDK_PATH}/Examples/Util/AEGP_SuiteHandler.cpp
    ${AE_SDK_PATH}/Examples/Util/entry.h
)

# Link After Effects SDK libraries
target_link_libraries(PunkDither 
    "-framework Carbon"
    OpenMP::OpenMP_CXX
)

