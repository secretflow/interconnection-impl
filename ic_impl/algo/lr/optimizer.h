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

#pragma once

#include <variant>

#include "interconnection/v2(rfc)/handshake/algos/optimizer.pb.h"

namespace ic_impl::algo::optimizer {

struct Optimizer {
  using OptimizerParam =
      std::variant<std::monostate,
                   org::interconnection::v2::algos::SgdOptimizer,
                   org::interconnection::v2::algos::MomentumOptimizer,
                   org::interconnection::v2::algos::AdagradOptimizer,
                   org::interconnection::v2::algos::AdadeltaOptimizer,
                   org::interconnection::v2::algos::RMSpropOptimizer,
                   org::interconnection::v2::algos::AdamOptimizer,
                   org::interconnection::v2::algos::AdamaxOptimizer,
                   org::interconnection::v2::algos::NadamOptimizer>;

  int32_t type;
  OptimizerParam param;
};

Optimizer SuggestedOptimizer();

}  // namespace ic_impl::algo::optimizer
