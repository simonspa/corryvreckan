# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_DETECTOR_MODULE(MODULE_NAME)
CORRYVRECKAN_DETECTOR_TYPE(${MODULE_NAME} "waveform")

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    EventLoaderWaveform.cpp
    DirectoryLoader.cpp
    # ADD SOURCE FILES HERE...
)

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
