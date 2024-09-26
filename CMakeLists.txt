cmake_minimum_required(VERSION 3.14)
project(libndtp)

set(CMAKE_CXX_STANDARD 17)

# Find Protobuf
find_package(Protobuf REQUIRED)

# Find Python
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Fetch pybind11
include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.10.4  # Specify a version tag
)
FetchContent_MakeAvailable(pybind11)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/synapse-api)
include_directories(${Protobuf_INCLUDE_DIRS})

# Add the extension module
pybind11_add_module(libndtp ndtp.cpp)

# Link against Protobuf
target_link_libraries(libndtp PRIVATE ${Protobuf_LIBRARIES})