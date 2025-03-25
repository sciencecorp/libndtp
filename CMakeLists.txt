cmake_minimum_required(VERSION 3.23.5)

SET(VCPKG_MANIFEST_FEATURES  "" CACHE STRING "vcpkg features")
if(NOT DEFINED LIBNDTP_VERSION)
  set(LIBNDTP_VERSION "0.0.1" CACHE STRING "Version of libndtp")
endif()
project(libndtp VERSION ${LIBNDTP_VERSION})

set(TARGET_NAME "science-${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Protobuf REQUIRED CONFIG)

add_library(${PROJECT_NAME})

configure_file(include/science/libndtp/version.h.in ${CMAKE_BINARY_DIR}/include/science/libndtp/version.h @ONLY)

get_filename_component(PROTO_INCLUDE_DIR external/sciencecorp/synapse-api REALPATH)
set(PROTO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/sciencecorp/synapse-api")
set(PROTO_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/science/libndtp/synapse")
file(MAKE_DIRECTORY ${PROTO_OUT_DIR})
file(GLOB_RECURSE PROTOS ${PROTO_INCLUDE_DIR}/api/*.proto)

message(STATUS "PROTO_INCLUDE_DIR: ${PROTO_INCLUDE_DIR}")

protobuf_generate(
  TARGET ${PROJECT_NAME}
  LANGUAGE cpp
  IMPORT_DIRS ${PROTO_INCLUDE_DIR}
  PROTOS ${PROTOS}
  PROTOC_OUT_DIR ${PROTO_OUT_DIR}
  OUT_VAR PROTO_SOURCES
)

file(GLOB_RECURSE SOURCES src/science/libndtp/*.cpp)
target_sources(
  ${PROJECT_NAME}
  PRIVATE
  ${SOURCES}
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/science/libndtp/synapse>
  $<INSTALL_INTERFACE:include>
  $<INSTALL_INTERFACE:include/science/libndtp/synapse>
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  protobuf::libprotobuf
)

include(GNUInstallDirs)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${TARGET_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)

# Export targets
install(
  EXPORT ${TARGET_NAME}Targets
  FILE ${TARGET_NAME}Targets.cmake
  NAMESPACE science::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
  FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

if ("tests" IN_LIST VCPKG_MANIFEST_FEATURES)
  enable_testing()

  find_package(GTest REQUIRED)

  file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp")

  add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

  target_link_libraries(${PROJECT_NAME}_tests
    PRIVATE
    ${PROJECT_NAME}
    GTest::GTest
    GTest::Main
  )

  include(GoogleTest)
  gtest_discover_tests(${PROJECT_NAME}_tests)
endif()
