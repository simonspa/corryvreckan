CORRYVRECKAN_ENABLE_DEFAULT(OFF)

# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_UNIQUE_MODULE(MODULE_NAME)

# Peary and its CLICpix2 component is required for data decoding:
FIND_PACKAGE(Peary REQUIRED COMPONENTS CLICpix2)

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    Clicpix2EventLoader.cpp
)

TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${Peary_INCLUDE_DIRS})

# Link required Peary libraries
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Peary_LIBRARIES})

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
