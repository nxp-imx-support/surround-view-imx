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
/*
 * Copyright 2026 NXP
 *
 * Reuse code from Tensorflow label_image example.
 * Modified to apply inference on video capture buffer.
 * And use neutron delegate.
 */

#include "Tensorflow.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Time.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model_builder.h"

namespace tflite {

Tensorflow::Tensorflow()
{
    const char* argv[] = { "SV3D-Wayland", "--external_delegate_path=/usr/lib/libneutron_delegate.so" };
    int argc = sizeof(argv) / sizeof(char*);
    bool parse_result = mDelegateProviders.InitFromCmdlineArgs(&argc, const_cast<const char**>(argv));
    if (!parse_result) {
        LogError("Parsing TFlite Arguments failed");
    }

    mSettings.loop_count = 1;

    mDelegateProviders.MergeSettingsIntoParams(mSettings);

    if (false == ReadLabelsFile(AI_LABELS)) {
        LogError("Could not read labels");
    }
    if (false == LoadModel(AI_MODEL)) {
        LogError("Could not read model");
    }
}

Tensorflow::~Tensorflow()
{
    // Destroy the interpreter earlier than delegates objects.
    mInterpreter.reset();
}

bool Tensorflow::ReadLabelsFile(const std::string& file_name)
{
    std::ifstream file(file_name);
    if (!file) {
        LogError("Labels file %s not found", file_name.c_str());
        return false;
    }
    mLabels.clear();
    std::string line;
    while (std::getline(file, line)) {
        mLabels.push_back(line);
    }
    const int padding = 16;
    while (mLabels.size() % padding) {
        mLabels.emplace_back();
    }
    return true;
}

bool Tensorflow::LoadModel(const string file)
{
    mModel = tflite::FlatBufferModel::BuildFromFile(file.c_str());
    if (!mModel) {
        LogError("Failed to mmap model %s", file.c_str());
        return false;
    }
    LogInfo("Loaded model %s", file.c_str());
    mModel->error_reporter();

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder(*mModel, resolver)(&mInterpreter);
    if (!mInterpreter) {
        LogError("Failed to construct mInterpreter");
        exit(-1);
    }

    mInterpreter->SetAllowFp16PrecisionForFp32(mSettings.allow_fp16);

    LogDebug("Tensors size: %lu", mInterpreter->tensors_size());
    LogDebug("Nodes size:  %lu", mInterpreter->nodes_size());
    LogDebug("Inputs:  %lu", mInterpreter->inputs().size());
    LogDebug("Input(0) name: %s", mInterpreter->GetInputName(0));

#if LOG_DEBUG == 1
    for (int i = 0; i < mInterpreter->tensors_size(); i++) {
        if (mInterpreter->tensor(i)->name) {
            LogDebug("%d: %s, %ld, %d, %f, %d", i, mInterpreter->tensor(i)->name, mInterpreter->tensor(i)->bytes,
                mInterpreter->tensor(i)->type, mInterpreter->tensor(i)->params.scale,
                mInterpreter->tensor(i)->params.zero_point);
        }
    }
#endif

    if (mSettings.number_of_threads != -1) {
        mInterpreter->SetNumThreads(mSettings.number_of_threads);
    }

    LogDebug("Input: %d", mInterpreter->inputs()[0]);
    LogDebug("Number of inputs: %lu", mInterpreter->inputs().size());
    LogDebug("Number of outputs: %lu", mInterpreter->outputs().size());

    auto delegates = mDelegateProviders.CreateAllDelegates();
    for (auto& delegate : delegates) {
        const auto delegate_name = delegate.provider->GetName();
        LogInfo("Delegate_name: %s", delegate_name.c_str());
        if (mInterpreter->ModifyGraphWithDelegate(std::move(delegate.delegate)) != kTfLiteOk) {
            LogError("Failed to apply %s delegate", delegate_name.c_str());
        } else {
            LogInfo("Applied %s delegate", delegate_name.c_str());
        }
    }

    if (mInterpreter->AllocateTensors() != kTfLiteOk) {
        LogError("Failed to allocate tensors!");
        exit(-1);
    }

    return true;
}

std::vector<DetectedObject> Tensorflow::RunInference(uint8_t* data, uint32_t width, uint32_t height, uint32_t channels,
    float scoreMin, const std::vector<int>& categoryFilters)
{
    struct timeval start_time, stop_time;

    int input = mInterpreter->inputs()[0];
    mInterpreter->tensor(input)->data.raw = reinterpret_cast<char*>(data);

    // Invoke
    Time start = Time::Get();
    if (mInterpreter->Invoke() != kTfLiteOk) {
        LogError("Failed to invoke tflite");
    }
    Time stop = Time::Get();
    LogDebug("Time: %f ms", (stop - start).GetMs());

    std::vector<DetectedObject> top_results;
    auto boxes = mInterpreter->typed_output_tensor<float>(0);
    auto categories = mInterpreter->typed_output_tensor<float>(1);
    auto scores = mInterpreter->typed_output_tensor<float>(2);

    for (int i = 0; i < 10; ++i) {
        if (scores[i] >= scoreMin && (categoryFilters.empty() || (std::find(categoryFilters.begin(), categoryFilters.end(), categories[i]) != categoryFilters.end()))) {
            DetectedObject object;
            object.Score = scores[i];
            object.Category = categories[i];
            std::memcpy(object.Box, &boxes[4 * i], sizeof(object.Box));
            LogDebug("Detected:%s(%d), Score:%f", mLabels[object.Category].c_str(), object.Category, object.Score);
            LogDebug("\tBox:min(y,x)=(%f,%f) max(y,x)=(%f,%f)", object.Box[0], object.Box[1], object.Box[2],
                object.Box[3]);
            top_results.push_back(object);
        }
    }

    return top_results;
}
} // namespace tflite
