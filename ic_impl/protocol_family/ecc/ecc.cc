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

#include "interconnection/v2(rfc)/handshake/protocol_family/ecc.pb.h"

DEFINE_string(curve_type, "Curve25519", "elliptic curve type");
DEFINE_string(hash_strategy, "TRY_AND_REHASH", "hash to curve strategy");

namespace ic_impl::protocol_family::ecc {

int32_t SuggestedCurveType() {
  return util::GetFlagValue(
      org::interconnection::v2::protocol::CurveType_descriptor(), "CURVE_TYPE_",
      FLAGS_curve_type);
}

int32_t SuggestedHashStrategy() {
  return util::GetFlagValue(
      org::interconnection::v2::protocol::HashToCurveStrategy_descriptor(),
      "HASH_TO_CURVE_STRATEGY_", FLAGS_hash_strategy);
}

int32_t SuggestedPointOctetFormat() {
  return org::interconnection::v2::protocol::POINT_OCTET_FORMAT_UNCOMPRESSED;
}

}  // namespace ic_impl::protocol_family::ecc
