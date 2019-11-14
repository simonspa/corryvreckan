# include dependencies
INCLUDE_DIRECTORIES(SYSTEM ${CORRYVRECKAN_DEPS_INCLUDE_DIRS})

# create executable and link the libs
ADD_EXECUTABLE(corry corry.cpp)
TARGET_LINK_LIBRARIES(corry ${CORRYVRECKAN_LIBRARIES})

# prelink all module libraries
# NOTE: fixes both the RPATH problem as well as the TLS problems
# FIXME: should be removed when we have a better solution
TARGET_LINK_LIBRARIES(corry ${CORRYVRECKAN_MODULE_LIBRARIES})
TARGET_COMPILE_OPTIONS(corry PRIVATE ${CORRYVREKCAN_CXX_FLAGS})

# set install location
INSTALL(TARGETS corry EXPORT corry_install
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib)
