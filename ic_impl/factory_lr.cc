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

#include "ic_impl/algo/lr/lr_handler.h"
#include "ic_impl/factory.h"

namespace ic_impl {

std::unique_ptr<AlgoV2Handler> LrHandlerFactory::CreateAlgoV2Handler(
    std::shared_ptr<IcContext> ic_ctx) {
  auto ctx = algo::lr::CreateLrContext(std::move(ic_ctx));
  return std::make_unique<algo::lr::LrHandler>(std::move(ctx));
}

}  // namespace ic_impl
