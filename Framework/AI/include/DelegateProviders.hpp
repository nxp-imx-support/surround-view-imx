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

#include "TfSettings.hpp"

#include "tensorflow/lite/tools/delegates/delegate_provider.h"

#include <vector>

namespace tflite {

using ProvidedDelegateList = tflite::tools::ProvidedDelegateList;

class DelegateProviders
{
public:
    DelegateProviders();

    // Initialize delegate-related parameters from parsing command line arguments,
    // and remove the matching arguments from (*argc, argv). Returns true if all
    // recognized arg values are parsed correctly.
    bool InitFromCmdlineArgs(int* argc, const char** argv);

    // According to passed-in settings `s`, this function sets corresponding
    // parameters that are defined by various delegate execution providers. See
    // lite/tools/delegates/README.md for the full list of parameters defined.
    void MergeSettingsIntoParams(const TfSettings& s);

    // Create a list of TfLite delegates based on what have been initialized (i.e.
    // 'params_').
    std::vector<ProvidedDelegateList::ProvidedDelegate> CreateAllDelegates() const;

    std::string GetHelpMessage(const std::string& cmdline) const;

private:
    // Contain delegate-related parameters that are initialized from command-line
    // flags.
    tflite::tools::ToolParams params_;

    // A helper to create TfLite delegates.
    ProvidedDelegateList delegate_list_util_;

    // Contains valid flags
    std::vector<tflite::Flag> flags_;
};
} // namespace tflite
