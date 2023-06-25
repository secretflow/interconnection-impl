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

#include "ic_impl/context.h"

#include "gflags/gflags.h"

#include "ic_impl/util.h"

#include "interconnection/v2(rfc)/handshake/entry.pb.h"

DEFINE_string(parties, "127.0.0.1:9530,127.0.0.1:9531",
              "server list, format: host1:port1[,host2:port2, ...].");
DEFINE_int32(rank, 0, "self rank");
DEFINE_int32(ic_version, 2, "handshake request version suggested");
DEFINE_string(algo, "ECDH_PSI", "algorithm suggested");
DEFINE_string(protocol_families, "ecc",
              "comma-separated list of protocol families");

namespace ic_impl {

namespace {

constexpr std::array<int32_t, 2> kSupportedVersions{1, 2};

int32_t SuggestedAlgo() {
  return util::GetFlagValue(org::interconnection::v2::AlgoType_descriptor(),
                            "ALGO_TYPE_",
                            util::GetParamEnv("algo", FLAGS_algo));
}

std::vector<int32_t> SuggestedProtocolFamilies() {
  return util::GetFlagValues(
      org::interconnection::v2::ProtocolFamily_descriptor(), "PROTOCOL_FAMILY_",
      util::GetParamEnv("protocol_families", FLAGS_protocol_families));
}

}  // namespace

std::shared_ptr<IcContext> CreateIcContext() {
  std::shared_ptr<IcContext> ic_ctx = std::make_shared<IcContext>();

  YACL_ENFORCE(util::IsFlagSupported(kSupportedVersions, FLAGS_ic_version));
  ic_ctx->version = FLAGS_ic_version;

  ic_ctx->algo = SuggestedAlgo();

  ic_ctx->protocol_families = SuggestedProtocolFamilies();

  YACL_ENFORCE(!ic_ctx->protocol_families.empty());

  ic_ctx->lctx = util::MakeLink(FLAGS_parties, FLAGS_rank);

  return ic_ctx;
}

}  // namespace ic_impl
