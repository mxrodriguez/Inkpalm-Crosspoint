cmake_minimum_required(VERSION 3.18)
project(crosspoint-android)

# Resolve to absolute real path (resolves symlinks, avoids /mnt/d/ issues)
get_filename_component(CROSSPOINT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" REALPATH)
set(STUB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android_stubs")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DANDROID=1)
add_definitions(-DCROSPOINT_ANDROID=1)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CROSSPOINT_ROOT}/lib
    ${CROSSPOINT_ROOT}/lib/ZipFile
    ${CROSSPOINT_ROOT}/lib/uzlib/src
    ${CROSSPOINT_ROOT}/lib/expat
    ${STUB_DIR}
)

# uzlib (compression) - C library
add_library(uzlib STATIC "${CROSSPOINT_ROOT}/lib/uzlib/src/tinflate.c" "${STUB_DIR}/uzlib_checksums.c")

# Logging (Android stub)
add_library(crosspoint_logging STATIC "${STUB_DIR}/Logging.cpp")

# Utf8
add_library(utf8 STATIC "${CROSSPOINT_ROOT}/lib/Utf8/Utf8.cpp")

# InflateReader (stub impl in android_stubs)
add_library(inflateReader STATIC "${STUB_DIR}/InflateReader.cpp")
target_link_libraries(inflateReader uzlib crosspoint_logging)

# ZipFile
add_library(zipfile STATIC "${CROSSPOINT_ROOT}/lib/ZipFile/ZipFile.cpp")
target_link_libraries(zipfile inflateReader crosspoint_logging)

# Expat (XML parser)
add_library(expat STATIC
    "${CROSSPOINT_ROOT}/lib/expat/xmlparse.c"
    "${CROSSPOINT_ROOT}/lib/expat/xmlrole.c"
    "${CROSSPOINT_ROOT}/lib/expat/xmltok.c"
)
target_compile_definitions(expat PRIVATE
    XML_GE=1
    XML_CONTEXT_BYTES=1024
    XML_DTD=1
    XML_NS=1
)

# JsonParser
add_library(jsonparser STATIC
    "${CROSSPOINT_ROOT}/lib/JsonParser/ReleaseJsonParser.cpp"
    "${CROSSPOINT_ROOT}/lib/JsonParser/StreamingJsonParser.cpp"
)

# Main JNI bridge
add_library(crosspoint-jni SHARED jni_bridge.cpp)
target_link_libraries(crosspoint-jni
    android log jnigraphics
    crosspoint_logging zipfile expat jsonparser uzlib utf8 inflateReader
)
target_include_directories(crosspoint-jni PRIVATE
    ${CMAKE_SOURCE_DIR} "${CROSSPOINT_ROOT}/lib" "${CROSSPOINT_ROOT}/lib/uzlib/src" "${CROSSPOINT_ROOT}/lib/expat" ${STUB_DIR}
)