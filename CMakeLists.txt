# CMake file for the Corryvreckan framework
CMAKE_MINIMUM_REQUIRED(VERSION 3.4.3 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW) # change linker path search behaviour
  CMAKE_POLICY(SET CMP0048 NEW) # set project version
ENDIF(COMMAND CMAKE_POLICY)

# Set default version
SET(CORRYVRECKAN_VERSION "v1.0")

# Set default build type
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Overwrite with the version from git if found
INCLUDE("cmake/tools.cmake")
GET_VERSION(CORRYVRECKAN_VERSION)

# Print version
MESSAGE(STATUS "Building Corryvreckan version ${CORRYVRECKAN_VERSION}.")

# Gather information about build time:
STRING(TIMESTAMP BUILD_TIME "%Y-%m-%d, %H:%M:%S UTC" UTC)

# Define the project with the simple version
STRING(REGEX MATCH "([0-9.]+)+" SIMPLE_VERSION "${CORRYVRECKAN_VERSION}")
# Set languages to NONE to allow the documentation to be built without CXX compiler:
PROJECT(Corryvreckan VERSION ${SIMPLE_VERSION} LANGUAGES NONE)
# Access the project name (for install locations) in the source
ADD_DEFINITIONS(-DCORRYVRECKAN_PROJECT_NAME="${CMAKE_PROJECT_NAME}" -DCORRYVRECKAN_PROJECT_VERSION="${CORRYVRECKAN_VERSION}" -DCORRYVRECKAN_BUILD_TIME="${BUILD_TIME}")

INCLUDE(cmake/CPackConfig.cmake)
INCLUDE(CPack)

###############################################
# Setup the environment for the documentation #
###############################################

OPTION(BUILD_DOCS_ONLY "Build documentation only" OFF)

# Add targets for Doxygen code reference and LaTeX User manual
ADD_SUBDIRECTORY(doc)

# If only building docs, stop processing the rest of the CMake file:
IF(BUILD_DOCS_ONLY)
  RETURN()
ENDIF()

###############################
# Setup the build environment #
###############################

# Enable CXX as project language to perform compiler checks:
ENABLE_LANGUAGE(CXX)

# Additional packages to be searched for by cmake
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Configure the installation prefix to allow both local as system-wide installation
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories" FORCE)
ENDIF()

# Set up the RPATH so executables find the libraries even when installed in non-default location
SET(CMAKE_MACOSX_RPATH TRUE)
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# Add the automatically determined parts of the RPATH which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" IsSystemDir)
IF("${IsSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${IsSystemDir}" STREQUAL "-1")

# Include corry cmake functions
INCLUDE("cmake/corryvreckan.cmake")

#########################################
# Define build flags for Corryvreckan   #
#########################################

# Set standard build flags
SET(COMPILER_FLAGS -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wconversion -Wuseless-cast -Wctor-dtor-privacy -Wzero-as-null-pointer-constant -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Wshadow -Wformat-security -Wdeprecated -fdiagnostics-color=auto -Wheader-hygiene)

INCLUDE("cmake/compiler-flag-checks.cmake")

# Check for C++17 and require C++17 if available else fallback to C++14 (should currently be sufficient)
CHECK_CXX_COMPILER_FLAG(-std=c++17 SUPPORT_STD_CXX17)
IF(SUPPORT_STD_CXX17)
    SET(CMAKE_CXX_STANDARD 17)
ELSE()
    SET(CMAKE_CXX_STANDARD 14)
ENDIF()
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

