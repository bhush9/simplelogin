cmake_minimum_required(VERSION 2.8.8)

project(SSSSDDDDDMMMM)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR};${CMAKE_MODULE_PATH}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Silence CMake 3.0 warnings
if(POLICY CMP0043)
    cmake_policy(SET CMP0043 OLD)
endif()

find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)

find_package(PAM REQUIRED)
add_definitions(-DUSE_PAM)


add_definitions(-Wall -std=c++11)

include_directories(
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/auth"
)
include_directories("${CMAKE_BINARY_DIR}/src/common")

set(HELPER_SOURCES
    HelperApp.cpp
    UserSession.cpp
    backend/PamHandle.cpp
)

add_executable(sddm-helper ${HELPER_SOURCES})

target_link_libraries(sddm-helper Qt5::Core ${PAM_LIBRARIES})


# install(TARGETS sddm-helper RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
