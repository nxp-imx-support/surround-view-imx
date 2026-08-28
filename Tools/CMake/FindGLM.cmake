# Quiet search GLM package
set(SAVED_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "")
set(GLM_FIND_REQUIRED 0)
find_package(GLM QUIET)
set(CMAKE_MODULE_PATH ${SAVED_CMAKE_MODULE_PATH})

# If not found, search
if(NOT GLM_INCLUDE_DIRS)
    find_path (GLM_INCLUDE_DIRS glm.hpp
        PATHS /usr/include/glm
        DOC "GLM include directory"
    )
    if (GLM_INCLUDE_DIRS)
        set (GLM_FOUND TRUE)
        mark_as_advanced(GLM_INCLUDE_DIRS GLM_FOUND)
        message(STATUS "Found: GLM: ${GLM_INCLUDE_DIRS}")
    endif ()
endif()

# If not found, fetch includes from github
if(NOT GLM_INCLUDE_DIRS)
    message(STATUS "GLM package not found, fetch from github")
    # Third party source dir
    if(NOT THIRD_PARTY_DIR)
        set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/ThirdParty)
    endif()
    set(GLM_INSTALL_DIR ${CMAKE_BINARY_DIR}/ThirdParty/glm)
    include(ExternalProject)
    ExternalProject_Add(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.8.4
        GIT_SHALLOW 1
        SOURCE_DIR ${THIRD_PARTY_DIR}/glm
        CMAKE_ARGS ${FORWARD_CMAKE_PARAM} -DCMAKE_INSTALL_PREFIX=${GLM_INSTALL_DIR}
    )
    set(GLM_INCLUDE_DIRS ${GLM_INSTALL_DIR}/include)
    set(GLM_FOUND TRUE)
    mark_as_advanced(GLM_INCLUDE_DIRS GLM_FOUND)
endif()

# Create an GLM::GLM target
if(NOT TARGET GLM::GLM)
    add_library(GLM::GLM INTERFACE IMPORTED GLOBAL)
    if(TARGET glm)
        # Path to include directory does not exist yet and is required by IMPORTED target
        file(MAKE_DIRECTORY ${GLM_INCLUDE_DIRS})
        add_dependencies(GLM::GLM glm)
    endif()
    target_include_directories(GLM::GLM INTERFACE "${GLM_INCLUDE_DIRS}")
endif()
