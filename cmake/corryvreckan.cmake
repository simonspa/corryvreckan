# For every module, build a separate library to be loaded by corryvreckan core
MACRO(corryvreckan_enable_default val)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_corryvreckan_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all modules by default if not specified otherwise
    OPTION(BUILD_${_corryvreckan_module_dir} "Build module in directory ${_corryvreckan_module_dir}?" ${val})
ENDMACRO()

# Common module definitions
MACRO(_corryvreckan_module_define_common name)
    # Get the name of the module
    GET_FILENAME_COMPONENT(_corryvreckan_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all modules by default if not specified otherwise
    OPTION(BUILD_${_corryvreckan_module_dir} "Build module in directory ${_corryvreckan_module_dir}?" ON)

    # Put message
    MESSAGE(STATUS "Building module " ${BUILD_${_corryvreckan_module_dir}} "\t- " ${_corryvreckan_module_dir})

    # Quit the file if not building this file or all modules
    IF(NOT (BUILD_${_corryvreckan_module_dir} OR BUILD_ALL_MODULES))
        RETURN()
    ENDIF()

    # Prepend with the module prefix to create the name of the module
    SET(${name} "CorryvreckanModule${_corryvreckan_module_dir}")

    # Save the module library for prelinking in the executable (NOTE: see exec folder)
    SET(CORRYVRECKAN_MODULE_LIBRARIES ${CORRYVRECKAN_MODULE_LIBRARIES} ${${name}} CACHE INTERNAL "Module libraries")

    # Set default module class name
    SET(_corryvreckan_module_class "${_corryvreckan_module_dir}")

    # Find if alternative module class name is passed or we can use the default
    SET (extra_macro_args ${ARGN})
    LIST(LENGTH extra_macro_args num_extra_args)
    IF (${num_extra_args} GREATER 0)
        MESSAGE (AUTHOR_WARNING "Provided non-standard module class name! Naming it ${_corryvreckan_module_class} is recommended")
        LIST(GET extra_macro_args 0 _corryvreckan_module_class)
    ENDIF ()

    # check if main header file is defined
    IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corryvreckan_module_class}.h")
        MESSAGE(FATAL_ERROR "Header file ${_corryvreckan_module_class}.h does not exist, cannot build module! \
Create the header or provide the alternative class name as first argument")
    ENDIF()

    # Define the library
    ADD_LIBRARY(${${name}} SHARED "")

    # Add the current directory as include directory
    TARGET_INCLUDE_DIRECTORIES(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Set the special header flags and add the special dynamic implementation file
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_MODULE_NAME=${_corryvreckan_module_class})
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_MODULE_HEADER="${_corryvreckan_module_class}.h")

    TARGET_SOURCES(${${name}} PRIVATE "${PROJECT_SOURCE_DIR}/src/core/module/dynamic_module_impl.cpp")
    SET_PROPERTY(SOURCE "${PROJECT_SOURCE_DIR}/src/core/module/dynamic_module_impl.cpp" APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_corryvreckan_module_class}.h")
ENDMACRO()

# Put this at the start of every global module
MACRO(corryvreckan_global_module name)
    _corryvreckan_module_define_common(${name} ${ARGN})

    # Set the unique flag to true
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_MODULE_GLOBAL=1)
ENDMACRO()

# Put this at the start of every detector module
MACRO(corryvreckan_detector_module name)
    _corryvreckan_module_define_common(${name} ${ARGN})

    # Set the unique flag to false
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_MODULE_GLOBAL=0)
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_MODULE_DUT=0)
ENDMACRO()

# Append list of possible detector types as compile definition
MACRO(corryvreckan_detector_type name)
    SET(extra_macro_args ${ARGN})

    LIST(LENGTH extra_macro_args num_extra_args)
    IF(${num_extra_args} GREATER 0)
        STRING(REPLACE ";" "," TYPESLIST "${extra_macro_args}")
        TARGET_COMPILE_DEFINITIONS(${name} PRIVATE CORRYVRECKAN_DETECTOR_TYPE=${TYPESLIST})
    ENDIF()
ENDMACRO()

# Put this at the start of every detector module
MACRO(corryvreckan_dut_module name)
    _corryvreckan_module_define_common(${name} ${ARGN})

    # Set the unique flag to false
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_MODULE_GLOBAL=0)
    TARGET_COMPILE_DEFINITIONS(${${name}} PRIVATE CORRYVRECKAN_MODULE_DUT=1)
ENDMACRO()

# Add sources to the module
MACRO(corryvreckan_module_sources name)
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

# Provide default install target for the module
MACRO(corryvreckan_module_install name)
    INSTALL(TARGETS ${name}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
ENDMACRO()
