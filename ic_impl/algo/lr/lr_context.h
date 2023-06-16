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

#include "ic_impl/algo/lr/optimizer.h"
#include "ic_impl/context.h"
#include "ic_impl/protocol_family/ss/ss.h"

namespace ic_impl::algo::lr {

struct LrHyperParam {
  int64_t num_epoch{};
  int64_t batch_size{};
  int32_t last_batch_policy{};
  double l0_norm = 0.0;
  double l1_norm = 0.0;
  double l2_norm = 0.0;
};

struct LrIoParam {
  int64_t sample_size{};
  std::vector<int32_t> feature_nums{};
  int32_t label_rank = -1;
};

struct LrContext {
  LrHyperParam lr_param;

  LrIoParam io_param;

  int32_t sigmoid_mode{};

  protocol_family::ss::TrustedThirdPartyConfig ttp_config;

  protocol_family::ss::SsProtocolParam ss_param;

  optimizer::Optimizer optimizer;

  std::shared_ptr<IcContext> ic_ctx;

  bool HasLabel() const {
    return static_cast<int32_t>(ic_ctx->lctx->Rank()) == io_param.label_rank;
  }
};

std::shared_ptr<LrContext> CreateLrContext(std::shared_ptr<IcContext>);

}  // namespace ic_impl::algo::lr
