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

#include "ic_impl/algo/psi/v2/psi_handler_v2.h"

#include "libspu/psi/bucket_psi.h"

namespace org::interconnection::v2::protocol {

bool operator<(const EcSuit &lhs, const EcSuit &rhs) {
  if (lhs.curve() < rhs.curve()) {
    return true;
  } else if (lhs.curve() > rhs.curve()) {
    return false;
  }

  if (lhs.hash() < rhs.hash()) {
    return true;
  } else if (lhs.hash() > rhs.hash()) {
    return false;
  }

  if (lhs.hash2curve_strategy() < rhs.hash2curve_strategy()) {
    return true;
  } else if (lhs.hash2curve_strategy() > rhs.hash2curve_strategy()) {
    return false;
  }

  return false;
}

}  // namespace org::interconnection::v2::protocol

namespace ic_impl::algo::psi::v2 {

using org::interconnection::v2::ALGO_TYPE_ECDH_PSI;
using org::interconnection::v2::PROTOCOL_FAMILY_ECC;
using org::interconnection::v2::algos::PsiDataIoProposal;
using org::interconnection::v2::algos::PsiDataIoResult;
using org::interconnection::v2::protocol::EccProtocolProposal;
using org::interconnection::v2::protocol::EccProtocolResult;
using org::interconnection::v2::protocol::EcSuit;

namespace {
std::vector<EccProtocolProposal> ExtractReqEccParams(
    const std::vector<HandshakeRequestV2> &requests) {
  int enum_field_num = HandshakeRequestV2::kProtocolFamiliesFieldNumber;
  int param_field_num = HandshakeRequestV2::kProtocolFamilyParamsFieldNumber;
  return util::ExtractParams<HandshakeRequestV2, EccProtocolProposal>(
      requests, enum_field_num, param_field_num, PROTOCOL_FAMILY_ECC);
}

std::optional<EccProtocolResult> ExtractRspEccParam(
    const HandshakeResponseV2 &response) {
  return ExtractRspPfParam<EccProtocolResult>(response, PROTOCOL_FAMILY_ECC);
}

std::set<EcSuit> IntersectEcSuits(
    const std::vector<EccProtocolProposal> &ecc_params) {
  int field_num = EccProtocolProposal::kEcSuitsFieldNumber;
  return util::IntersectParamItems<EccProtocolProposal, EcSuit>(ecc_params,
                                                                field_num);
}

std::set<int32_t> IntersectPointOctetFormats(
    const std::vector<EccProtocolProposal> &ecc_params) {
  int field_num = EccProtocolProposal::kPointOctetFormatsFieldNumber;
  return util::IntersectParamItems<EccProtocolProposal>(ecc_params, field_num);
}

}  // namespace

EcdhPsiV2Handler::EcdhPsiV2Handler(std::shared_ptr<EcdhPsiContext> ctx)
    : AlgoV2Handler(ctx->ic_ctx), ctx_(std::move(ctx)) {}

EcdhPsiV2Handler::~EcdhPsiV2Handler() = default;

bool EcdhPsiV2Handler::PrepareDataset() {
  bucket_psi_ = CreateBucketPsi(*ctx_);
  auto checker = bucket_psi_->CheckInput();

  ctx_->item_num = checker->data_count();

  return true;
}

HandshakeRequestV2 EcdhPsiV2Handler::BuildHandshakeRequest() {
  HandshakeRequestV2 request;

  request.set_version(ctx_->ic_ctx->version);
  request.set_requester_rank(ctx_->ic_ctx->lctx->Rank());
  request.add_supported_algos(ctx_->ic_ctx->algo);

  request.add_protocol_families(PROTOCOL_FAMILY_ECC);
  EccProtocolProposal ecc_param;
  ecc_param.add_supported_versions(1);
  auto *ec_suit = ecc_param.add_ec_suits();
  ec_suit->set_curve(ctx_->curve_type);
  ec_suit->set_hash(ctx_->hash_type);
  ec_suit->set_hash2curve_strategy(ctx_->hash_to_curve_strategy);
  ecc_param.add_point_octet_formats(ctx_->point_octet_format);
  ecc_param.set_support_point_truncation(ctx_->bit_length_after_truncated !=
                                         -1);
  request.add_protocol_family_params()->PackFrom(ecc_param);

  PsiDataIoProposal psi_io;
  psi_io.add_supported_versions(1);
  psi_io.set_item_num(ctx_->item_num);
  psi_io.set_result_to_rank(ctx_->result_to_rank);
  request.mutable_io_param()->PackFrom(psi_io);

  return request;
}

status::ErrorStatus EcdhPsiV2Handler::NegotiateHandshakeParams(
    const std::vector<HandshakeRequestV2> &requests) {
  auto status = NegotiateEccParams(requests);
  if (!status.ok()) {
    return status;
  }

  status = NegotiatePsiIoParams(requests);
  if (!status.ok()) {
    return status;
  }

  return status::OkStatus();
}

bool EcdhPsiV2Handler::NegotiateEcSuits(
    const std::vector<EccProtocolProposal> &ecc_params) {
  auto ec_suits = IntersectEcSuits(ecc_params);
  EcSuit ec_suit;
  ec_suit.set_curve(ctx_->curve_type);
  ec_suit.set_hash(ctx_->hash_type);
  ec_suit.set_hash2curve_strategy(ctx_->hash_to_curve_strategy);
  return ec_suits.find(ec_suit) != ec_suits.end();
}

bool EcdhPsiV2Handler::NegotiatePointOctetFormats(
    const std::vector<EccProtocolProposal> &ecc_params) {
  auto formats = IntersectPointOctetFormats(ecc_params);
  return formats.find(ctx_->point_octet_format) != formats.end();
}

bool EcdhPsiV2Handler::NegotiateBitLengthAfterTruncated(
    const std::vector<EccProtocolProposal> &ecc_params) {
  int field_num = EccProtocolProposal::kSupportPointTruncationFieldNumber;
  auto optional =
      util::AlignParamItem<EccProtocolProposal, bool>(ecc_params, field_num);
  bool support_point_trunc = optional.has_value() && optional.value();

  if (!support_point_trunc) {
    ctx_->bit_length_after_truncated = -1;
  }

  return true;
}

bool EcdhPsiV2Handler::NegotiateResultToRank(
    const std::vector<PsiDataIoProposal> &io_params) {
  int field_num = PsiDataIoProposal::kResultToRankFieldNumber;
  auto result_to_rank =
      util::AlignParamItem<PsiDataIoProposal, int32_t>(io_params, field_num);

  return ctx_->result_to_rank == result_to_rank;
}

status::ErrorStatus EcdhPsiV2Handler::NegotiateEccParams(
    const std::vector<HandshakeRequestV2> &requests) {
  auto ecc_params = ExtractReqEccParams(requests);
  if (ecc_params.empty()) {
    return status::InvalidRequestError("certain request has no ecc params");
  }

  if (!NegotiateEcSuits(ecc_params)) {
    return status::HandshakeRefusedError("negotiate ec suits failed");
  }

  if (!NegotiatePointOctetFormats(ecc_params)) {
    return status::HandshakeRefusedError("negotiate point octet format failed");
  }

  if (!NegotiateBitLengthAfterTruncated(ecc_params)) {
    return status::HandshakeRefusedError(
        "negotiate bit length after truncated failed");
  }

  return status::OkStatus();
}

status::ErrorStatus EcdhPsiV2Handler::NegotiatePsiIoParams(
    const std::vector<HandshakeRequestV2> &requests) {
  auto io_params = ExtractReqIoParams<PsiDataIoProposal>(requests);
  if (io_params.empty()) {
    return status::InvalidRequestError("certain request has no psi io params");
  }

  if (!NegotiateResultToRank(io_params)) {
    return status::HandshakeRefusedError("negotiate result_to_rank failed");
  }

  return status::OkStatus();
}

HandshakeResponseV2 EcdhPsiV2Handler::BuildHandshakeResponse() {
  HandshakeResponseV2 response;
  response.mutable_header()->set_error_code(org::interconnection::OK);
  response.set_algo(ALGO_TYPE_ECDH_PSI);

  response.add_protocol_families(PROTOCOL_FAMILY_ECC);
  EccProtocolResult ecc_param;
  auto ec_suit = ecc_param.mutable_ec_suit();
  ec_suit->set_curve(ctx_->curve_type);
  ec_suit->set_hash(ctx_->hash_type);
  ec_suit->set_hash2curve_strategy(ctx_->hash_to_curve_strategy);
  ecc_param.set_point_octet_format(ctx_->point_octet_format);
  ecc_param.set_bit_length_after_truncated(ctx_->bit_length_after_truncated);
  response.add_protocol_family_params()->PackFrom(ecc_param);

  PsiDataIoProposal psi_io;
  psi_io.set_item_num(ctx_->item_num);
  psi_io.set_result_to_rank(ctx_->result_to_rank);
  response.mutable_io_param()->PackFrom(psi_io);

  return response;
}

bool EcdhPsiV2Handler::ProcessHandshakeResponse(
    const HandshakeResponseV2 &response) {
  if (!AlgoV2Handler::ProcessHandshakeResponse(response)) {
    return false;
  }

  YACL_ENFORCE(response.algo() == ALGO_TYPE_ECDH_PSI);

  auto ecc_param_optional = ExtractRspEccParam(response);
  YACL_ENFORCE(ecc_param_optional.has_value());
  const auto &ecc_param = ecc_param_optional.value();
  YACL_ENFORCE(ecc_param.ec_suit().curve() == ctx_->curve_type);
  YACL_ENFORCE(ecc_param.ec_suit().hash() == ctx_->hash_type);
  YACL_ENFORCE(ecc_param.ec_suit().hash2curve_strategy() ==
               ctx_->hash_to_curve_strategy);
  YACL_ENFORCE(ecc_param.point_octet_format() == ctx_->point_octet_format);
  if (ecc_param.bit_length_after_truncated() != -1) {
    YACL_ENFORCE(ctx_->bit_length_after_truncated != -1);
  }
  ctx_->bit_length_after_truncated = ecc_param.bit_length_after_truncated();

  return true;
}

void EcdhPsiV2Handler::RunAlgo() {
  YACL_ENFORCE(bucket_psi_);

  try {
    spu::psi::PsiResultReport report;
    report.set_original_count(ctx_->item_num);
    uint64_t self_items_count = ctx_->item_num;
    auto indices = bucket_psi_->RunPsi(self_items_count);
    bucket_psi_->ProduceOutput(false, indices, report);

    SPDLOG_INFO("rank:{} original_count:{} intersection_count:{}",
                ctx_->ic_ctx->lctx->Rank(), report.original_count(),
                report.intersection_count());
  } catch (const std::exception &e) {
    SPDLOG_ERROR("run psi failed: {}", e.what());
  }
}

}  // namespace ic_impl::algo::psi::v2
