# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_DETECTOR_MODULE(MODULE_NAME)
CORRYVRECKAN_DETECTOR_TYPE(${MODULE_NAME} "CLICpix2")

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    EventLoaderCLICpix2.cpp
    CLICpix2/clicpix2_frameDecoder.cpp
    CLICpix2/clicpix2_frameDecoder_lfsr5.cpp
    CLICpix2/clicpix2_frameDecoder_lfsr8.cpp
    CLICpix2/clicpix2_frameDecoder_lfsr13.cpp
    CLICpix2/clicpix2_utilities.cpp
)

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
