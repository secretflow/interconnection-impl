// Copyright 2023 Ant Group Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gflags/gflags.h"

#include "ic_impl/util.h"

#include "interconnection/v2(rfc)/handshake/op/sigmoid.pb.h"

DEFINE_string(sigmoid_mode, "Minimax_1", "Sigmoid approximation method");

namespace ic_impl::op::sigmoid {

int32_t SuggestedSigmoidMode() {
  return util::GetFlagValue(
      org::interconnection::v2::op::SigmoidMode_descriptor(), "SIGMOID_MODE_",
      FLAGS_sigmoid_mode);
}

}  // namespace ic_impl::op::sigmoid
