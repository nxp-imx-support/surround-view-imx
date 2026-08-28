set(PARAM_CAM_HEIGHT 800)
set(PARAM_CAM_WIDTH 1280)
set(PARAM_CAM_FORMAT YUYV)
set(PARAM_CAM_DEWARP 1)
set(PARAM_CAM_1_SRC "v4l2:/dev/video3")
set(PARAM_CAM_2_SRC "v4l2:/dev/video4")
set(PARAM_CAM_3_SRC "v4l2:/dev/video5")
set(PARAM_CAM_4_SRC "v4l2:/dev/video6")
set(PARAM_DECTECTION 0)

set(PRESET_DIR "OV10635")
set(PRESETS
    ${PRESET_DIR}/CameraModels
    ${PRESET_DIR}/Chessboards
    ${PRESET_DIR}/Compensator
    ${PRESET_DIR}/Masks
    ${PRESET_DIR}/Vertices)
