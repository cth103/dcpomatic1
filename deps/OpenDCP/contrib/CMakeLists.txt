# Third Party Libraries
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE(ExternalProject)

IF(WIN32)
    IF(TARGET_ARCH STREQUAL x86_64)
        SET(CONFIGURE mingw64-configure)
        SET(MAKE mingw64-make)
        SET(INSTALL sudo make)
    ELSE()
        SET(CONFIGURE mingw32-configure)
        SET(MAKE mingw32-make)
        SET(INSTALL sudo make)
    ENDIF()
ELSE()
    SET(MAKE make)
    SET(CONFIGURE <SOURCE_DIR>/configure)
    SET(INSTALL make)
ENDIF()

IF(COMPILE_ZLIB)
    EXTERNALPROJECT_ADD(00_ZLIB
        URL http://zlib.net/zlib-1.2.5.tar.gz 
        BUILD_IN_SOURCE 1
        PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/zlib-1.2.5.patch
        CONFIGURE_COMMAND sh <SOURCE_DIR>/mkzlib.sh ${PREFIX} ${AUTO_CFLAGS} ${AUTO_LDFLAGS}
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
    )
ENDIF(COMPILE_ZLIB)

IF(COMPILE_LIBXML2)
    EXTERNALPROJECT_ADD(02_XML2
        URL ${OPENDCP_URL}/libxml2-2.7.8.tar.gz
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
                          --with-iconv=no
                          --with-threads=off
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
    )
ENDIF(COMPILE_LIBXML2)

IF(ENABLE_XMLSEC AND COMPILE_XMLSEC)
    IF(COMPILE_LIBXSLT)
        EXTERNALPROJECT_ADD(03_XSLT
            URL ${OPENDCP_URL}/libxslt-1.1.26.tar.gz
            BUILD_IN_SOURCE 1
            PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/libxslt-1.1.26.patch
            CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                             --disable-dependency-tracking --enable-static=yes --enable-shared=no
                             --with-libxml-prefix=${PREFIX}
                             --with-libxml-include-prefix=${PREFIX}/include/libxml2
                             --with-libxml-libs-prefix=${PREFIX}/lib
            BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
            INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
    )
    ENDIF(COMPILE_LIBXSLT)

    EXTERNALPROJECT_ADD(06_XMLSEC1
        URL http://www.aleksey.com/xmlsec/download/xmlsec1-1.2.18.tar.gz
        BUILD_IN_SOURCE 1
        PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/xmlsec1-1.1.28.patch
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --enable-pkgconfig=no
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
                          --enable-crypto-dl=no
                          --enable-apps-crypto-dl=no
                          --enable-static-linking=yes
                          --with-libxml=${PREFIX}
                          --with-libxslt=${PREFIX}
                          --with-openssl=${PREFIX}
                          --with-default-crypto=openssl
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
     )
ENDIF(ENABLE_XMLSEC AND COMPILE_XMLSEC)

IF(COMPILE_EXPAT)
    EXTERNALPROJECT_ADD(04_EXPAT
        URL ${OPENDCP_URL}/expat-2.0.1.tar.gz
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                          --disable-dependency-tracking --enable-static=yes --enable-shared=no
        BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
        INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_EXPAT)

SET(OPENSSL_MAKE make)
IF(APPLE)
    SET(OPENSSL_CONFIGURE <SOURCE_DIR>/Configure darwin64-x86_64-cc ${PREFIX_ARG})
ELSEIF(MINGW)
    SET(OPENSSL_CONFIGURE <SOURCE_DIR>/config mingw ${PREFIX_ARG})
ELSE()
    SET(OPENSSL_CONFIGURE <SOURCE_DIR>/config ${PREFIX_ARG})
ENDIF()

IF(COMPILE_OPENSSL)
EXTERNALPROJECT_ADD(05_OPENSSL
    URL http://www.openssl.org/source/openssl-0.9.8r.tar.gz
    BUILD_IN_SOURCE 1
    PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/openssl-0.9.8r.patch
    CONFIGURE_COMMAND ${OPENSSL_CONFIGURE}
    BUILD_COMMAND ${OPENSSL_MAKE} -f <SOURCE_DIR>/Makefile
    INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_OPENSSL)

IF(PREFIX)
    SET(OPENSSL_PREFIX ${PREFIX})
    SET(EXPAT_PREFIX ${PREFIX})
ELSE()
    SET(OPENSSL_PREFIX "yes")
    SET(EXPAT_PREFIX "yes")
ENDIF()

IF(COMPILE_TIFF)
EXTERNALPROJECT_ADD(08_TIFF
    URL http://download.osgeo.org/libtiff/tiff-4.0.0.tar.gz
    BUILD_IN_SOURCE 1
    PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/libtiff-4.0.0.patch
    CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                      --disable-dependency-tracking --enable-static=yes --enable-shared=no
                      --enable-jpeg=no
    BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
    INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_TIFF)

IF(COMPILE_OPENJPEG)
EXTERNALPROJECT_ADD(09_OpenJPEG
    URL ${OPENDCP_URL}/openjpeg_v1_4_sources_r697.tar.gz
    PATCH_COMMAND patch -d <SOURCE_DIR> -p0 -t -N < ${PROJECT_SOURCE_DIR}/contrib/openjpeg_v1_4.patch
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CONFIGURE} ${PREFIX_ARG} ${AUTO_CFLAGS} ${AUTO_CXXFLAGS} ${AUTO_LDFLAGS}
                      --disable-dependency-tracking --enable-static=yes --enable-shared=no --enable-png=no --enable-tiff=no ${TARGET}
    BUILD_COMMAND ${MAKE} -f <SOURCE_DIR>/Makefile
    INSTALL_COMMAND ${INSTALL} -f <SOURCE_DIR>/Makefile install ${INSTALL_DIR}
)
ENDIF(COMPILE_OPENJPEG)
