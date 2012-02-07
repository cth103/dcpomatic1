cmake_minimum_required(VERSION 2.8)

MESSAGE(STATUS)
MESSAGE(STATUS "-------------------------------------------------------------------------------")
MESSAGE(STATUS "Creating GUI Build Files")
MESSAGE(STATUS "-------------------------------------------------------------------------------")

IF(CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS(-g)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

#--find qt4-------------------------------------------------------------
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
#-----------------------------------------------------------------------------

#--set gui source-------------------------------------------------------------
SET(OPENDCP_GUI_CXX_SRC
    main.cpp
    mainwindow.cpp
    j2k.cpp
    mxf.cpp
    xml.cpp
    generatetitle.cpp
    dialogj2kconversion.cpp
    dialogmxfconversion.cpp
    mxf-writer.cpp
)

SET(OPENDCP_GUI_H_SRC
    mainwindow.h
    generatetitle.h
    dialogj2kconversion.h
    dialogmxfconversion.h
    mxf-writer.h
)

SET(OPENDCP_GUI_UI
    mainwindow.ui
    generatetitle.ui
    dialogj2kconversion.ui
    dialogmxfconversion.ui
)

SET(OPENDCP_GUI_RES
     ${CMAKE_CURRENT_SOURCE_DIR}/resources/opendcp.qrc
)

SET(OPENDCP_GUI_RES_CXX
     ${CMAKE_CURRENT_BINARY_DIR}/opendcp_resources.cpp 
)
#-----------------------------------------------------------------------------

#--set application icon and information---------------------------------------
IF(APPLE)
    SET(EXE_NAME ${OPENDCP_NAME})
    SET(MACOSX_BUNDLE_ICON_FILE opendcp.icns)
    SET(MACOSX_BUNDLE_GUI_IDENTIFIER org.opendcp.opendcp)
    SET(MACOSX_BUNDLE_LONG_VERSION_STRING ${OPENDCP_VERSION})
    SET(MACOSX_BUNDLE_SHORT_VERSION_STRING ${OPENDCP_VERSION})
    SET(MACOSX_BUNDLE_BUNDLE_VERSION ${OPENDCP_VERSION})
    SET(MACOSX_BUNDLE_BUNDLE_NAME ${OPENDCP_NAME})
    SET(MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2010-2011 Terrence Meiczinger. All Rights Reserved.")

    IF(ENABLE_OSX386)
        SET(CMAKE_OSX_ARCHITECTURES i386;x86_64)
    ELSE()
        SET(CMAKE_OSX_ARCHITECTURES x86_64)
    ENDIF()

    SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/resources/opendcp.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    SET(OPENDCP_GUI_CXX_SRC ${OPENDCP_GUI_CXX_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/resources/opendcp.icns)
ELSEIF(UNIX)
    STRING(TOLOWER ${OPENDCP_NAME} EXE_NAME)
    IF(NOT DESKTOP_ENTRY)
        SET(DESKTOP_ENTRY opendcp.desktop)
    ENDIF(NOT DESKTOP_ENTRY)
 
    ADD_CUSTOM_COMMAND(OUTPUT ${DESKTOP_ENTRY}
        COMMAND touch ${DESKTOP_ENTRY}
        COMMAND sh ${PROJECT_SOURCE_DIR}/packages/opendcp-desktop.sh ${CMAKE_INSTALL_PREFIX} >${DESKTOP_ENTRY}
        DEPENDS ${PROJECT_SOURCE_DIR}/packages/opendcp-desktop.sh
        COMMENT "Generating desktop entry file"
    )

    ADD_CUSTOM_TARGET(DESKTOP_ENTRY_FILE ALL
        DEPENDS ${DESKTOP_ENTRY}
    )
 
    SET(APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/opendcp.png)
ELSEIF(WIN32)
    STRING(TOLOWER ${OPENDCP_NAME} EXE_NAME)
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/opendcp_ico.o
                       COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}/resources/opendcp.rc 
                               -o ${CMAKE_CURRENT_BINARY_DIR}/opendcp_ico.o)
    SET(OPENDCP_GUI_CXX_SRC ${OPENDCP_GUI_CXX_SRC} ${CMAKE_CURRENT_BINARY_DIR}/opendcp_ico.o)
ENDIF()
#-----------------------------------------------------------------------------

#--add qt4 src----------------------------------------------------------------
QT4_ADD_RESOURCES(OPENDCP_GUI_RES_CXX ${OPEDNDCP_GUI_RES})
ADD_CUSTOM_COMMAND(OUTPUT ${OPENDCP_GUI_RES_CXX}  
    COMMAND ${QT_RCC_EXECUTABLE}
    ARGS ${rcc_options} -name ${OPENDCP_GUI_RES_CXX} -o ${OPENDCP_GUI_RES_CXX} ${OPENDCP_GUI_RES} 
    MAIN_DEPENDENCY ${OPENDCP_GUI_RES} 
    DEPENDS ${_RC_DEPENDS})
QT4_WRAP_UI(OPENDCP_FORM_H ${OPENDCP_GUI_UI})
QT4_WRAP_CPP(OPENDCP_MOC_H ${OPENDCP_GUI_H_SRC})
#-----------------------------------------------------------------------------

#--compile gui----------------------------------------------------------------
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
IF(APPLE)
    ADD_EXECUTABLE(${EXE_NAME} MACOSX_BUNDLE ${OPENDCP_GUI_CXX_SRC} ${OPENDCP_GUI_RES_CXX} ${OPENDCP_FORM_H} ${OPENDCP_MOC_H})
ELSEIF(WIN32)
    ADD_EXECUTABLE(${EXE_NAME} WIN32 ${OPENDCP_GUI_CXX_SRC} ${OPENDCP_GUI_RES_CXX} ${OPENDCP_FORM_H} ${OPENDCP_MOC_H})
ELSE()
    ADD_EXECUTABLE(${EXE_NAME} ${OPENDCP_GUI_CXX_SRC} ${OPENDCP_GUI_RES_CXX} ${OPENDCP_FORM_H} ${OPENDCP_MOC_H})
ENDIF()
#----------------------------------------------------------------------------

#--build---------------------------------------------------------------------
SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS})
TARGET_LINK_LIBRARIES(${EXE_NAME} ${QT_LIBRARIES} ${OPENDCP_LIB} ${ASDCP_LIBRARIES} ${LIBS})
#----------------------------------------------------------------------------

#--install-------------------------------------------------------------------
INSTALL(
    TARGETS ${EXE_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

IF(APPLE)
    SET(PLUGIN_DEST_DIR opendcp.app/Contents/MacOS)
    SET(QTCONF_DEST_DIR opendcp.app/Contents/Resources)
    SET(OSX_LIB_DIRS ${QT_LIBRARY_DIRS})
    SET(APPLICATION "\${CMAKE_INSTALL_PREFIX}/${EXE_NAME}.app")

    INSTALL(
        CODE "
        file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
        "
    )

    INSTALL(
        CODE "
        file(GLOB_RECURSE QTPLUGINS
             \"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_DEST_DIR}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
        include(BundleUtilities)
        fixup_bundle(\"${APPLICATION}\" \"\${QTPLUGINS}\" \"${OSX_LIB_DIRS}\")
        "
    )
ELSEIF(UNIX)
    INSTALL(FILES ${APP_ICON} DESTINATION share/icons)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/opendcp.desktop DESTINATION share/applications)
ENDIF()
#----------------------------------------------------------------------------
