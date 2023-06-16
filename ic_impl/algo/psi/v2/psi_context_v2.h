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

#include "ic_impl/context.h"

namespace spu::psi {
class BucketPsi;
}

namespace ic_impl::algo::psi::v2 {

struct EcdhPsiContext {  // TODO: change name to V2?
  int32_t curve_type;
  int32_t hash_to_curve_strategy;
  int32_t point_octet_format;
  int64_t item_num;
  int32_t result_to_rank;
  std::shared_ptr<IcContext> ic_ctx;
};

std::shared_ptr<EcdhPsiContext> CreateEcdhPsiContext(
    std::shared_ptr<IcContext>);

std::unique_ptr<spu::psi::BucketPsi> CreateBucketPsi(
    const EcdhPsiContext &);  // TODO: 放到别处

}  // namespace ic_impl::algo::psi::v2
