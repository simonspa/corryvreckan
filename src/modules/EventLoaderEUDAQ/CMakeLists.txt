CORRYVRECKAN_ENABLE_DEFAULT(OFF)

# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_GLOBAL_MODULE(MODULE_NAME)

FIND_PACKAGE(EUDAQ1 QUIET)
IF(NOT EUDAQ_FOUND)
    MESSAGE(FATAL_ERROR "Could not find EUDAQ 1.7, make sure to set the path to your installation using\n"
    "$ export EUDAQPATH=/your/path/to/EUDAQ")
ENDIF()

INCLUDE_DIRECTORIES(SYSTEM ${EUDAQ_INCLUDE_DIR})

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    EventLoaderEUDAQ.cpp
    SequentialReader.cpp
)

# Add EUDAQ libraries
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${EUDAQ_LIBRARY})

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
