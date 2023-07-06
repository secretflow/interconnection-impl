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

#include "ic_impl/protocol_family/ss/ss.h"

#include "gflags/gflags.h"

#include "ic_impl/util.h"

#include "interconnection/handshake/protocol_family/ss.pb.h"

DEFINE_string(protocol, "semi2k", "ss protocol suggested");

DEFINE_string(field, "64",
              "field type，32 for Ring32, 64 for Ring64, 128 for Ring128");
DEFINE_int32(fxp_bits, 18, "number of fraction bits of fixed-point number");
DEFINE_string(trunc_mode, "probabilistic", "truncation mode");
DEFINE_string(shard_serialize_format, "raw",
              "serialization format used in communicating secret shares");

DEFINE_bool(use_ttp, false, "whether use trusted third party's beaver service");
DEFINE_string(
    ttp_server_host, "127.0.0.1:9449",
    "trustedThirdParty beaver server's remote ip:port or load-balance uri");
DEFINE_string(ttp_session_id, "interconnection-root",
              "trustedThirdParty beaver server's session id");
DEFINE_int32(ttp_adjust_rank, 0, "which rank do adjust rpc call");

namespace ic_impl::protocol_family::ss {

namespace {

int32_t SuggestedProtocol() {
  return util::GetFlagValue(
      org::interconnection::v2::protocol::ProtocolKind_descriptor(),
      "PROTOCOL_KIND_", util::GetParamEnv("protocol", FLAGS_protocol));
}

int32_t SuggestedFieldType() {
  return util::GetFlagValue(
      org::interconnection::v2::protocol::FieldType_descriptor(), "FIELD_TYPE_",
      util::GetParamEnv("field", FLAGS_field));
}

int32_t SuggestedFxpBits() {
  return util::GetParamEnv("fxp_bits", FLAGS_fxp_bits);
}

int32_t SuggestedTruncationMode() {
  return util::GetFlagValue(
      org::interconnection::v2::protocol::TruncMode_descriptor(), "TRUNC_MODE_",
      util::GetParamEnv("trunc_mode", FLAGS_trunc_mode));
}

int32_t SuggestedShardSerializeFormat() {
  return util::GetFlagValue(
      org::interconnection::v2::protocol::ShardSerializeFormat_descriptor(),
      "SHARED_SERIALIZE_FORMAT_",
      util::GetParamEnv("shard_serialize_format",
                        FLAGS_shard_serialize_format));
}

bool SuggestedUseTtp() { return util::GetParamEnv("use_ttp", FLAGS_use_ttp); }

std::string SuggestedTtpServerHost() {
  return util::GetParamEnv("ttp_server_host", FLAGS_ttp_server_host);
}

std::string SuggestedTtpSessionId() {
  return util::GetParamEnv("ttp_session_id", FLAGS_ttp_session_id);
}

int32_t SuggestedTtpAdjustRank() {
  return util::GetParamEnv("ttp_adjust_rank", FLAGS_ttp_adjust_rank);
}

}  // namespace

SsProtocolParam SuggestedSsProtocolParam() {
  SsProtocolParam ss_param;
  ss_param.protocol = SuggestedProtocol();
  ss_param.field_type = SuggestedFieldType();
  ss_param.fxp_bits = SuggestedFxpBits();
  ss_param.trunc_mode = SuggestedTruncationMode();
  ss_param.shard_serialize_format = SuggestedShardSerializeFormat();

  return ss_param;
}

TrustedThirdPartyConfig SuggestedTtpConfig() {
  TrustedThirdPartyConfig ttp_config;
  ttp_config.use_ttp = SuggestedUseTtp();
  ttp_config.ttp_server_host = SuggestedTtpServerHost();
  ttp_config.ttp_session_id = SuggestedTtpSessionId();
  ttp_config.ttp_adjust_rank = SuggestedTtpAdjustRank();

  return ttp_config;
}

}  // namespace ic_impl::protocol_family::ss
