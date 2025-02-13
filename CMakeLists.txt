cmake_minimum_required(VERSION 3.15)
project(MyMacOSProject)

set(CMAKE_CXX_STANDARD 17)

set(AE_SDK_PATH $ENV{GITHUB_WORKSPACE}/AfterEffectsSDK)

add_executable(MyApp PunkDither.cpp)  # Change 'main.cpp' to your actual source file


