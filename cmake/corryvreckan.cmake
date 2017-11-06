# For every algorithm, build a separate library to be loaded by corryvreckan core
MACRO(corryvreckan_enable_default val)
    # Get the name of the algorithm
    GET_FILENAME_COMPONENT(_corryvreckan_algorithm_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all algorithms by default if not specified otherwise
    OPTION(BUILD_${_corryvreckan_algorithm_dir} "Build algorithm in directory ${_corryvreckan_algorithm_dir}?" ${val})
ENDMACRO()

# Common algorithm definitions
MACRO(corryvreckan_algorithm name)
    # Get the name of the algorithm
    GET_FILENAME_COMPONENT(_corryvreckan_algorithm_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all algorithms by default if not specified otherwise
    OPTION(BUILD_${_corryvreckan_algorithm_dir} "Build algorithm in directory ${_corryvreckan_algorithm_dir}?" ON)

    # Quit the file if not building this file or all algorithms
    IF(NOT (BUILD_${_corryvreckan_algorithm_dir} OR BUILD_ALL_ALGORITHMS))
        RETURN()
    ENDIF()

    # Put message
    MESSAGE( STATUS "Building algorithm: " ${_corryvreckan_algorithm_dir} )

    # Prepend with the algorithm prefix to create the name of the algorithm
    SET(${name} "CorryvreckanAlgorithm${_corryvreckan_algorithm_dir}")

    # Save the algorithm library for prelinking in the executable (NOTE: see exec folder)
    SET(CORRYVRECKAN_ALGORITHM_LIBRARIES ${CORRYVRECKAN_ALGORITHM_LIBRARIES} ${${name}} CACHE INTERNAL "Algorithm libraries")

    # Set default algorithm class name
    SET(_corryvreckan_algorithm_class "${_corryvreckan_algorithm_dir}")

    # Find if alternative algorithm class name is passed or we can use the default
    SET (extra_macro_args ${ARGN})
    LIST(LENGTH extra_macro_args num_extra_args)
    IF (${num_extra_args} GREATER 0)
        MESSAGE (AUTHOR_WARNING "Provided non-standard algorithm class name! Naming it ${_corryvreckan_algorithm_class} is recommended")
        LIST(GET extra_macro_args 0 _corryvreckan_algorithm_class)
    ENDIF ()

    # check if main header file is defined
    IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corryvreckan_algorithm_class}.h")
        MESSAGE(FATAL_ERROR "Header file ${_corryvreckan_algorithm_class}.h does not exist, cannot build algorithm! \
Create the header or provide the alternative class name as first argument")
    ENDIF()

    # Define the library
    ADD_LIBRARY(${${name}} SHARED "")

    # Add the current directory as include directory
    TARGET_INCLUDE_DIRECTORIES(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Set the special header flags and add the special dynamic implementation file
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_ALGORITHM_NAME=${_corryvreckan_algorithm_class})
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_ALGORITHM_HEADER="${_corryvreckan_algorithm_class}.h")

    TARGET_SOURCES(${${name}} PRIVATE "${PROJECT_SOURCE_DIR}/src/core/algorithm/dynamic_algorithm_impl.cpp")
    SET_PROPERTY(SOURCE "${PROJECT_SOURCE_DIR}/src/core/algorithm/dynamic_algorithm_impl.cpp" APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_corryvreckan_algorithm_class}.h")
ENDMACRO()

# Add sources to the algorithm
MACRO(corryvreckan_algorithm_sources name)
    # Get the list of sources
    SET(_list_var "${ARGN}")
    LIST(REMOVE_ITEM _list_var ${name})

    # Include directories for dependencies
    INCLUDE_DIRECTORIES(SYSTEM ${CORRYVRECKAN_DEPS_INCLUDE_DIRS})

    # Add the library
    TARGET_SOURCES(${name} PRIVATE ${_list_var})

    # Link the standard corryvreckan libraries
    TARGET_LINK_LIBRARIES(${name} ${CORRYVRECKAN_LIBRARIES} ${CORRYVRECKAN_DEPS_LIBRARIES})
ENDMACRO()

# Provide default install target for the algorithm
MACRO(corryvreckan_algorithm_install name)
    INSTALL(TARGETS ${name}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
ENDMACRO()
