# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_GLOBAL_MODULE(MODULE_NAME)

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    JSONWriter.cpp
    # ADD SOURCE FILES HERE...
)

TARGET_LINK_LIBRARIES(${MODULE_NAME} ROOT::Tree ROOT::RIO)

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
