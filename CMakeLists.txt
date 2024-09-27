cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0135 NEW)

# Use dynamic runtime library (/MD)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

project(libndtp VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include FetchContent module
include(FetchContent)

# ======================================
# Fetch and Build Abseil
# ======================================

FetchContent_Declare(
    absl
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG        20230125.3  # Use a stable release. Adjust as needed.
    CMAKE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
        -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY}
        -DABSL_BUILD_TESTING=OFF
        -DABSL_ENABLE_INSTALL=ON       # Enable installation
        -DBUILD_SHARED_LIBS=ON          # Build shared libraries
        -DABSL_USE_STATIC_MSVC_RUNTIME=OFF  # Use dynamic runtime
)
FetchContent_MakeAvailable(absl)

# ======================================
# Fetch and Build Protobuf
# ======================================

# Define Protobuf version and source
set(PROTOBUF_VERSION "28.2")
set(PROTOBUF_URL "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v${PROTOBUF_VERSION}.tar.gz")
set(PROTOBUF_SHA256 "b2340aa47faf7ef10a0328190319d3f3bee1b24f426d4ce8f4253b6f27ce16db")

FetchContent_Declare(
    protobuf
    GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
    GIT_TAG        v${PROTOBUF_VERSION}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/protobuf-install
        -Dprotobuf_BUILD_SHARED_LIBS=ON
        -Dprotobuf_MSVC_STATIC_RUNTIME=OFF
        -Dprotobuf_WITH_ZLIB=OFF
        -Dprotobuf_BUILD_TESTS=OFF
        -Dprotobuf_BUILD_EXAMPLES=OFF
        -Dprotobuf_BUILD_PROTOC_BINARIES=ON
        -DBUILD_TESTING=OFF
        -Dprotobuf_ABSL_PROVIDER=package  # Use external Abseil
        -Dabsl_DIR=${absl_BINARY_DIR}/absl
        -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY}
        -Dprotobuf_USE_STATIC_MSVC_RUNTIME=OFF  # Use dynamic runtime
)
FetchContent_MakeAvailable(protobuf)

# Set the path to the protoc executable
set(PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)

# ======================================
# Fetch and Configure Pybind11
# ======================================

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6
    CMAKE_ARGS
        -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}  # Ensure consistency
)
FetchContent_MakeAvailable(pybind11)

# ======================================
# Generate Protobuf Sources
# ======================================

# Collect all .proto files recursively
file(GLOB_RECURSE PROTO_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/synapse-api/*.proto"
    "${CMAKE_CURRENT_SOURCE_DIR}/synapse-api/**/*.proto"
)

# Function to compile .proto files to C++ sources
set(GENERATED_PROTO_SRCS)
set(GENERATED_PROTO_HDRS)
foreach(proto_file ${PROTO_FILES})
    # Get the relative path of the .proto file from the source directory
    file(RELATIVE_PATH proto_rel_path "${CMAKE_CURRENT_SOURCE_DIR}/synapse-api" "${proto_file}")
    # Get the directory of the .proto file
    get_filename_component(proto_rel_dir "${proto_rel_path}" DIRECTORY)
    # Get the filename without extension
    get_filename_component(proto_name "${proto_file}" NAME_WE)

    # Set the output paths in the build directory, maintaining directory structure
    set(proto_src "${CMAKE_CURRENT_BINARY_DIR}/${proto_rel_dir}/${proto_name}.pb.cc")
    set(proto_hdr "${CMAKE_CURRENT_BINARY_DIR}/${proto_rel_dir}/${proto_name}.pb.h")

    list(APPEND GENERATED_PROTO_SRCS "${proto_src}")
    list(APPEND GENERATED_PROTO_HDRS "${proto_hdr}")

    add_custom_command(
        OUTPUT "${proto_src}" "${proto_hdr}"
        COMMAND ${PROTOC_EXECUTABLE}
        ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
             -I "${CMAKE_CURRENT_SOURCE_DIR}/synapse-api"
             -I "${CMAKE_CURRENT_SOURCE_DIR}/synapse-api/api"
             -I "${CMAKE_CURRENT_SOURCE_DIR}"
             -I "${protobuf_SOURCE_DIR}/src"
             "${proto_file}"
        DEPENDS "${proto_file}"
    )
endforeach()

# Include the generated Protobuf directory (build directory)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Also include subdirectories where generated headers are placed
include_directories("${CMAKE_CURRENT_BINARY_DIR}/synapse-api")

# Include the Protobuf source directory for standard .proto files
include_directories("${protobuf_SOURCE_DIR}/src")

# ======================================
# Define Source Files
# ======================================

# Collect all library source files
file(GLOB LIB_SOURCES "src/*.cpp" "bindings.cpp")

# ======================================
# Add the Pybind11 Python Module
# ======================================

pybind11_add_module(libndtp ${LIB_SOURCES} ${GENERATED_PROTO_SRCS})

# Explicitly set MSVC runtime library for libndtp target
set_target_properties(libndtp PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# ======================================
# Link Libraries
# ======================================

# Link against Pybind11 and Protobuf libraries
target_link_libraries(libndtp PRIVATE pybind11::module protobuf::libprotobuf)

# Ensure libndtp is built after protobuf
add_dependencies(libndtp protobuf::libprotobuf)

# ======================================
# Platform-specific Definitions
# ======================================

if(MSVC)
    target_compile_definitions(libndtp PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Additional MSVC-specific settings can go here
endif()

# ======================================
# Set Output Directories
# ======================================

set_target_properties(libndtp PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    PREFIX ""  # Remove 'lib' prefix on Unix-like systems
    SUFFIX "${Python_EXTENSION_MODULE_SUFFIX}"
)

# ======================================
# Installation Rules (Optional)
# ======================================

install(TARGETS libndtp
    LIBRARY DESTINATION lib
)