cmake_minimum_required(VERSION 3.14)

project(epub_process LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set( PKGS_TO_FIND Core Network WebEngine WebEngineWidgets Widgets Xml Concurrent PrintSupport LinguistTools )
find_package(QT NAMES Qt6 Qt5 COMPONENTS ${PKGS_TO_FIND})
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS  ${PKGS_TO_FIND})

set(TS_FILES epub_process_zh_CN.ts)
add_executable(epub_process
  main.cpp
  QCodePage437Codec.cpp
  QCodePage437Codec.h
  epubbook.cpp
  epubbook.h
  utility.cpp
  utility.h
  mediatypes.cpp
  mediatypes.h
  ${TS_FILES}
)
include_directories( BEFORE
                     ${HUNSPELL_INCLUDE_DIRS}
                     ${PCRE_INCLUDE_DIRS}
                     ${MINIZIP_INCLUDE_DIRS}
                     ${ZLIB_INCLUDE_DIRS}
                     ${PTHON_INCLUDE_DIRS}
                     ${GUMBO_INCLUDE_DIRS}
                     ${SPDLOG_INCLUDE_DIRS}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_BINARY_DIR} )
set( LIBS_TO_LINK ${HUNSPELL_LIBRARIES} ${PCRE_LIBRARIES} ${GUMBO_LIBRARIES} ${MINIZIP_LIBRARIES}
                  Qt5::Widgets  Qt5::Xml  Qt5::PrintSupport  Qt5::WebEngine
                  Qt5::WebEngineWidgets  Qt5::Network  Qt5::Concurrent )
target_link_libraries( ${PROJECT_NAME} ${LIBS_TO_LINK} )

if(COMMAND qt_create_translation)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()


