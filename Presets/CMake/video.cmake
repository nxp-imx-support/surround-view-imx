set(PARAM_CAM_HEIGHT 800)
set(PARAM_CAM_WIDTH 1280)
set(PARAM_CAM_FORMAT RGB)
set(PARAM_CAM_DEWARP 1)
set(PARAM_CAM_1_SRC "vid:Content/CameraInputs/CameraSrc1.avi")
set(PARAM_CAM_2_SRC "vid:Content/CameraInputs/CameraSrc2.avi")
set(PARAM_CAM_3_SRC "vid:Content/CameraInputs/CameraSrc3.avi")
set(PARAM_CAM_4_SRC "vid:Content/CameraInputs/CameraSrc4.avi")
set(PARAM_DECTECTION 0)

set(PRESET_DIR "OV10635")
set(PRESETS
    ${PRESET_DIR}/CameraModels
    ${PRESET_DIR}/Chessboards
    ${PRESET_DIR}/Compensator
    ${PRESET_DIR}/Masks
    ${PRESET_DIR}/Vertices)
