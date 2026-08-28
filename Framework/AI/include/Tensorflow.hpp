/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include "DelegateProviders.hpp"
#include "TfSettings.hpp"

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/model.h"

struct DetectedObject
{
    float Score;
    int Category;
    float Box[4];
};

namespace tflite {

class Tensorflow
{
public:
    Tensorflow();
    virtual ~Tensorflow();
    std::vector<DetectedObject> RunInference(uint8_t* data, uint32_t width, uint32_t height, uint32_t channels,
        float scoreMin, const std::vector<int>& categoryFilters);

protected:
    bool ReadLabelsFile(const std::string& file_name);
    bool LoadModel(const std::string file);

    TfSettings mSettings;
    DelegateProviders mDelegateProviders;
    std::vector<string> mLabels;
    std::unique_ptr<tflite::FlatBufferModel> mModel;
    std::unique_ptr<tflite::Interpreter> mInterpreter;

    std::vector<uint8_t> tmpData;
};
} // namespace tflite
