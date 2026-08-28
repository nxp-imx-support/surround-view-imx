# Quiet search Assimp package
set(SAVED_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "")
set(Assimp_FIND_REQUIRED 0)
find_package(Assimp QUIET)
set(CMAKE_MODULE_PATH ${SAVED_CMAKE_MODULE_PATH})

# If not found, search
if(NOT Assimp_INCLUDE_DIRS OR NOT Assimp_LIBRARIES)
    find_path (Assimp_INCLUDE_DIRS Importer.hpp
        PATHS /usr/include/assimp
        DOC "Assimp include directory"
    )
    find_library (Assimp_LIBRARIES libassimp.so
        PATHS /usr/lib
        DOC "Assimp library"
    )
    if (Assimp_INCLUDE_DIRS AND Assimp_LIBRARIES)
        set (Assimp_FOUND TRUE)
        mark_as_advanced(Assimp_INCLUDE_DIRS Assimp_LIBRARIES Assimp_FOUND)
    endif ()
endif()

# If not found, build Assimp libs from sources
if(NOT Assimp_INCLUDE_DIRS OR NOT Assimp_LIBRARIES)
    message(STATUS "Assimp package not found, build it from sources")
    # Third party source dir
    if(NOT THIRD_PARTY_DIR)
        set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/ThirdParty)
    endif()
    set(ASSIMP_INSTALL_DIR ${CMAKE_BINARY_DIR}/ThirdParty/Assimp)
    include(ExternalProject)
    ExternalProject_Add(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.0.1
        GIT_SHALLOW 1
        SOURCE_DIR ${THIRD_PARTY_DIR}/Assimp
        CMAKE_ARGS ${FORWARD_CMAKE_PARAM} -DCMAKE_INSTALL_PREFIX=${ASSIMP_INSTALL_DIR} -DASSIMP_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
    )
    set(Assimp_INCLUDE_DIRS ${ASSIMP_INSTALL_DIR}/include)
    set(Assimp_LIBRARIES
        -L${ASSIMP_INSTALL_DIR}/lib
        -lassimp
        -lIrrXML
    )
    set(Assimp_FOUND TRUE)
    mark_as_advanced(Assimp_INCLUDE_DIRS Assimp_LIBRARIES Assimp_FOUND)
endif()

# Create an Assimp::Assimp target
if(NOT TARGET Assimp::Assimp)
    add_library(Assimp::Assimp INTERFACE IMPORTED GLOBAL)
    if(TARGET assimp)
        # Path to include directory does not exist yet and is required by IMPORTED target
        file(MAKE_DIRECTORY ${Assimp_INCLUDE_DIRS})
        add_dependencies(Assimp::Assimp assimp)
    endif()
    target_include_directories(Assimp::Assimp INTERFACE "${Assimp_INCLUDE_DIRS}")
    target_link_libraries(Assimp::Assimp INTERFACE "${Assimp_LIBRARIES}")
endif()