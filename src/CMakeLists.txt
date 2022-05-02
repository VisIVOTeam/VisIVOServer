cmake_minimum_required(VERSION 3.15)

project(VisIVOServer)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(UTILS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Utils")
set(IMPORTER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/VisIVOImporter")
set(FILTERS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/VisIVOFilters")
set(VSUTILS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/VisIVOUtils")
set(VIEWER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/VisIVOViewer")

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(${IMPORTER_SOURCE_DIR})
add_subdirectory(${FILTERS_SOURCE_DIR})
add_subdirectory(${VSUTILS_SOURCE_DIR})
add_subdirectory(${VIEWER_SOURCE_DIR})
