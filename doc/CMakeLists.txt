#############################################
# Doxygen target to generate API reference  #
#############################################

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/doc/reference/Doxyfile.in ${CMAKE_BINARY_DIR}/reference/Doxyfile @ONLY)
    ADD_CUSTOM_TARGET(reference
        ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/reference/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/reference
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
        )
ENDIF(DOXYGEN_FOUND)

###############################################
# LaTeX target to compile the PDF user manual #
###############################################

# Information and the manual on the UseLATEX.cmake package can be found here
# http://www.cmake.org/Wiki/CMakeUserUseLATEX

FIND_PACKAGE(LATEX)

# Only add target if we have LaTeX
IF(LATEX_COMPILER)
    MESSAGE(STATUS "Found LaTeX compiler.")
    SET(LATEX_COMPILER_FLAGS "-shell-escape" "-interaction nonstopmode" "-halt-on-error" "-file-line-error" CACHE STRING "Flags passed to latex.")
    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/LATEX.cmake)

    SET(HTLATEX_COMPILER_TEX4HT_FLAGS "usermanual/html/corryvreckan,charset=utf-8" CACHE STRING "Flags passed to Tex4HT" FORCE)
    SET(HTLATEX_COMPILER_TEX4HT_POSTPROCESSOR_FLAGS "-utf8 -cunihtf" CACHE STRING "Flags passed to the postprocessor of Tex4HT" FORCE)

    # Markdown files that should be included in the manual relative to the root of the repository
    SET(DOC_README_FILES
            jobsub/README.md
    )

    # Check for pandoc for markdown conversion
    INCLUDE(${CMAKE_SOURCE_DIR}/cmake/PANDOC.cmake)
    IF(PANDOC_EXECUTABLE)
        # Loop through all algorithms and fetch their README
        SET(algorithm_dir ${CMAKE_SOURCE_DIR}/src/algorithms)
        FILE(GLOB subdirs RELATIVE ${algorithm_dir} ${algorithm_dir}/*)
        FOREACH(subdir ${subdirs})
            IF(EXISTS ${algorithm_dir}/${subdir}/README.md AND NOT ${subdir} STREQUAL "Dummy")
                ADD_PANDOC_DOCUMENT(
                    ${CMAKE_BINARY_DIR}/usermanual/algorithms/${subdir}.tex
                    SOURCES              ${algorithm_dir}/${subdir}/README.md
                    PRODUCT_DIRECTORY    usermanual/algorithms/
                    PANDOC_DIRECTIVES    -t latex --listings --biblatex
                    NO_EXPORT_PRODUCT
                )
                SET(algorithm_tex_files ${algorithm_tex_files} ${CMAKE_BINARY_DIR}/usermanual/algorithms/${subdir}.tex)
            ENDIF()
        ENDFOREACH()

        # Generate other tex files from README we need and put them in the MD folder of the user manual
        SET(other_tex_files "")
        FOREACH(doc_readme_file ${DOC_README_FILES})
            GET_FILENAME_COMPONENT(readme_full_dir ${doc_readme_file} DIRECTORY)
            GET_FILENAME_COMPONENT(readme_parent_dir ${readme_full_dir} DIRECTORY)
            GET_FILENAME_COMPONENT(readme_dir_name ${readme_full_dir} NAME)

            ADD_PANDOC_DOCUMENT(
                ${CMAKE_BINARY_DIR}/usermanual/md/${readme_parent_dir}/${readme_dir_name}.tex
                SOURCES              ${CMAKE_SOURCE_DIR}/${doc_readme_file}
                PRODUCT_DIRECTORY    usermanual/md/${readme_parent_dir}/
                PANDOC_DIRECTIVES    -t latex --listings --biblatex
                NO_EXPORT_PRODUCT
            )

            SET(other_tex_files ${other_tex_files} ${CMAKE_BINARY_DIR}/usermanual/md/${readme_parent_dir}/${readme_dir_name}.tex)
        ENDFOREACH()
    ELSE()
        SET(algorithm_tex_files)
        SET(other_tex_files "")
        MESSAGE(WARNING "No pandoc command, cannot add Markdown documentation to user manual")
    ENDIF()

    # Build array of LaTex files to add to the documentation
    LIST(LENGTH algorithm_tex_files NUM_ALGORITHMS)
    MESSAGE(STATUS "Manual: adding ${NUM_ALGORITHMS} algorithm descriptions")
    FOREACH(algorithm_tex_file ${algorithm_tex_files})
        IF(NOT CORRYVRECKAN_ALGORITHM_FILES)
            SET(CORRYVRECKAN_ALGORITHM_FILES "{${algorithm_tex_file}")
        ELSE()
            SET(CORRYVRECKAN_ALGORITHM_FILES "${CORRYVRECKAN_ALGORITHM_FILES},${algorithm_tex_file}")
        ENDIF()
    ENDFOREACH()
    IF(CORRYVRECKAN_ALGORITHM_FILES)
        SET(CORRYVRECKAN_ALGORITHM_FILES "${CORRYVRECKAN_ALGORITHM_FILES}}")
    ELSE()
        SET(CORRYVRECKAN_ALGORITHM_FILES "")
    ENDIF()

    # Check if we have biber and do not build if not
    IF(BIBER_COMPILER)
        SET(LATEX_OUTPUT_PATH ${CMAKE_BINARY_DIR}/usermanual)
        ADD_LATEX_DOCUMENT(
            usermanual/corryvreckan-manual.tex
            IMAGES
            logo.png
            cc-by.png
            INPUTS
            usermanual/chapters/algorithms.tex
            usermanual/chapters/tools.tex
            usermanual/config.tex

            CONFIGURE
            usermanual/config.tex
            EXCLUDE_FROM_ALL
            DEPENDS ${algorithm_tex_files} ${other_tex_files})
    ELSE()
        MESSAGE(WARNING "No biber command, cannot compile user manual.")
    ENDIF()
ELSE(LATEX_COMPILER)
    MESSAGE(WARNING "No LaTeX found, cannot compile user manual.")
ENDIF(LATEX_COMPILER)
