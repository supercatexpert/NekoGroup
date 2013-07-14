cmake_minimum_required(VERSION 1.8)
project(NekoGroup C)
set(CMAKE_C_COMPLER cc)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

### 
include(UseGlibMarshal)
find_package(PkgConfig REQUIRED)
find_package(Gettext REQUIRED)
find_program(INTLTOOL_EXTRACT intltool-extract REQUIRED)
find_program(INTLTOOL_UPDATE intltool-update REQUIRED)
find_program(INTLTOOL_MERGE intltool-merge REQUIRED)

### 
configure_file(config.h.in config.h)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

### 
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

### 
PKG_CHECK_MODULES(LOUDMOUTH REQUIRED "loudmouth-1.0")
PKG_CHECK_MODULES(MONGO REQUIRED "libmongo-client")
PKG_CHECK_MODULES(GIO REQUIRED "gio-2.0 >= 2.26")
PKG_CHECK_MODULES(LIBSECRET REQUIRED "libsecret-1")

### 
set(DEPS_INCLUDE_DIRS ${LOUDMOUTH_INCLUDE_DIRS} ${MONGO_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} ${LIBSECRET_INCLUDE_DIRS})
set(DEPS_LIBRARY_DIRS ${LOUDMOUTH_LIBRARY_DIRS} ${MONGO_LIBRARY_DIRS} ${GIO_LIBRARY_DIRS} ${LIBSECRET_LIBRARY_DIRS})
set(DEPS_LIBRARIES ${LOUDMOUTH_LIBRARIES} ${MONGO_LIBRARIES} ${GIO_LIBRARIES} ${LIBSECRET_LIBRARIES})

#### 
add_subdirectory(src)
add_subdirectory(po)

