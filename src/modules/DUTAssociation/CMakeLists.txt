# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_UNIQUE_MODULE(MODULE_NAME)

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    DUTAssociation.cpp
    # ADD SOURCE FILES HERE...
)

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
