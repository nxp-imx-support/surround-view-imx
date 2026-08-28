# Quiet search LibXml2 package
set(SAVED_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "")
set(LibXml2_FIND_REQUIRED 0)
find_package(LibXml2 QUIET)
set(CMAKE_MODULE_PATH ${SAVED_CMAKE_MODULE_PATH})

# If not found, build LibXml2 libs from sources
if(NOT LIBXML2_INCLUDE_DIRS OR NOT LIBXML2_LIBRARIES)
    message(STATUS "LibXml2 package not found, build it from sources")
    # Third party source dir
    if(NOT THIRD_PARTY_DIR)
        set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/ThirdParty)
    endif()
    set(LIBXML2_INSTALL_DIR ${CMAKE_BINARY_DIR}/ThirdParty/LibXml2)
    ExternalProject_Add(libxml2
        GIT_REPOSITORY https://github.com/aosp-mirror/platform_external_libxml2.git
        GIT_TAG platform-tools-30.0.5
        GIT_SHALLOW 1
        SOURCE_DIR ${THIRD_PARTY_DIR}/LibXml2
        PATCH_COMMAND rm -f config.h
        CMAKE_ARGS ${FORWARD_CMAKE_PARAM} -DCMAKE_INSTALL_PREFIX=${LIBXML2_INSTALL_DIR} -DLIBXML2_WITH_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DLIBXML2_WITH_PYTHON=OFF
    )
    set(LIBXML2_INCLUDE_DIRS ${LIBXML2_INSTALL_DIR}/include/libxml2)
    set(LIBXML2_LIBRARIES
        -L${LIBXML2_INSTALL_DIR}/lib
        -lxml2
    )
    set(LibXml2_FOUND TRUE)
    mark_as_advanced(LIBXML2_INCLUDE_DIRS LIBXML2_LIBRARIES LibXml2_FOUND)
endif()

# Create an LibXml2::LibXml2 target
if(NOT TARGET LibXml2::LibXml2)
    add_library(LibXml2::LibXml2 INTERFACE IMPORTED)
    if(TARGET libxml2)
        # Path to include directory does not exist yet and is required by IMPORTED target
        file(MAKE_DIRECTORY ${LIBXML2_INCLUDE_DIRS})
        add_dependencies(LibXml2::LibXml2 libxml2)
    endif()
    target_include_directories(LibXml2::LibXml2 INTERFACE "${LIBXML2_INCLUDE_DIRS}")
    target_link_libraries(LibXml2::LibXml2 INTERFACE "${LIBXML2_LIBRARIES}")
endif()