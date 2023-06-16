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

#include <memory>
#include <variant>

#include "ic_impl/handler.h"

namespace ic_impl {

struct IcContext;
class AlgoHandlerFactory;

class Party {
 public:
  explicit Party(std::shared_ptr<IcContext> ctx);

  ~Party();

  void Run();

 private:
  using HandshakeRequestVariant =
      std::variant<std::monostate, HandshakeRequestV1, HandshakeRequestV2>;

  std::unique_ptr<AlgoHandlerFactory> CreateHandlerFactory();

  std::vector<HandshakeRequestVariant> RecvHandshakeRequests();

  std::shared_ptr<IcContext> ctx_;
};

}  // namespace ic_impl
