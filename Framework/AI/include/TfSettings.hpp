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

namespace tflite {

struct TfSettings
{
    bool accel = false;
    bool allow_fp16 = false;
    bool gl_backend = false;
    bool hexagon_delegate = false;
    bool xnnpack_delegate = false;
    int loop_count = 1;
    float input_mean = 127.5f;
    float input_std = 127.5f;
    int number_of_threads = 4;
    int number_of_results = 5;
    int max_profiling_buffer_entries = 1024;
    int number_of_warmup_runs = 2;
};
} // namespace tflite
