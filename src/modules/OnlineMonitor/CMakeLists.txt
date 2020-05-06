# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_GLOBAL_MODULE(MODULE_NAME)

# Find the RootNewMacros.cmake file and include it
GET_FILENAME_COMPONENT(ROOT_CMAKE_DIR ${ROOT_USE_FILE} DIRECTORY)
FIND_FILE(ROOT_MACROS_FILE
          NAMES RootNewMacros.cmake RootMacros.cmake
          HINTS ${ROOT_CMAKE_DIR} ${ROOT_CMAKE_DIR}/modules/ $ENV{ROOTSYS}/cmake/modules)

IF(NOT ROOT_MACROS_FILE)
    MESSAGE(WARNING "Cannot find ROOT macros, including generic file as fallback.")
    INCLUDE(${ROOT_USE_FILE})
ELSE()
    INCLUDE(${ROOT_MACROS_FILE})
ENDIF()

# Generate the ROOT dictionary
ROOT_GENERATE_DICTIONARY(GuiDisplayDictionary
    ${CMAKE_CURRENT_SOURCE_DIR}/GuiDisplay.hpp
    LINKDEF
    ${CMAKE_CURRENT_SOURCE_DIR}/Linkdef.h
    OPTIONS
    -inlineInputHeader
    -I${CMAKE_CURRENT_SOURCE_DIR}
    MODULE
    GuiDisplay
)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/GuiDisplayDictionary.cxx
  PROPERTIES GENERATED TRUE COMPILE_FLAGS "-w"
)

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    OnlineMonitor.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/GuiDisplayDictionary.cxx
)

SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES COMPILE_FLAGS "-Wno-error=zero-as-null-pointer-constant")
TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::GuiBld)

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})

# Also install the dictionary objects
INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libGuiDisplay_rdict.pcm
    ${CMAKE_CURRENT_BINARY_DIR}/libGuiDisplay.rootmap
    DESTINATION lib)
