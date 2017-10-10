# Include the dependencies
INCLUDE_DIRECTORIES(SYSTEM ${CORRYVRECKAN_DEPS_INCLUDE_DIRS})

# Create core library
ADD_LIBRARY(CorryvreckanCore SHARED
    Analysis.cpp
    Parameters.cpp
    utils/log.cpp
    utils/unit.cpp
    config/ConfigManager.cpp
    config/ConfigReader.cpp
    config/Configuration.cpp
    config/exceptions.cpp
)

# Link the dependencies
TARGET_LINK_LIBRARIES(CorryvreckanCore ${CORRYVRECKAN_DEPS_LIBRARIES})
TARGET_LINK_LIBRARIES(CorryvreckanCore ${CORRYVRECKAN_LIBRARIES})

# Define compile-time library extension
TARGET_COMPILE_DEFINITIONS(CorryvreckanCore PRIVATE SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
# Link the DL libraries
TARGET_LINK_LIBRARIES(CorryvreckanCore ${CMAKE_DL_LIBS})

# Create standard install target
INSTALL(TARGETS CorryvreckanCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
