# Define module and return the generated name as ALGORITHM_NAME
CORRYVRECKAN_ALGORITHM(ALGORITHM_NAME)

# Add source files to library
CORRYVRECKAN_ALGORITHM_SOURCES(${ALGORITHM_NAME}
    ClicpixAnalysis.cpp
    # ADD SOURCE FILES HERE...
)

# Provide standard install target
CORRYVRECKAN_ALGORITHM_INSTALL(${ALGORITHM_NAME})
