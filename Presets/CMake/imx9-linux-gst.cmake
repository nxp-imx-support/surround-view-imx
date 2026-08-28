set(PARAM_CAM_HEIGHT 1280)
set(PARAM_CAM_WIDTH 1920)
set(PARAM_CAM_FORMAT YUYV)
set(PARAM_CAM_DEWARP 1)
set(PARAM_CAM_1_SRC "gst:libcamerasrc camera-name=/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@0/mx95mbcam@40 ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_CAM_2_SRC "gst:libcamerasrc camera-name=/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@1/mx95mbcam@40 ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_CAM_3_SRC "gst:libcamerasrc camera-name=/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@2/mx95mbcam@40 ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_CAM_4_SRC "gst:libcamerasrc camera-name=/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@3/mx95mbcam@40 ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_DECTECTION 1)

set(PRESET_DIR "OX03C10")
set(PRESETS
    ${PRESET_DIR}/CameraModels
    ${PRESET_DIR}/Chessboards
    ${PRESET_DIR}/Compensator
    ${PRESET_DIR}/Masks
    ${PRESET_DIR}/Vertices)
