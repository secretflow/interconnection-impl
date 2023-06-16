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

#include "ic_impl/algo/psi/v2/psi_context_v2.h"

#include "gflags/gflags.h"
#include "libspu/psi/bucket_psi.h"

#include "ic_impl/protocol_family/ecc/ecc.h"

#include "interconnection/v2(rfc)/handshake/protocol_family/ecc.pb.h"

DEFINE_string(in_path, "data.csv", "psi data in file path");
DEFINE_string(field_names, "id", "field names");
DEFINE_string(out_path, "", "psi out file path");
DEFINE_bool(should_sort, false, "whether sort psi result");
DEFINE_bool(precheck_input, false, "whether precheck input dataset");

DEFINE_int32(result_to_rank, -1, "which rank gets the result");

namespace ic_impl::algo::psi::v2 {

std::shared_ptr<EcdhPsiContext> CreateEcdhPsiContext(
    std::shared_ptr<IcContext> ic_context) {
  auto ctx = std::make_shared<EcdhPsiContext>();
  ctx->curve_type = protocol_family::ecc::SuggestedCurveType();
  ctx->hash_to_curve_strategy = protocol_family::ecc::SuggestedHashStrategy();
  ctx->point_octet_format = protocol_family::ecc::SuggestedPointOctetFormat();
  ctx->result_to_rank = FLAGS_result_to_rank;  // TODO: check

  ctx->ic_ctx = std::move(ic_context);

  return ctx;
}

std::unique_ptr<spu::psi::BucketPsi> CreateBucketPsi(
    const EcdhPsiContext &ctx) {
  spu::psi::BucketPsiConfig config;
  config.mutable_input_params()->set_path(FLAGS_in_path);
  auto field_list = absl::StrSplit(FLAGS_field_names, ',');
  config.mutable_input_params()->mutable_select_fields()->Add(
      field_list.begin(), field_list.end());
  config.mutable_input_params()->set_precheck(FLAGS_precheck_input);
  config.mutable_output_params()->set_path(FLAGS_out_path);
  config.mutable_output_params()->set_need_sort(FLAGS_should_sort);

  config.set_psi_type(spu::psi::PsiType::ECDH_PSI_2PC);

  if (ctx.result_to_rank == -1) {
    config.set_broadcast_result(true);
  } else {
    config.set_broadcast_result(false);
    config.set_receiver_rank(ctx.result_to_rank);
  }

  // TODO: support sm2
  if (ctx.curve_type ==
      org::interconnection::v2::protocol::CURVE_TYPE_CURVE25519) {
    config.set_curve_type(spu::psi::CurveType::CURVE_25519);
  }

  return std::make_unique<spu::psi::BucketPsi>(config, ctx.ic_ctx->lctx, true);
}

}  // namespace ic_impl::algo::psi::v2
