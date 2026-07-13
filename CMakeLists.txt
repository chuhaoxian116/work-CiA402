cmake_minimum_required(VERSION 3.16)

project(cia402
    VERSION 0.6.0
    DESCRIPTION "Generic CiA 402 helper library"
    LANGUAGES CXX
)

option(CIA402_BUILD_SHARED "Build cia402 as a shared library" ON)
option(CIA402_BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CIA402_BUILD_SHARED)
    set(CIA402_LIBRARY_TYPE SHARED)
else()
    set(CIA402_LIBRARY_TYPE STATIC)
endif()

add_library(cia402 ${CIA402_LIBRARY_TYPE}
    src/cia402.cpp
)

add_library(cia402::cia402 ALIAS cia402)

target_include_directories(cia402
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(cia402 PROPERTIES
    OUTPUT_NAME cia402
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "include/cia402/cia402.h"
)

if(CIA402_BUILD_EXAMPLES)
    add_executable(cia402_basic_usage examples/basic_usage.cpp)
    target_link_libraries(cia402_basic_usage PRIVATE cia402::cia402)
endif()

include(GNUInstallDirs)

install(TARGETS cia402
    EXPORT cia402Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cia402
)

install(EXPORT cia402Targets
    NAMESPACE cia402::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cia402
)
