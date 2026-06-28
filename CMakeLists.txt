cmake_minimum_required(VERSION 3.20)
project(VideoFilter)

set(CMAKE_CXX_STANDARD 17)

# Windows SDK + DirectShow BaseClasses
# נבנה ידנית נגד strmbase.lib

add_library(VideoFilter SHARED
    src/VideoFilter.cpp
)

target_compile_definitions(VideoFilter PRIVATE
    WIN32
    _WINDOWS
    _USRDLL
    UNICODE
    _UNICODE
)

# DirectShow headers + strmbase
target_include_directories(VideoFilter PRIVATE
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um"
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/shared"
    "${CMAKE_SOURCE_DIR}/baseclasses"
)

target_link_libraries(VideoFilter PRIVATE
    "${CMAKE_SOURCE_DIR}/baseclasses/strmbase.lib"
    strmiids.lib
    ole32.lib
    oleaut32.lib
    uuid.lib
    advapi32.lib
    winmm.lib
)

set_target_properties(VideoFilter PROPERTIES
    OUTPUT_NAME "VideoFilter"
    SUFFIX ".ax"
)
