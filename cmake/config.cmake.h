#pragma once

// CMake uses config.cmake.h to generate config.h within the build folder.
#ifndef CORRYVRECKAN_CONFIG_H
#define CORRYVRECKAN_CONFIG_H

#define PACKAGE_NAME "@CMAKE_PROJECT_NAME@"
#define PACKAGE_VERSION "@CORRYVRECKAN_LIB_VERSION@"
#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION

#endif