# Set some debug flags
# FIXME: not using the flag checker now because it wrongly rejects a sanitizer flag..
IF(CMAKE_BUILD_TYPE MATCHES Debug AND ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")))
    MESSAGE(STATUS "Running debug build - Adding extra sanitizer flags")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
    # FIXME: remove earlier shared linker flags, because they can break the sanitizer debug build
    SET(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address -fsanitize=undefined")
ENDIF()

###################################
# Prerequisistes for Corryvreckan #
###################################

# Define the libraries
SET(CORRYVRECKAN_LIBRARIES "")

# ROOT is required for vector and persistency etc
FIND_PACKAGE(ROOT REQUIRED COMPONENTS Minuit Minuit2 Gui GenVector Geom Core Hist RIO NO_MODULE)
IF(NOT ROOT_FOUND)
    MESSAGE(FATAL_ERROR "Could not find ROOT, make sure to source the ROOT environment\n"
    "$ source YOUR_ROOT_DIR/bin/thisroot.sh")
ENDIF()

# Downgrade to C++14 if ROOT is not build with C++17 support
IF(ROOT_CXX_FLAGS MATCHES ".*std=c\\+\\+17.*")
    IF(NOT SUPPORT_STD_CXX17)
        MESSAGE(FATAL_ERROR "ROOT was built with C++17 support but current compiler doesn't support it")
    ENDIF()
ELSEIF(ROOT_CXX_FLAGS MATCHES ".*std=c\\+\\+1[14].*")
    SET(CMAKE_CXX_STANDARD 14)
ELSEIF(ROOT_CXX_FLAGS MATCHES ".*std=c\\+\\+.*")
    MESSAGE(FATAL_ERROR "ROOT was built with an unsupported C++ version: ${ROOT_CXX_FLAGS}")
ELSE()
    MESSAGE(FATAL_ERROR "Could not deduce ROOT's C++ version from build flags: ${ROOT_CXX_FLAGS}")
ENDIF()

# Check ROOT version
IF (NOT ${ROOT_VERSION} VERSION_GREATER "6.0")
    MESSAGE(FATAL_ERROR "ROOT versions below 6.0 are not supported")
ENDIF()

# Eigen3
FIND_PACKAGE(Eigen3 REQUIRED NO_MODULE)

# Prepare ROOT and EIGEN3 Targets if necessary:
CORRYVRECKAN_SETUP_ROOT_TARGETS()
CORRYVRECKAN_SETUP_EIGEN_TARGETS()

ADD_SUBDIRECTORY(3rdparty/GeneralBrokenLines)

# Set the dependencies
SET(CORRYVRECKAN_DEPS_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS})
SET(CORRYVRECKAN_DEPS_LIBRARIES Eigen3::Eigen ROOT::Core ROOT::GenVector ROOT::Geom ROOT::RIO ROOT::Hist ROOT::Minuit ROOT::Minuit2 GBL)

# Add "thisroot.sh" as runtime dependency for setup.sh file:
ADD_RUNTIME_DEP(thisroot.sh)


###################################
# Load cpp format and check tools #
###################################

# Set the clang-format version required by the CI for correct formatting:
SET(CLANG_FORMAT_VERSION "8")

# Set the source files to clang-format (FIXME: determine this better)
FILE(GLOB_RECURSE
     CHECK_CXX_SOURCE_FILES
        src/*.[tch]pp src/*.h src/*.C
     )
INCLUDE("cmake/clang-cpp-checks.cmake")

#########################################
# Define build targets for Corryvreckan #
#########################################

# Always include sources from top directory
INCLUDE_DIRECTORIES(src)

# Build objects library
ADD_SUBDIRECTORY(src/objects)
SET(CORRYVRECKAN_LIBRARIES ${CORRYVRECKAN_LIBRARIES} CorryvreckanObjects)

# Build core Corryvreckan library
ADD_SUBDIRECTORY(src/core)
SET(CORRYVRECKAN_LIBRARIES ${CORRYVRECKAN_LIBRARIES} CorryvreckanCore)


# Build required modules
ADD_SUBDIRECTORY(src/modules)

# Build the executable
ADD_SUBDIRECTORY(src/exec)

###################################
# Setup tests for allpix          #
###################################

# Enable testing
ENABLE_TESTING()

# Include all tests
ADD_SUBDIRECTORY(testing)

#############################
# Create a local setup file #
#############################

# Add compiler if necessary:
GET_FILENAME_COMPONENT(CMP_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
GET_FILENAME_COMPONENT(PARENT_PATH ${CMP_PATH} DIRECTORY)
IF(EXISTS ${PARENT_PATH}/setup.sh)
  LIST(APPEND CORRY_RUNTIME_DEPS "${PARENT_PATH}/setup.sh")
ENDIF()

# Build configuration string with commands
FOREACH(dep ${CORRY_RUNTIME_DEPS})
    SET(SETUP_FILE_DEPS "${SETUP_FILE_DEPS}source ${dep}\n")
ENDFOREACH()
FOREACH(dep ${CORRY_RUNTIME_LIBS})
    SET(SETUP_FILE_DEPS "${SETUP_FILE_DEPS}export LD_LIBRARY_PATH=\"${dep}:$LD_LIBRARY_PATH\"\n")
ENDFOREACH()
# Create setup file
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/setup.cmake.sh" "${CMAKE_CURRENT_BINARY_DIR}/setup/setup.sh" @ONLY)
