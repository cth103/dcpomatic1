CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#--build type-------------------------------------------------------------------
PROJECT(OpenDCP)
SET(CMAKE_BUILD_TYPE Release)
STRING(TOLOWER ${PROJECT_NAME} PACKAGE_NAME)
#-------------------------------------------------------------------------------

#--version----------------------------------------------------------------------
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 0)
SET(VERSION_BUILD 24)
SET(OPENDCP_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD})
SET(OPENDCP_NAME ${PROJECT_NAME})
SET(OPENDCP_COPYRIGHT "(c) 2010-2012 Terrence Meiczinger. All rights reserved.")
SET(OPENDCP_LICENSE "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.")
SET(OPENDCP_WEBSITE "http://www.opendcp.org")
#-------------------------------------------------------------------------------

#--check 32 or 64-bit-----------------------------------------------------------
IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET(HOST_ARCH x86_64)
ELSE()
    SET(HOST_ARCH i686)
ENDIF()
#-------------------------------------------------------------------------------

#--options----------------------------------------------------------------------
OPTION(ENABLE_XMLSEC  "Enable XML digital singatures and security features" ON)
OPTION(ENABLE_OSX10.5 "Enable OSX 10.5 compatibility" OFF)
OPTION(ENABLE_OPENMP  "Enable OPENMP multithreading" ON)
OPTION(ENABLE_GUI     "Enable GUI compiling" ON)
OPTION(32BIT          "Enable 32-bit" OFF)
OPTION(RPM            "Create RPM package" OFF)
OPTION(DEB            "Create DEB package" OFF)

IF(UNIX)
    IF(EXISTS /usr/bin/rpmbuild)
        SET(RPM ON)
    ELSEIF(EXISTS /usr/bin/dpkg)
        SET(DEB ON)
    ENDIF()
ENDIF()

IF(ENABLE_XMLSEC)
    ADD_DEFINITIONS(-DXMLSEC)
    ADD_DEFINITIONS(-DLIBXML_STATIC)
    ADD_DEFINITIONS(-DLIBXSLT_STATIC)
    ADD_DEFINITIONS(-DXMLSEC_STATIC)
ENDIF()

IF(ENABLE_OPENMP)
    ADD_DEFINITIONS(-DOPENMP)
    SET(OPENMP -fopenmp)
ENDIF()

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
#-------------------------------------------------------------------------------

#--cmake options----------------------------------------------------------------
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#-------------------------------------------------------------------------------

#--set base compiler flags------------------------------------------------------
SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -O3 ${OPENMP}")
SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
#-------------------------------------------------------------------------------

#--adjust lib paths for 32-bit--------------------------------------------------
IF(32BIT)
    SET(CMAKE_CROSS_COMPILING ON)
    SET(TARGET_ARCH i686)
    IF(EXISTS /usr/lib64)
        SET(LIB_SEARCH  "lib64")
        SET(LIB_REPLACE "lib")
    ELSEIF(EXISTS /usr/lib32)
        SET(LIB_SEARCH  "lib")
        SET(LIB_REPLACE "lib32")
    ENDIF()
ELSE()
    SET(TARGET_ARCH ${HOST_ARCH})
ENDIF()
#-------------------------------------------------------------------------------

#--set OS Specific Compiler Flags-----------------------------------------------
IF(APPLE)
    IF(ENABLE_OSX10.5)
        SET(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.5.sdk)
        SET(OSXMIN -mmacosx-version-min=10.5)
        SET(OSXSDK -isysroot /Developer/SDKs/MacOSX10.5.sdk)
    ENDIF()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OSXMIN} -O3 -mfpmath=sse -mtune=native")
    SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-search_paths_first")
    SET(AUTO_CFLAGS "CFLAGS=${OSXMIN} ${OSXSDK} -Wl,-search_paths_first -O3 -mfpmath=sse -mtune=native")
    SET(AUTO_CXXFLAGS "CXXFLAGS=${OSXMIN} ${OSXSDK} -Wl,-search_paths_first")
    SET(AUTO_LDFLAGS "LDFLAGS=-Wl,-search_paths_first")
