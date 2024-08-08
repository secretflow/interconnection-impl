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

#include <string>

namespace ic_impl::protocol_family::ss {

struct SsProtocolParam {
  int32_t protocol{};
  int32_t field_type{};
  int32_t fxp_bits{};
  int32_t trunc_mode{};
  int32_t shard_serialize_format{};
};

struct TrustedThirdPartyConfig {
  bool use_ttp;
  std::string ttp_server_host;
  int32_t ttp_server_version = 2;
  std::string ttp_asym_crypto_schema;
  std::string ttp_public_key;
  int32_t ttp_adjust_rank;
};

SsProtocolParam SuggestedSsProtocolParam();

TrustedThirdPartyConfig SuggestedTtpConfig();

}  // namespace ic_impl::protocol_family::ss
