# Define module and return the generated name as MODULE_NAME
CORRYVRECKAN_DUT_MODULE(MODULE_NAME)
CORRYVRECKAN_DETECTOR_TYPE(${MODULE_NAME} "Timepix3")

# Add source files to library
CORRYVRECKAN_MODULE_SOURCES(${MODULE_NAME}
    AnalysisPowerPulsing.cpp
)

# Provide standard install target
CORRYVRECKAN_MODULE_INSTALL(${MODULE_NAME})
