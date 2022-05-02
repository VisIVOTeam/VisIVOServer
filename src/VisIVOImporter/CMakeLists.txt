cmake_minimum_required(VERSION 3.15)

project(VisIVOImporter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CFITSIO REQUIRED)
find_package(HDF5 REQUIRED)
find_package(CURL REQUIRED)

set(IMPORTER_SOURCE
    Importers/abstractsource.cpp
    Importers/abstractsource.h
    Importers/asciisource.cpp
    Importers/asciisource.h
    Importers/binsource.cpp
    Importers/binsource.h
    Importers/csvsource.cpp
    Importers/csvsource.h
    Importers/fitsimagesource.cpp
    Importers/fitsimagesource.h
    Importers/fitstablesource.cpp
    Importers/fitstablesource.h
    Importers/flysource.cpp
    Importers/flysource.h
    Importers/gadgetsource.cpp
    Importers/gadgetsource.h
    Importers/hdf5source.cpp
    Importers/hdf5source.h
    Importers/mainImporter.cpp
    Importers/muportalsource.cpp
    Importers/muportalsource.h
    Importers/ramsessource.cpp
    Importers/ramsessource.h
    Importers/rawgridsource.cpp
    Importers/rawgridsource.h
    Importers/rawpointssource.cpp
    Importers/rawpointssource.h
    Importers/vosourcenew.cpp
    Importers/vosourcenew.h
)

set(UTILS_SOURCE
    ${UTILS_SOURCE_DIR}/commandline.cpp
    ${UTILS_SOURCE_DIR}/commandline.h
    ${UTILS_SOURCE_DIR}/historyXmlWriter.cc
    ${UTILS_SOURCE_DIR}/historyXmlWriter.h
    ${UTILS_SOURCE_DIR}/tinystr.cpp
    ${UTILS_SOURCE_DIR}/tinystr.h
    ${UTILS_SOURCE_DIR}/tinyxml.cpp
    ${UTILS_SOURCE_DIR}/tinyxml.h
    ${UTILS_SOURCE_DIR}/tinyxmlerror.cpp
    ${UTILS_SOURCE_DIR}/tinyxmlparser.cpp
    ${UTILS_SOURCE_DIR}/visivoutils.cpp
    ${UTILS_SOURCE_DIR}/visivoutils.h
)

add_executable(VisIVOImporter ${IMPORTER_SOURCE} ${UTILS_SOURCE})

target_compile_definitions(VisIVOImporter PRIVATE LIGHT)

target_include_directories(VisIVOImporter PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/Importers
    ${UTILS_SOURCE_DIR}
)

target_link_libraries(VisIVOImporter PRIVATE
    CFITSIO::CFITSIO
    HDF5::HDF5
    CURL::libcurl
)
