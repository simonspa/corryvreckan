FUNCTION(GET_TEST_REGEX INP OUTPUT_PASS OUTPUT_FAIL)
    IF(NOT OUTPUT_PASS_)
        FILE(STRINGS ${INP} OUTPUT_PASS_ REGEX "#PASS ")
    ENDIF()
    IF(NOT OUTPUT_FAIL_)
        FILE(STRINGS ${INP} OUTPUT_FAIL_ REGEX "#FAIL ")
    ENDIF()

    # Check for number of arguments - should only be one:
    LIST(LENGTH OUTPUT_PASS_ LISTCOUNT_PASS)
    LIST(LENGTH OUTPUT_FAIL_ LISTCOUNT_FAIL)
    IF(LISTCOUNT_PASS GREATER 1)
        MESSAGE(FATAL_ERROR "More than one PASS expressions defined in test ${INP}")
    ENDIF()
    IF(LISTCOUNT_FAIL GREATER 1)
        MESSAGE(FATAL_ERROR "More than one FAIL expressions defined in test ${INP}")
    ENDIF()

    # Escape possible regex patterns in the expected output:
    ESCAPE_REGEX("${OUTPUT_PASS_}" OUTPUT_PASS_)
    ESCAPE_REGEX("${OUTPUT_FAIL_}" OUTPUT_FAIL_)

    SET(${OUTPUT_PASS} "${OUTPUT_PASS_}" PARENT_SCOPE)
    SET(${OUTPUT_FAIL} "${OUTPUT_FAIL_}" PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(ESCAPE_REGEX INP OUTPUT)
    # Escape possible regex patterns in the expected output:
    STRING(REPLACE "#PASS " "" _TMP_STR "${INP}")
    STRING(REPLACE "#FAIL " "" _TMP_STR "${_TMP_STR}")
    STRING(REGEX REPLACE "([][+-.*()^])" "\\\\\\1" _TMP_STR "${_TMP_STR}")
    SET(${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(ADD_CORRYVRECKAN_TEST TEST)
    # Allow the test to specify additional module CLI parameters:
    FILE(STRINGS ${TEST} OPTS REGEX "#OPTION ")
    FOREACH(OPT ${OPTS})
        STRING(REPLACE "#OPTION " "" OPT "${OPT}")
        SET(CLIOPTIONS "${CLIOPTIONS} -o ${OPT}")
    ENDFOREACH()
    # Allow the test to specify additional geometry CLI parameters:
    FILE(STRINGS ${TEST} OPTS REGEX "#DETOPTION ")
    FOREACH(OPT ${OPTS})
        STRING(REPLACE "#DETOPTION " "" OPT "${OPT}")
        SET(CLIOPTIONS "${CLIOPTIONS} -g ${OPT}")
    ENDFOREACH()

    # Some tests might depend on others:
    FILE(STRINGS ${TEST} DEPENDENCY REGEX "#DEPENDS ")

    # Read the datasets from the configuration file:
    FILE(STRINGS ${TEST} OPTS REGEX "#DATASET ")
    LIST(LENGTH OPTS LISTCOUNT_DATA)

    # Either we need a data set to operate on or another test output:
    IF(LISTCOUNT_DATA LESS 1 AND NOT DEPENDENCY)
        MESSAGE(FATAL_ERROR "No dataset defined for test \"${TEST}\"")
    ENDIF()
    FOREACH(OPT ${OPTS})
        STRING(REPLACE "#DATASET " "" OPT "${OPT}")
        SET(DATASETS "${DATASETS} ${OPT}")
    ENDFOREACH()

    ADD_TEST(NAME ${TEST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh "${DATASETS}" "${CMAKE_INSTALL_PREFIX}/bin/corry -c ${CMAKE_CURRENT_SOURCE_DIR}/${TEST} ${CLIOPTIONS}"
    )

    # Parse configuration file for pass/fail conditions:
    GET_TEST_REGEX(${TEST} EXPRESSIONS_PASS EXPRESSIONS_FAIL)
    IF(EXPRESSIONS_PASS)
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES PASS_REGULAR_EXPRESSION "${EXPRESSIONS_PASS}")
    ENDIF()
    IF(EXPRESSIONS_FAIL)
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES FAIL_REGULAR_EXPRESSION "${EXPRESSIONS_FAIL}")
    ENDIF()

    IF(DEPENDENCY)
        STRING(REPLACE "#DEPENDS " "" DEPENDENCY "${DEPENDENCY}")
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES DEPENDS "${DEPENDENCY}")
    ENDIF()

    # Add individual timeout criteria:
    FILE(STRINGS ${TEST} TESTTIMEOUT REGEX "#TIMEOUT ")
    IF(TESTTIMEOUT)
        STRING(REPLACE "#TIMEOUT " "" TESTTIMEOUT "${TESTTIMEOUT}")
        SET_TESTS_PROPERTIES(${TEST} PROPERTIES TIMEOUT "${TESTTIMEOUT}")
    ENDIF()

ENDFUNCTION()

###################################
# Add data-driven framework tests #
###################################

OPTION(TEST_CORE "Perform data-driven tests to ensure framework functionality?" ON)

IF(TEST_CORE)
    FILE(GLOB TEST_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test_*.conf)
    MESSAGE(STATUS "Tests: data-driven framework functionality")
    FOREACH(TEST ${TEST_LIST})
        ADD_CORRYVRECKAN_TEST(${TEST})
        MESSAGE(STATUS "  - Test \"${TEST}\"")
    ENDFOREACH()
ELSE()
    MESSAGE(STATUS "Unit tests: data-driven framework functionality tests deactivated.")
ENDIF()
