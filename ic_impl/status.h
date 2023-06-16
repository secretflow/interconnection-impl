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

#include "interconnection/common/header.pb.h"

namespace ic_impl::status {

class ErrorStatus {
 public:
  explicit ErrorStatus() : code_(org::interconnection::ErrorCode::OK) {}

  explicit ErrorStatus(org::interconnection::ErrorCode code,
                       std::string_view message)
      : code_(code), message_(message.data()) {}

  [[nodiscard]] bool ok() const {
    return code_ == org::interconnection::ErrorCode::OK;
  }

  [[nodiscard]] int32_t code() const { return code_; }

  [[nodiscard]] const std::string& message() const { return message_; }

 private:
  org::interconnection::ErrorCode code_;
  std::string message_;
};

inline ErrorStatus OkStatus() { return ErrorStatus(); }

ErrorStatus InvalidRequestError(std::string_view message);

ErrorStatus HandshakeRefusedError(std::string_view message);

ErrorStatus UnsupportedArgumentError(std::string_view message);

}  // namespace ic_impl::status
