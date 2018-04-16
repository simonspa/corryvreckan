CORRYVRECKAN_ENABLE_DEFAULT(OFF)

# Define module and return the generated name as ALGORITHM_NAME
CORRYVRECKAN_ALGORITHM(ALGORITHM_NAME)

# Peary and its CLICpix2 component is required for data decoding:
FIND_PACKAGE(Peary REQUIRED COMPONENTS CLICpix2)

# Add source files to library
CORRYVRECKAN_ALGORITHM_SOURCES(${ALGORITHM_NAME}
    Clicpix2EventLoader.cpp
)

TARGET_INCLUDE_DIRECTORIES(${ALGORITHM_NAME} SYSTEM PRIVATE ${Peary_INCLUDE_DIRS}/devices/clicpix2 ${Peary_INCLUDE_DIRS}/peary/utils)

# Link required Peary libraries
TARGET_LINK_LIBRARIES(${ALGORITHM_NAME} ${Peary_LIBRARIES})

# Provide standard install target
CORRYVRECKAN_ALGORITHM_INSTALL(${ALGORITHM_NAME})
