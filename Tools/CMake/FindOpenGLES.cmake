# Find EGL
find_path (EGL_INCLUDE_DIR egl.h
    PATHS /usr/include/EGL
    DOC "EGL include directory"
)

find_library (EGL_LIBRARY libEGL.so
    PATHS /usr/lib
    DOC "EGL library"
)

# Find GLES3, if not found find GLES2
find_path (GLES_INCLUDE_DIR gl3.h
    PATHS /usr/include/GLES3
    DOC "GLES3 include directory"
)

find_library (GLES_LIBRARY libGLESv3.so
    PATHS /usr/lib
    DOC "GLES3 library"
)

# Find GLES2 if GLES3 is not found
if(NOT GLES_INCLUDE_DIR OR NOT GLES_LIBRARY)
    find_path (GLES_INCLUDE_DIR gl2.h
        PATHS /usr/include/GLES2
        DOC "GLES2 include directory"
    )

    find_library (GLES_LIBRARY libGLESv2.so
        PATHS /usr/lib
        DOC "GLES2 library"
    )
endif()

if (EGL_INCLUDE_DIR AND GLES_INCLUDE_DIR AND EGL_LIBRARY AND GLES_LIBRARY)
    set (OpenGLES_FOUND TRUE)
    message(STATUS "Found: OpenGLES and EGL: ${GLES_LIBRARY}")
else()
    set (OpenGLES_FOUND FALSE)
    message(STATUS "OpenGLES or EGL is not found")
endif ()

set(OpenGLES_INCLUDE_DIRS ${EGL_INCLUDE_DIR} ${GLES_INCLUDE_DIR})
set(OpenGLES_LIBRARIES ${EGL_LIBRARY} ${GLES_LIBRARY})

mark_as_advanced (OpenGLES_INCLUDE_DIRS OpenGLES_LIBRARIES OpenGLES_FOUND)

# Create an OpenGLES::OpenGLES target
if(NOT TARGET OpenGLES::OpenGLES)
    add_library(OpenGLES::OpenGLES INTERFACE IMPORTED)
    target_include_directories(OpenGLES::OpenGLES INTERFACE "${OpenGLES_INCLUDE_DIRS}")
    target_link_libraries(OpenGLES::OpenGLES INTERFACE "${OpenGLES_LIBRARIES}")
endif()