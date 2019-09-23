# Define module
CORRYVRECKAN_GLOBAL_MODULE(MODULE_NAME)

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    FileWriter.cpp
)

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
