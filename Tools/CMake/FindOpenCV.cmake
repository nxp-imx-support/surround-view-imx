# Search includes and libs first
find_path (OpenCV_INCLUDE_DIR opencv2/core.hpp
    PATHS /usr/include/opencv4
    DOC "OpenCV include directory"
)

find_library (OpenCV_LIBRARY libopencv_core.so
    PATHS /usr/lib
    DOC "OpenCV library"
)

set(OPENCV_LIBS
    -lopencv_calib3d -lopencv_core -lopencv_dnn -lopencv_features2d -lopencv_flann
    -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_ml -lopencv_objdetect
    -lopencv_photo -lopencv_stitching -lopencv_video -lopencv_videoio
)

if (OpenCV_INCLUDE_DIR AND OpenCV_LIBRARY)
    message(STATUS "Found: OpenCV")
    set(OpenCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIR})
    set(OpenCV_LIBRARIES ${OPENCV_LIBS})
    mark_as_advanced(OpenCV_INCLUDE_DIRS OpenCV_LIBRARIES)
endif ()

# If not found, build OpenCV libs from sources
if(NOT OpenCV_INCLUDE_DIRS OR NOT OpenCV_LIBRARIES)
    message(STATUS "OpenCV package not found, build it from sources")
    # Third party source dir
    if(NOT THIRD_PARTY_DIR)
        set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/ThirdParty)
    endif()
    set(OPENCV_INSTALL_DIR ${CMAKE_BINARY_DIR}/ThirdParty/OpenCV)
    include(ExternalProject)
    ExternalProject_Add(opencv
        GIT_REPOSITORY https://github.com/opencv/opencv.git
        GIT_TAG 4.12.0
        GIT_SHALLOW 1
        SOURCE_DIR ${THIRD_PARTY_DIR}/OpenCV
        CMAKE_ARGS
            ${FORWARD_CMAKE_PARAM}
            -DCMAKE_INSTALL_PREFIX=${OPENCV_INSTALL_DIR}
            -DBUILD_ANDROID_EXAMPLES=OFF
            -DBUILD_ANDROID_PROJECTS=OFF
            -DBUILD_TESTS=OFF
            -DBUILD_PERF_TESTS=OFF
            -DWITH_KLEIDICV=OFF
            -DWITH_GSTREAMER=OFF
            -DWITH_OPENJPEG=OFF
            -DWITH_JASPER=OFF
    )
    set(OpenCV_INCLUDE_DIRS ${OPENCV_INSTALL_DIR}/sdk/native/jni/include)
    set(OpenCV_LIBRARIES
        -L${OPENCV_INSTALL_DIR}/sdk/native/staticlibs/${ANDROID_ABI}/
        ${OPENCV_LIBS}
        -L${OPENCV_INSTALL_DIR}/sdk/native/3rdparty/libs/${ANDROID_ABI}/
        -lcpufeatures -lIlmImf -littnotify -llibjpeg-turbo -llibpng -llibprotobuf -llibtiff -llibwebp -ltegra_hal
    )
    mark_as_advanced(OpenCV_INCLUDE_DIRS OpenCV_LIBRARIES)
endif()

# Here OpenCV is found or built, default package does not set OpenCV_FOUND to TRUE
set(OpenCV_FOUND TRUE)
mark_as_advanced(OpenCV_FOUND)

# Create an OpenCV::OpenCV target
if(NOT TARGET OpenCV::OpenCV)
    add_library(OpenCV::OpenCV INTERFACE IMPORTED)
    if(TARGET opencv)
        # Path to include directory does not exist yet and is required by IMPORTED target
        file(MAKE_DIRECTORY ${OpenCV_INCLUDE_DIRS})
        add_dependencies(OpenCV::OpenCV opencv)
    endif()
    target_include_directories(OpenCV::OpenCV INTERFACE "${OpenCV_INCLUDE_DIRS}")
    target_link_libraries(OpenCV::OpenCV INTERFACE "${OpenCV_LIBRARIES}")
endif()