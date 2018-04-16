# Option to build all algorithms
OPTION(BUILD_ALL_ALGORITHMS "Build all algorithms?" OFF)

# reset the saved libraries
SET(CORRYVRECKAN_ALGORITHM_LIBRARIES "" CACHE INTERNAL "Algorithm libraries")

FILE(GLOB subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
FOREACH(subdir ${subdirs})
    IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
        # Internal macros check if the module is actually enabled
        ADD_SUBDIRECTORY(${subdir})
    ENDIF()
ENDFOREACH()
