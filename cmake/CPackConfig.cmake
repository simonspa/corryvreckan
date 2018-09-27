# Packaging configuration
SET(CPACK_PACKAGE_NAME "corryvreckan")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Maelstrom for Your Test Beam Data")
SET(CPACK_PACKAGE_VENDOR "The Corryvreckan Authors")
SET(CPACK_PACKAGE_CONTACT "The Corryvreckan Authors <corryvreckan.info@cern.ch>")
SET(CPACK_PACKAGE_ICON "doc/logo_small.png")
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.md")

# Figure out, which system we are running on:
IF(DEFINED ENV{BUILD_FLAVOUR})
    SET(CPACK_SYSTEM_NAME $ENV{BUILD_FLAVOUR})
ELSE()
    SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
ENDIF()

# Set version dervied from project version, or "latest" for untagged:
IF(NOT TAG_FOUND)
    SET(CPACK_PACKAGE_VERSION "latest")
ELSE()
    SET(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
ENDIF()

SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")

# Configure the targets and components to include
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_COMPONENTS_ALL application modules)
SET(CPACK_INSTALLED_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/setup/;.")
