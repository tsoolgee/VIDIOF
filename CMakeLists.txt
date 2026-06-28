cmake_minimum_required(VERSION 3.20)
project(VideoFilter)

set(CMAKE_CXX_STANDARD 17)

set(BASECLASSES_DIR "${CMAKE_SOURCE_DIR}/baseclasses")

set(BASECLASSES_SOURCES
    ${BASECLASSES_DIR}/amextra.cpp
    ${BASECLASSES_DIR}/amfilter.cpp
    ${BASECLASSES_DIR}/amvideo.cpp
    ${BASECLASSES_DIR}/arithutil.cpp
    ${BASECLASSES_DIR}/combase.cpp
    ${BASECLASSES_DIR}/cprop.cpp
    ${BASECLASSES_DIR}/ctlutil.cpp
    ${BASECLASSES_DIR}/ddmm.cpp
    ${BASECLASSES_DIR}/dllentry.cpp
    ${BASECLASSES_DIR}/dllsetup.cpp
    ${BASECLASSES_DIR}/mtype.cpp
    ${BASECLASSES_DIR}/outputq.cpp
    ${BASECLASSES_DIR}/perflog.cpp
    ${BASECLASSES_DIR}/pstream.cpp
    ${BASECLASSES_DIR}/pullpin.cpp
    ${BASECLASSES_DIR}/refclock.cpp
    ${BASECLASSES_DIR}/renbase.cpp
    ${BASECLASSES_DIR}/schedule.cpp
    ${BASECLASSES_DIR}/seekpt.cpp
    ${BASECLASSES_DIR}/source.cpp
    ${BASECLASSES_DIR}/strmctl.cpp
    ${BASECLASSES_DIR}/sysclock.cpp
    ${BASECLASSES_DIR}/transfrm.cpp
    ${BASECLASSES_DIR}/transip.cpp
    ${BASECLASSES_DIR}/videoctl.cpp
    ${BASECLASSES_DIR}/vtrans.cpp
    ${BASECLASSES_DIR}/winctrl.cpp
    ${BASECLASSES_DIR}/winutil.cpp
    ${BASECLASSES_DIR}/wxdebug.cpp
    ${BASECLASSES_DIR}/wxlist.cpp
    ${BASECLASSES_DIR}/wxutil.cpp
)

add_library(strmbase STATIC ${BASECLASSES_SOURCES})

target_compile_definitions(strmbase PRIVATE
    WIN32 _WINDOWS UNICODE _UNICODE
    _CRT_SECURE_NO_WARNINGS WINVER=0x0601
)

target_include_directories(strmbase PRIVATE
    "${BASECLASSES_DIR}"
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um"
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/shared"
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/ucrt"
)

add_library(VideoFilter SHARED
    SRC/VideoFilter.cpp
    VideoFilter.def
)

target_compile_definitions(VideoFilter PRIVATE
    WIN32 _WINDOWS _USRDLL UNICODE _UNICODE
    _CRT_SECURE_NO_WARNINGS WINVER=0x0601
)

target_include_directories(VideoFilter PRIVATE
    "${BASECLASSES_DIR}"
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um"
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/shared"
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/ucrt"
)

target_link_libraries(VideoFilter PRIVATE
    strmbase
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
