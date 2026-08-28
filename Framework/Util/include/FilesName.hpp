/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

// Directories
#define CONTENT_DIR "Content/"
#define MESHES_DIR CONTENT_DIR "Meshes/"
#define MODELS_DIR CONTENT_DIR "Models/"
#define TEMPLATES_DIR CONTENT_DIR "Templates/"
#define CAMERA_INPUTS_DIR CONTENT_DIR "CameraInputs/"
#define CAMERA_MODELS_DIR CONTENT_DIR "CameraModels/"
#define AI_DIR CONTENT_DIR "AI/"

// Generated folder contain files generated during calibration
// Default sample files are provided for convenience
#define GENERATED_DIR CONTENT_DIR "Generated/"
#define VERTICES_DIR GENERATED_DIR "Vertices/"
#define COMPENSATOR_DIR GENERATED_DIR "Compensator/"
#define MASKS_DIR GENERATED_DIR "Masks/"
#define CHESSBOARDS_DIR GENERATED_DIR "Chessboards/"

// Files full path
#define SETTINGS_PATH_FILE CONTENT_DIR "Settings.xml"
#define MODEL_PATH_FILE CONTENT_DIR "Model.cfg"
#define FONT_PATH_FILE CONTENT_DIR "Font.png"
#define QUAD_INV_Y MESHES_DIR "QuadInvY"
#define AI_MODEL AI_DIR "ssd_mobilenet_v1_quant_95.tflite"
#define AI_LABELS AI_DIR "labels.txt"

// Files name without path
#define COMPENSATOR_FILE "Compensator"

// Files prefixes, file name without index
#define CAMERA_INPUT_PREFIX "CameraSrc"
#define CHESSBOARD_PREFIX "Chessboard"
#define FRAME_PREFIX "frame"
#define TEMPLATE_PREFIX "Template"
#define CORNER_PREFIX "Corner"
#define CALIB_RESULTS_PREFIX "calib_results_"
#define ARRAY_PREFIX "Array"
#define MASK_PREFIX "Mask"