ELSEIF(WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPJ_STATIC -DKM_WIN32 -static")
    SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS}) 
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
ELSE(UNIX)
    IF(TARGET_ARCH STREQUAL "x86_64")
        SET(AUTO_CFLAGS "CFLAGS=-fPIC -O3")
    ELSE()
        SET(AUTO_CFLAGS "CFLAGS=-m32")
        SET(TARGET --target=i686)
        SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -m32")
        SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32 -melf_i386")
    ENDIF()
ENDIF()
#-------------------------------------------------------------------------------

#--build paths------------------------------------------------------------------
INCLUDE_DIRECTORIES(${OPENDCP_INCLUDE_DIR}/libopendcp
                    ${PROJECT_BINARY_DIR}/libopendcp/include
                    ${PROJECT_SOURCE_DIR}/libopendcp/image
                    ${PROJECT_SOURCE_DIR}/libasdcp
)
SET(OPENDCP_LIB opendcp-lib)
SET(ASDCP_LIBRARIES opendcp-asdcp opendcp-kumu)

IF(UNIX AND NOT APPLE AND TARGET_ARCH STREQUAL "x86_64")
    SET(LIB_INSTALL_PATH "lib64")
ELSE()
    SET(LIB_INSTALL_PATH "lib")
ENDIF()
#-------------------------------------------------------------------------------

#--contrib paths----------------------------------------------------------------
SET(OPENDCP_URL http://opendcp.googlecode.com/files)
SET(PREFIX ${PROJECT_BINARY_DIR}/contrib)
SET(PREFIX_ARG --prefix=${PREFIX})
#-------------------------------------------------------------------------------

MESSAGE(STATUS)
MESSAGE(STATUS "================================================================================")
MESSAGE(STATUS "OpenDCP Version ${OPENDCP_VERSION} CMake - ${CMAKE_SYSTEM_NAME} (${TARGET_ARCH})")
MESSAGE(STATUS "================================================================================")

#--set os specifc linking mode--------------------------------------------------
IF(APPLE)
    #-- Apple includes some old versions of certain libraries, so we want to compile the
    #-- recent versions of those. In other cases, we can use dynamic libraries.
    INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include)
    SET(LIB_DIR ${PREFIX}/lib)

    SET(COMPILE_ZLIB 0)
    SET(LIBS ${LIBS} -lz)
    
    SET(COMPILE_OPENSSL 0)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lssl -lcrypto)

    INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include/libxml2)

    SET(COMPILE_LIBXML2 1)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lxml2)

    IF(ENABLE_XMLSEC)
        INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include/libxslt)
        INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include/xmlsec1)
        SET(COMPILE_LIBXSLT 1)
        SET(COMPILE_XMLSEC 1)
        SET(LIBS ${LIBS} -L${LIB_DIR} -lxmlsec1-openssl -lxmlsec1 -lxslt)
    ENDIF(ENABLE_XMLSEC)

    SET(COMPILE_EXPAT 1)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lexpat)
   
    SET(COMPILE_TIFF 1)
    SET(LIBS ${LIBS} -L${LIB_DIR} -ltiff)

    SET(COMPILE_OPENJPEG 1)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lopenjpeg)

    ADD_SUBDIRECTORY(contrib)
