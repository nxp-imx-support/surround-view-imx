set(PARAM_CAM_HEIGHT 1280)
set(PARAM_CAM_WIDTH 1920)
set(PARAM_CAM_FORMAT YUYV)
set(PARAM_CAM_DEWARP 0)
set(PARAM_CAM_1_SRC "cam2:0")
set(PARAM_CAM_2_SRC "cam2:1")
set(PARAM_CAM_3_SRC "cam2:2")
set(PARAM_CAM_4_SRC "cam2:3")
set(PARAM_DECTECTION 0)

set(PRESET_DIR "OX03C10-dewarped")
set(PRESETS
    ${PRESET_DIR}/Chessboards
    ${PRESET_DIR}/Compensator
    ${PRESET_DIR}/Masks
    ${PRESET_DIR}/Vertices)
