#############################################
# Doxygen target to generate API reference  #
#############################################

SET(DOCS_DOXYGEN_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/reference")

FIND_PACKAGE(Doxygen)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/reference/Doxyfile" "${DOCS_DOXYGEN_BINARY_DIR}/Doxyfile" @ONLY)

ADD_CUSTOM_TARGET(
    corry_docs_reference
    COMMAND Doxygen::doxygen "${DOCS_DOXYGEN_BINARY_DIR}/Doxyfile"
    DEPENDS Doxygen::doxygen
    WORKING_DIRECTORY "${DOCS_DOXYGEN_BINARY_DIR}"
    COMMENT "Generating API reference with Doxygen"
    VERBATIM)

###############################################
# LaTeX target to compile the PDF user manual #
###############################################

# Information and the manual on the UseLATEX.cmake package can be found here
# http://www.cmake.org/Wiki/CMakeUserUseLATEX

FIND_PACKAGE(LATEX)

# Only add target if we have LaTeX
IF(LATEX_COMPILER)
    MESSAGE(STATUS "Found LaTeX compiler.")

    find_program(IMAGEMAGICK NAMES magick convert)
    IF (NOT IMAGEMAGICK)
        # stop processing / don't add documentation targets
        MESSAGE(WARNING "ImageMagick is not found, but is required by UseLATEX.cmake!")
        RETURN()
    ENDIF()

    SET(LATEX_COMPILER_FLAGS "-shell-escape" "-interaction nonstopmode" "-halt-on-error" "-file-line-error" CACHE STRING "Flags passed to latex.")
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/LATEX.cmake)

    SET(HTLATEX_COMPILER_TEX4HT_FLAGS "usermanual/html/corryvreckan,charset=utf-8" CACHE STRING "Flags passed to Tex4HT" FORCE)
    SET(HTLATEX_COMPILER_TEX4HT_POSTPROCESSOR_FLAGS "-utf8 -cunihtf" CACHE STRING "Flags passed to the postprocessor of Tex4HT" FORCE)

    # Markdown files that should be included in the manual relative to the root of the repository
    SET(DOC_README_FILES
            jobsub/README.md
    )

    # Check for pandoc for markdown conversion
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/PANDOC.cmake)
    IF(PANDOC_EXECUTABLE)
        # Loop through all modules and fetch their README
        SET(module_dir ${PROJECT_SOURCE_DIR}/src/modules)
        FILE(GLOB subdirs RELATIVE ${module_dir} ${module_dir}/*)
        FOREACH(subdir ${subdirs})
            IF(EXISTS ${module_dir}/${subdir}/README.md AND NOT ${subdir} STREQUAL "Dummy")
                ADD_PANDOC_DOCUMENT(
                    ${CMAKE_BINARY_DIR}/usermanual/modules/${subdir}.tex
                    SOURCES              ${module_dir}/${subdir}/README.md
                    PRODUCT_DIRECTORY    usermanual/modules/
                    PANDOC_DIRECTIVES    -t latex --listings --biblatex
                    NO_EXPORT_PRODUCT
                )
                SET(module_tex_files ${module_tex_files} ${CMAKE_BINARY_DIR}/usermanual/modules/${subdir}.tex)
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
                SOURCES              ${PROJECT_SOURCE_DIR}/${doc_readme_file}
                PRODUCT_DIRECTORY    usermanual/md/${readme_parent_dir}/
                PANDOC_DIRECTIVES    -t latex --listings --biblatex
                NO_EXPORT_PRODUCT
            )

            SET(other_tex_files ${other_tex_files} ${CMAKE_BINARY_DIR}/usermanual/md/${readme_parent_dir}/${readme_dir_name}.tex)
        ENDFOREACH()
    ELSE()
        SET(module_tex_files)
        SET(other_tex_files "")
        MESSAGE(WARNING "No pandoc command, cannot add Markdown documentation to user manual")
    ENDIF()

    # Build array of LaTex files to add to the documentation
    LIST(LENGTH module_tex_files NUM_MODULES)
    MESSAGE(STATUS "Manual: adding ${NUM_MODULES} module descriptions")
    FOREACH(module_tex_file ${module_tex_files})
        IF(NOT CORRYVRECKAN_MODULE_FILES)
            SET(CORRYVRECKAN_MODULE_FILES "{${module_tex_file}")
        ELSE()
            SET(CORRYVRECKAN_MODULE_FILES "${CORRYVRECKAN_MODULE_FILES},${module_tex_file}")
        ENDIF()
    ENDFOREACH()
    IF(CORRYVRECKAN_MODULE_FILES)
        SET(CORRYVRECKAN_MODULE_FILES "${CORRYVRECKAN_MODULE_FILES}}")
    ELSE()
        SET(CORRYVRECKAN_MODULE_FILES "")
    ENDIF()

    # Check if we have biber and do not build if not
    IF(BIBER_COMPILER)
        SET(LATEX_OUTPUT_PATH ${CMAKE_BINARY_DIR}/usermanual)
        ADD_LATEX_DOCUMENT(
            usermanual/corryvreckan-manual.tex
            BIBFILES
            usermanual/references.bib
            USE_BIBLATEX
            IMAGES
            logo.png
            cc-by.png
            usermanual/figures/trackChi2ndof_goodexample.pdf
            usermanual/figures/residualX_goodexample.pdf
            usermanual/figures/correlationX_goodexample.pdf
            usermanual/figures/corrymanual_eventbuilding_datadriven.pdf
            usermanual/figures/corrymanual_eventbuilding_framebased.pdf
            usermanual/figures/reconstruction-chain-simple.png
            usermanual/figures/reconstruction-chain-complicated.png
            usermanual/figures/onlinemon.png
            INPUTS
            usermanual/chapters/introduction.tex
            usermanual/chapters/installation.tex
            usermanual/chapters/framework.tex
            usermanual/chapters/configuration.tex
            usermanual/chapters/eventbuilding.tex
            usermanual/chapters/onlinemonitoring.tex
            usermanual/chapters/howtoalign.tex
            usermanual/chapters/modules.tex
            usermanual/chapters/development.tex
            usermanual/chapters/testing.tex
            usermanual/chapters/tools.tex
            usermanual/config.tex
	    usermanual/chapters/objects.tex
            CONFIGURE
            usermanual/config.tex
            EXCLUDE_FROM_ALL
            DEPENDS ${module_tex_files} ${other_tex_files})
    ELSE()
        MESSAGE(WARNING "No biber command, cannot compile user manual.")
    ENDIF()
ELSE(LATEX_COMPILER)
    MESSAGE(WARNING "No LaTeX found, cannot compile user manual.")
ENDIF(LATEX_COMPILER)
