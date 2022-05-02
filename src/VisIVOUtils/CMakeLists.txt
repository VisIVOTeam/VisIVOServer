cmake_minimum_required(VERSION 3.15)

project(VisIVOUtils CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)
find_package(Boost REQUIRED)

set(VSUTILS_SOURCE
    mainUtils.cpp
    vscreategenericslices.cpp
    vscreategenericslices.h
    vscreatepath.cpp
    vscreatepath.h
    vscreateslices.cpp
    vscreateslices.h
    vsloadhistory.cpp
    vsloadhistory.h
    vstextcol.cpp
    vstextcol.h
    vsutils.cpp
    vsutils.h
    vuparametersparser.cpp
    vuparametersparser.h
)

set(UTILS_SOURCE
    ${UTILS_SOURCE_DIR}/fparser.cpp
    ${UTILS_SOURCE_DIR}/fparser.h
    ${UTILS_SOURCE_DIR}/parametersparser.cpp
    ${UTILS_SOURCE_DIR}/parametersparser.h
    ${UTILS_SOURCE_DIR}/tinystr.cpp
    ${UTILS_SOURCE_DIR}/tinystr.h
    ${UTILS_SOURCE_DIR}/tinyxml.cpp
    ${UTILS_SOURCE_DIR}/tinyxml.h
    ${UTILS_SOURCE_DIR}/tinyxmlerror.cpp
    ${UTILS_SOURCE_DIR}/tinyxmlparser.cpp
    ${UTILS_SOURCE_DIR}/visivoutils.cpp
    ${UTILS_SOURCE_DIR}/visivoutils.h
    ${UTILS_SOURCE_DIR}/vsobject.cpp
    ${UTILS_SOURCE_DIR}/vsobject.h
    ${UTILS_SOURCE_DIR}/vstable.cpp
    ${UTILS_SOURCE_DIR}/vstable.h
)

add_executable(VisIVOUtils ${VSUTILS_SOURCE} ${UTILS_SOURCE})

target_include_directories(VisIVOUtils PRIVATE 
    ${UTILS_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(VisIVOUtils PRIVATE CURL::libcurl)
