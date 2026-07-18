# ---------------------------------------------------------------------------
# API documentation via Doxygen. Enabled with -DLOB_BUILD_DOCS=ON; requires
# doxygen to be installed. Produces HTML under docs/html/.
# ---------------------------------------------------------------------------
find_package(Doxygen REQUIRED)

# doxygen_add_docs() generates a Doxyfile from these DOXYGEN_* variables, so we
# don't hand-maintain one. Only the non-default settings we care about are set.
set(DOXYGEN_GENERATE_HTML       YES)
set(DOXYGEN_GENERATE_LATEX      NO)
set(DOXYGEN_EXTRACT_ALL         YES)
set(DOXYGEN_EXTRACT_STATIC      YES)
set(DOXYGEN_JAVADOC_AUTOBRIEF   YES)
set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C NO)
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
set(DOXYGEN_SORT_MEMBER_DOCS    NO)
set(DOXYGEN_OUTPUT_DIRECTORY    ${CMAKE_CURRENT_SOURCE_DIR})
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md)
set(DOXYGEN_PROJECT_BRIEF       "C++20 Limit Order Book and Matching Engine")

doxygen_add_docs(docs
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md
    COMMENT "Generating API documentation with Doxygen")
