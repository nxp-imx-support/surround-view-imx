set(PARAM_CAM_HEIGHT 1280)
set(PARAM_CAM_WIDTH 1920)
set(PARAM_CAM_FORMAT YUYV)
set(PARAM_CAM_DEWARP 0)
set(CAM_1 "/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@0/mx95mbcam@40")
set(CAM_2 "/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@1/mx95mbcam@40")
set(CAM_3 "/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@2/mx95mbcam@40")
set(CAM_4 "/base/soc/bus@42000000/i2c@42530000/max96724@27/i2c-mux/i2c@3/mx95mbcam@40")
set(G2D_DEWARP "imxvideoconvert_g2d video-warp-enable=true video-warp-coord-file=Content/Generated/CameraModels/ox03c_absolute_32bpp_dewarp_file-1920x1280.bin")
set(PARAM_CAM_1_SRC "gst:libcamerasrc camera-name=${CAM_1} ! ${G2D_DEWARP} ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_CAM_2_SRC "gst:libcamerasrc camera-name=${CAM_2} ! ${G2D_DEWARP} ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_CAM_3_SRC "gst:libcamerasrc camera-name=${CAM_3} ! ${G2D_DEWARP} ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_CAM_4_SRC "gst:libcamerasrc camera-name=${CAM_4} ! ${G2D_DEWARP} ! video/x-raw,format=YUY2 ! queue ! glupload ! appsink name=sv3dsink")
set(PARAM_DECTECTION 1)

set(PRESET_DIR "OX03C10-dewarped")
set(PRESETS
    ${PRESET_DIR}/CameraModels
    ${PRESET_DIR}/Chessboards
    ${PRESET_DIR}/Compensator
    ${PRESET_DIR}/Masks
    ${PRESET_DIR}/Vertices)
