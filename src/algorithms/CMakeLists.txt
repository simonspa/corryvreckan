MACRO(corryvreckan_algorithm name)
  # Get the list of sources
  SET(_list_var "${ARGN}")
  LIST(REMOVE_ITEM _list_var ${name})

  # Include directories for dependencies
  INCLUDE_DIRECTORIES(SYSTEM ${CORRYVRECKAN_DEPS_INCLUDE_DIRS})

  # Build all modules by default if not specified otherwise
  OPTION(BUILD_${name} "Build algorithm ${name}?" ON)

  # Put message
  MESSAGE(STATUS "Building algorithm: " ${name} )

  # Prepend with the allpix module prefix to create the name of the module
  SET(LIBNAME "CorryvreckanAlgorithm${name}")

  # Save the module library for prelinking in the executable (NOTE: see exec folder)
  SET(CORRYVRECKAN_ALGORITHM_LIBRARIES ${CORRYVRECKAN_ALGORITHM_LIBRARIES} ${LIBNAME} CACHE INTERNAL "Algorithm libraries")

  # Define the library
  ADD_LIBRARY(${LIBNAME} SHARED ${_list_var})

  # Add the current directory as include directory
  TARGET_INCLUDE_DIRECTORIES(${LIBNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  # Set the special header flags and add the special dynamic implementation file
  TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE CORRYVRECKAN_ALGORITHM_NAME=${name})
  TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE CORRYVRECKAN_ALGORITHM_HEADER="${name}.h")

  TARGET_SOURCES(${LIBNAME} PRIVATE "${PROJECT_SOURCE_DIR}/src/core/dynamic_algorithm_impl.cpp")
  SET_PROPERTY(SOURCE "${PROJECT_SOURCE_DIR}/src/core/dynamic_algorithm_impl.cpp" APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.h")

  # Link the standard allpix libraries
  TARGET_LINK_LIBRARIES(${LIBNAME} ${CORRYVRECKAN_LIBRARIES} ${CORRYVRECKAN_DEPS_LIBRARIES})

  INSTALL(TARGETS ${LIBNAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
ENDMACRO()

# Include the dependencies
INCLUDE_DIRECTORIES(SYSTEM ${_DEPS_INCLUDE_DIRS})

# Build all the modules
FILE(GLOB srcfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FOREACH(src ${srcfiles})
  GET_FILENAME_COMPONENT(name ${src} NAME_WE)
  # Add the library
  CORRYVRECKAN_ALGORITHM(${name} ${src})
ENDFOREACH()
