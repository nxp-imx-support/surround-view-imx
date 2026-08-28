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

// Reuse code from tensorflow/lite/examples/label_image/label_image.cc

#include "DelegateProviders.hpp"
#include "Log.hpp"

namespace tflite {

DelegateProviders::DelegateProviders()
    : delegate_list_util_(&params_)
{
    delegate_list_util_.AddAllDelegateParams();
    delegate_list_util_.AppendCmdlineFlags(flags_);

    // Remove the "help" flag to avoid printing "--help=false"
    params_.RemoveParam("help");
    delegate_list_util_.RemoveCmdlineFlag(flags_, "help");
}

bool DelegateProviders::InitFromCmdlineArgs(int* argc, const char** argv)
{
    // Note if '--help' is in argv, the Flags::Parse return false,
    // see the return expression in Flags::Parse.
    return Flags::Parse(argc, argv, flags_);
}

void DelegateProviders::MergeSettingsIntoParams(const TfSettings& s)
{
    // Parse settings related to GPU delegate.
    // Note that GPU delegate does support OpenCL. 'gl_backend' was introduced
    // when the GPU delegate only supports OpenGL. Therefore, we consider
    // setting 'gl_backend' to true means using the GPU delegate.

    if (!params_.HasParam("external_delegate_path")) {
        LogWarning("External delegate execution provider isn't linked or External "
                   "delegate isn't supported on the platform!");
    }

    if (s.gl_backend) {
        if (!params_.HasParam("use_gpu")) {
            LogWarning("GPU delegate execution provider isn't linked or GPU "
                       "delegate isn't supported on the platform!");
        } else {
            params_.Set<bool>("use_gpu", true);
            // The parameter "gpu_inference_for_sustained_speed" isn't available for
            // iOS devices.
            if (params_.HasParam("gpu_inference_for_sustained_speed")) {
                params_.Set<bool>("gpu_inference_for_sustained_speed", true);
            }
            params_.Set<bool>("gpu_precision_loss_allowed", s.allow_fp16);
        }
    }

    // Parse settings related to NNAPI delegate.
    if (s.accel) {
        if (!params_.HasParam("use_nnapi")) {
            LogWarning("NNAPI delegate execution provider isn't linked or NNAPI "
                       "delegate isn't supported on the platform!");
        } else {
            params_.Set<bool>("use_nnapi", true);
            params_.Set<bool>("nnapi_allow_fp16", s.allow_fp16);
        }
    }

    // Parse settings related to Hexagon delegate.
    if (s.hexagon_delegate) {
        if (!params_.HasParam("use_hexagon")) {
            LogWarning("Hexagon delegate execution provider isn't linked or "
                       "Hexagon delegate isn't supported on the platform!");
        } else {
            params_.Set<bool>("use_hexagon", true);
            params_.Set<bool>("hexagon_profiling", false);
        }
    }

    // Parse settings related to XNNPACK delegate.
    if (s.xnnpack_delegate) {
        if (!params_.HasParam("use_xnnpack")) {
            LogWarning("XNNPACK delegate execution provider isn't linked or "
                       "XNNPACK delegate isn't supported on the platform!");
        } else {
            params_.Set<bool>("use_xnnpack", true);
            params_.Set<int32_t>("num_threads", s.number_of_threads);
        }
    }
}

std::vector<ProvidedDelegateList::ProvidedDelegate> DelegateProviders::CreateAllDelegates() const
{
    return delegate_list_util_.CreateAllRankedDelegates();
}

std::string DelegateProviders::GetHelpMessage(const std::string& cmdline) const
{
    return Flags::Usage(cmdline, flags_);
}

} // namespace tflite