ELSEIF(WIN32)
    #-- Win32 cross-compiling
    SET(SYSROOT_PATH ${CMAKE_INSTALL_PREFIX})
    SET(LIB_DIR ${SYSROOT_PATH}/lib)
    SET(PREFIX ${SYSROOT_PATH})
    SET(PREFIX_ARG --prefix=${PREFIX})
    IF(TARGET_ARCH STREQUAL "i686")
        SET(CMAKE_RC_COMPILER /usr/bin/i686-w64-mingw32-windres) 
    ELSE()
        SET(CMAKE_RC_COMPILER /usr/bin/x86_64-w64-mingw32-windres) 
    ENDIF()

    INCLUDE_DIRECTORIES(${SYSROOT_PATH}/include/libxml2)

    IF(ENABLE_XMLSEC)
        INCLUDE_DIRECTORIES(${SYSROOT_PATH}/include/libxslt)
        INCLUDE_DIRECTORIES(${SYSROOT_PATH}/include/xmlsec1)
        SET(COMPILE_XMLSEC 1)
        SET(LIBS ${LIBS} ${LIB_DIR}/libxmlsec1-openssl.a ${LIB_DIR}/libxmlsec1.a)
        SET(LIBS ${LIBS} -L${LIB_DIR} -lxslt)
    ENDIF(ENABLE_XMLSEC)

    SET(COMPILE_LIBXML2 0)
    SET(LIBS ${LIBS} -L{LIB_DIR} -lxml2)

    SET(COMPILE_EXPAT 0)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lexpat)

    SET(COMPILE_OPENJPEG 1)
    SET(LIBS ${LIBS} ${LIB_DIR}/libopenjpeg.a)

    SET(LIBS ${LIBS} ${LIB_DIR}/libtiff.a ${LIB_DIR}/libjpeg.a)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lcrypto -lssl)
    SET(LIBS ${LIBS} -L${LIB_DIR} -lz -lws2_32)

    ADD_SUBDIRECTORY(contrib)
ELSEIF(UNIX)
    SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules/)

    FIND_LIBRARY(ZLIB REQUIRED)
    FIND_PACKAGE(ZLIB REQUIRED)
    INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
    SET(LIBS ${LIBS} ${ZLIB_LIBRARIES})

    FIND_PACKAGE(OpenSSL REQUIRED)
    INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
    SET(LIBS ${LIBS} ${OPENSSL_LIBRARIES})

    FIND_PACKAGE(LibXml2 REQUIRED)
    INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
    SET(LIBS ${LIBS} ${LIBXML2_LIBRARIES})

    IF(ENABLE_XMLSEC)
        FIND_PACKAGE(LibXslt REQUIRED)
        INCLUDE_DIRECTORIES(${LIBXSLT_INCLUDE_DIR})
        SET(LIBS ${LIBS} ${LIBXSLT_LIBRARIES})

        FIND_PACKAGE(XMLSec REQUIRED)
        INCLUDE_DIRECTORIES(${XMLSEC1_INCLUDE_DIR})
        INCLUDE_DIRECTORIES(${XMLSEC1-OPENSSL_INCLUDE_DIR})
        SET(LIBS ${LIBS} ${XMLSEC1_LIBRARIES})
        SET(LIBS ${LIBS} ${XMLSEC1-OPENSSL_LIBRARIES})
    ENDIF(ENABLE_XMLSEC)

    FIND_PACKAGE(EXPAT REQUIRED)
    INCLUDE_DIRECTORIES(${XMLSEC1_INCLUDE_DIR})
    SET(LIBS ${LIBS} ${EXPAT_LIBRARIES})

    FIND_PACKAGE(TIFF REQUIRED)
    INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
    SET(LIBS ${LIBS} ${TIFF_LIBRARIES})

    #-- download and build openjpeg local because the versions
    #-- between linux distributions are not compatible
    SET(COMPILE_OPENJPEG 1)
    INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/contrib/include)
    SET(LIBS ${LIBS} ${PREFIX}/lib/libopenjpeg.a)

    ADD_SUBDIRECTORY(contrib)

    IF(CMAKE_CROSS_COMPILING)
        STRING(REGEX REPLACE ${LIB_SEARCH} ${LIB_REPLACE} LIBS "${LIBS}")
        MESSAGE(STATUS "Converting 64-bit path to 32-bit ${LIBS}")
    ENDIF()
ENDIF()
#-------------------------------------------------------------------------------

#--add source directories-------------------------------------------------------
ADD_SUBDIRECTORY(libasdcp)
ADD_SUBDIRECTORY(libopendcp)
ADD_SUBDIRECTORY(cli)
IF(ENABLE_GUI)
    ADD_SUBDIRECTORY(gui)
ENDIF()
#-------------------------------------------------------------------------------

#--build packages---------------------------------------------------------------
ADD_SUBDIRECTORY(packages)
#-------------------------------------------------------------------------------
