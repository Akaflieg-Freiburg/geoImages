cmake_minimum_required(VERSION 3.14)

project(geoImages LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_testing(true)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(SRC="${CMAKE_CURRENT_SOURCE_DIR}")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Positioning Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Positioning Test)

#
# GeoTIFF
#

add_executable(geoImages
    DataFileAbstract.h
    GeoTIFF.cpp
    GeoTIFF.h
    main.cpp
)
target_link_libraries(geoImages Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Positioning)

install(TARGETS geoImages
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


# GeoTIFFTest

ADD_EXECUTABLE(GeoTIFFTest
    DataFileAbstract.h
    GeoTIFF.cpp
    GeoTIFFTest.cpp
    GeoTIFFTest.h
)
TARGET_LINK_LIBRARIES(GeoTIFFTest Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Positioning Qt${QT_VERSION_MAJOR}::Test)
ADD_TEST(GeoTIFFTest GeoTIFFTest)
