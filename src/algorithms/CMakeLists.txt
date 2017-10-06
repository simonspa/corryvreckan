# Include the dependencies
INCLUDE_DIRECTORIES(SYSTEM ${CORRYVRECKAN_DEPS_INCLUDE_DIRS})

# Create Algorithms library
ADD_LIBRARY(CorryvreckanAlgorithms SHARED
    Alignment.C
    BasicTracking.C
    Clicpix2Correlator.C
    Clicpix2EventLoader.C
    ClicpixAnalysis.C
    CLICpixEventLoader.C
    DataDump.C
    DUTAnalysis.C
    EventDisplay.C
    FileReader.C
    FileWriter.C
    GenericAlgorithm.C
    GUI.C
    OnlineMonitor.C
    SpatialTracking.C
    TestAlgorithm.C
    Timepix1Clustering.C
    Timepix1Correlator.C
    Timepix1EventLoader.C
    Timepix3Clustering.C
    Timepix3EventLoader.C
    Timepix3MaskCreator.C
)

# Link the dependencies
TARGET_LINK_LIBRARIES(CorryvreckanAlgorithms ${CORRYVRECKANX_DEPS_LIBRARIES})
TARGET_LINK_LIBRARIES(CorryvreckanAlgorithms ${CORRYVRECKAN_LIBRARIES})

# Define compile-time library extension
TARGET_COMPILE_DEFINITIONS(CorryvreckanAlgorithms PRIVATE SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
# Link the DL libraries
TARGET_LINK_LIBRARIES(CorryvreckanAlgorithms ${CMAKE_DL_LIBS})

# Create standard install target
INSTALL(TARGETS CorryvreckanAlgorithms
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
