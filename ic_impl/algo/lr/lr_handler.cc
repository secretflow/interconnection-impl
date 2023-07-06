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

#include <cstdlib>
#include <fstream>

#include "absl/functional/bind_front.h"
#include "absl/strings/match.h"
#include "gflags/gflags.h"
#include "libspu/core/config.h"
#include "libspu/core/encoding.h"
#include "libspu/kernel/hal/hal.h"
#include "libspu/mpc/aby3/type.h"
#include "libspu/mpc/factory.h"
#include "libspu/mpc/semi2k/type.h"
#include "nlohmann/json.hpp"
#include "xtensor/xcsv.hpp"
#include "xtensor/xio.hpp"

DEFINE_string(dataset, "data.csv", "dataset file, only csv is supported");
DEFINE_int32(skip_rows, 1, "skip number of rows from dataset");
DEFINE_string(lr_output, "/tmp/sslr_result", "full path name of output file");
DECLARE_int32(rank);

namespace ic_impl::algo::lr {

using org::interconnection::v2::ALGO_TYPE_SS_LR;
using org::interconnection::v2::OP_TYPE_SIGMOID;
using org::interconnection::v2::PROTOCOL_FAMILY_SS;

using org::interconnection::v2::algos::LrDataIoProposal;
using org::interconnection::v2::algos::LrDataIoResult;
using org::interconnection::v2::algos::LrHyperparamsProposal;
using org::interconnection::v2::algos::LrHyperparamsResult;

using org::interconnection::v2::op::SIGMOID_MODE_MINIMAX_1;
using org::interconnection::v2::op::SigmoidParamsProposal;
using org::interconnection::v2::op::SigmoidParamsResult;

using org::interconnection::v2::protocol::CRYPTO_TYPE_AES128_CTR;
using org::interconnection::v2::protocol::PrgConfigProposal;

using org::interconnection::v2::protocol::TripleConfigProposal;

using org::interconnection::v2::protocol::PROTOCOL_KIND_ABY3;
using org::interconnection::v2::protocol::PROTOCOL_KIND_SEMI2K;
using org::interconnection::v2::protocol::SSProtocolProposal;
using org::interconnection::v2::protocol::SSProtocolResult;
using org::interconnection::v2::protocol::TRUNC_MODE_PROBABILISTIC;

namespace {

std::optional<std::string> GetIoFileNameFromEnv(bool input) {
  char* host_url = std::getenv("system.storage");
  if (!host_url || !absl::StartsWith(host_url, "file://")) {
    host_url = std::getenv("system.storage.host.url");
    if (!host_url || !absl::StartsWith(host_url, "file://")) {
      return std::nullopt;
    }
  }
  std::string_view root_path = host_url + 6;

  char* json_str = nullptr;
  if (input) {
    json_str = std::getenv("runtime.component.input.train_data");
  } else {
    json_str = std::getenv("runtime.component.output.train_data");
  }

  if (!json_str) {
    return std::nullopt;
  }

  auto json_object = nlohmann::json::parse(json_str);
  std::string relative_path = json_object.at("namespace");
  std::string file_name = json_object.at("name");

  std::string absolute_path = absl::StrCat(root_path, "/", relative_path);
  if (!input) {
    system(absl::StrCat("mkdir -p ", absolute_path).c_str());
  }

  return absl::StrCat(absolute_path, "/", file_name);
}

inline std::optional<std::string> GetInputFileNameFromEnv() {
  return GetIoFileNameFromEnv(true);
}

inline std::optional<std::string> GetOutputFileNameFromEnv() {
  return GetIoFileNameFromEnv(false);
}

std::string GetInputFileName() {
  std::string_view input_file_name = FLAGS_dataset;
  auto input_optional = GetInputFileNameFromEnv();
  if (input_optional.has_value()) {
    input_file_name = input_optional.value();
  }

  return input_file_name.data();
}

std::string GetOutputFileName() {
  std::string output_file_name = absl::StrCat(FLAGS_lr_output, ".", FLAGS_rank);
  auto output_optional = GetOutputFileNameFromEnv();
  if (output_optional.has_value()) {
    output_file_name = output_optional.value();
  }

  return output_file_name;
}

std::unique_ptr<xt::xarray<float>> ReadDataset() {
  std::string input_file = GetInputFileName();

  std::ifstream file(input_file);
  YACL_ENFORCE(file, "open file={} failed", input_file);

  return std::make_unique<xt::xarray<float>>(
      xt::load_csv<float>(file, ',', FLAGS_skip_rows));
}

std::vector<LrHyperparamsProposal> ExtractReqLrParams(
    const std::vector<HandshakeRequestV2>& requests) {
  return ExtractReqAlgoParams<LrHyperparamsProposal>(requests, ALGO_TYPE_SS_LR);
}

std::vector<SigmoidParamsProposal> ExtractReqSigmoidParams(
    const std::vector<HandshakeRequestV2>& requests) {
  return ExtractReqOpParams<SigmoidParamsProposal>(requests, OP_TYPE_SIGMOID);
}

std::optional<SigmoidParamsResult> ExtractRspSigmoidParam(
    const HandshakeResponseV2& response) {
  return ExtractRspOpParam<SigmoidParamsResult>(response, OP_TYPE_SIGMOID);
}

std::set<int32_t> IntersectOptimizers(
    const std::vector<LrHyperparamsProposal>& lr_params) {
  int field_num = LrHyperparamsProposal::kOptimizersFieldNumber;
  return util::IntersectParamItems<LrHyperparamsProposal>(lr_params, field_num);
}

std::set<int32_t> IntersectLastBatchPolicies(
    const std::vector<LrHyperparamsProposal>& lr_params) {
  int field_num = LrHyperparamsProposal::kLastBatchPoliciesFieldNumber;
  return util::IntersectParamItems<LrHyperparamsProposal>(lr_params, field_num);
}

std::set<int32_t> IntersectSigmoidModes(
    const std::vector<SigmoidParamsProposal>& sigmoid_params) {
  int field_num = SigmoidParamsProposal::kSigmoidModesFieldNumber;
  return util::IntersectParamItems<SigmoidParamsProposal>(sigmoid_params,
                                                          field_num);
}

std::vector<SSProtocolProposal> ExtractReqSsParams(
    const std::vector<HandshakeRequestV2>& requests) {
  return ExtractReqPfParams<SSProtocolProposal>(requests, PROTOCOL_FAMILY_SS);
}

std::optional<SSProtocolResult> ExtractRspSsParam(
    const HandshakeResponseV2& response) {
  return ExtractRspPfParam<SSProtocolResult>(response, PROTOCOL_FAMILY_SS);
}

std::set<int32_t> IntersectProtocols(
    const std::vector<SSProtocolProposal>& ss_params) {
  int field_num = SSProtocolProposal::kSupportedProtocolsFieldNumber;
  return util::IntersectParamItems<SSProtocolProposal>(ss_params, field_num);
}

std::set<int32_t> IntersectFieldTypes(
    const std::vector<SSProtocolProposal>& ss_params) {
  int field_num = SSProtocolProposal::kFieldTypesFieldNumber;
  return util::IntersectParamItems<SSProtocolProposal>(ss_params, field_num);
}

std::set<int32_t> IntersectShardSerializeFormats(
    const std::vector<SSProtocolProposal>& ss_params) {
  int field_num = SSProtocolProposal::kShardSerializeFormatsFieldNumber;
  return util::IntersectParamItems<SSProtocolProposal>(ss_params, field_num);
}

std::set<int32_t> IntersectPrgCryptoTypes(
    const std::vector<SSProtocolProposal>& ss_params) {
  int filed_num_1 = SSProtocolProposal::kPrgConfigsFieldNumber;
  int field_num_2 = PrgConfigProposal::kCryptoTypeFieldNumber;
  return util::IntersectParamItems<SSProtocolProposal, PrgConfigProposal>(
      ss_params, filed_num_1, field_num_2);
}

std::set<int32_t> IntersectTtpVersions(
    const std::vector<SSProtocolProposal>& ss_params) {
  int filed_num_1 = SSProtocolProposal::kTripleConfigsFieldNumber;
  int field_num_2 = TripleConfigProposal::kSeverVersionFieldNumber;
  return util::IntersectParamItems<SSProtocolProposal, TripleConfigProposal>(
      ss_params, filed_num_1, field_num_2);
}

bool UsePenaltyTerm(double value) { return !util::AlmostZero(value); }

void DisablePenaltyTerm(double& value) { value = 0.0; }

}  // namespace

LrHandler::LrHandler(std::shared_ptr<LrContext> ctx)
    : AlgoV2Handler(ctx->ic_ctx), ctx_(std::move(ctx)) {
  switch (ctx_->optimizer.type) {
    case org::interconnection::v2::algos::OPTIMIZER_SGD: {
      optimizer_ = absl::bind_front(&LrHandler::CalculateStepWithSgd, this);
      break;
    }
    case org::interconnection::v2::algos::OPTIMIZER_MOMENTUM:
    case org::interconnection::v2::algos::OPTIMIZER_ADAGRAD:
    case org::interconnection::v2::algos::OPTIMIZER_ADADELTA:
    case org::interconnection::v2::algos::OPTIMIZER_RMSPROP:
    case org::interconnection::v2::algos::OPTIMIZER_ADAM:
    case org::interconnection::v2::algos::OPTIMIZER_ADAMAX:
    case org::interconnection::v2::algos::OPTIMIZER_NADAM: {
      YACL_THROW("Unimplemented optimizer type {}", ctx_->optimizer.type);
      break;
    }
    default:
      YACL_THROW("Unspecified optimizer type {}", ctx_->optimizer.type);
  }
}

LrHandler::~LrHandler() = default;

bool LrHandler::PrepareDataset() {
  dataset_ = ReadDataset();
  YACL_ENFORCE(dataset_->shape().size() == 2);

  int64_t sample_size = dataset_->shape(0);
  int32_t feature_num =
      ctx_->HasLabel() ? dataset_->shape(1) - 1 : dataset_->shape(1);
  YACL_ENFORCE(sample_size > 0);
  YACL_ENFORCE(feature_num > 0);

  ctx_->io_param.sample_size = sample_size;
  ctx_->io_param.feature_nums.resize(ctx_->ic_ctx->lctx->WorldSize());
  ctx_->io_param.feature_nums[ctx_->ic_ctx->lctx->Rank()] = feature_num;

  return true;
}

bool LrHandler::ProcessHandshakeResponse(const HandshakeResponseV2& response) {
  if (!AlgoV2Handler::ProcessHandshakeResponse(response)) {
    return false;
  }

  // process lr hyperparameters
  LrHyperparamsResult lr_param;
  YACL_ENFORCE(response.algo() == ALGO_TYPE_SS_LR);
  YACL_ENFORCE(response.algo_param().UnpackTo(&lr_param));
  ctx_->lr_param.num_epoch = lr_param.num_epoch();
  ctx_->lr_param.batch_size = lr_param.batch_size();
  if (UsePenaltyTerm(lr_param.l0_norm())) {
    YACL_ENFORCE(UsePenaltyTerm(ctx_->lr_param.l0_norm));
  }
  ctx_->lr_param.l0_norm = lr_param.l0_norm();

  if (UsePenaltyTerm(lr_param.l1_norm())) {
    YACL_ENFORCE(UsePenaltyTerm(ctx_->lr_param.l1_norm));
  }
  ctx_->lr_param.l1_norm = lr_param.l1_norm();

  if (UsePenaltyTerm(lr_param.l2_norm())) {
    YACL_ENFORCE(UsePenaltyTerm(ctx_->lr_param.l2_norm));
  }
  ctx_->lr_param.l2_norm = lr_param.l2_norm();

  // process optimizer parameters, currently only support SGD optimizer
  YACL_ENFORCE(lr_param.optimizer_name() ==
               org::interconnection::v2::algos::OPTIMIZER_SGD);
  org::interconnection::v2::algos::SgdOptimizer optimizer;
  YACL_ENFORCE(lr_param.optimizer_param().UnpackTo(&optimizer));
  std::get<org::interconnection::v2::algos::SgdOptimizer>(ctx_->optimizer.param)
      .CopyFrom(optimizer);

  LrDataIoResult io_param;
  YACL_ENFORCE(response.io_param().UnpackTo(&io_param));
  YACL_ENFORCE(io_param.sample_size() == ctx_->io_param.sample_size);
  YACL_ENFORCE(io_param.feature_nums().size() ==
               ctx_->io_param.feature_nums.size());

  // process lr io parameters
  for (int i = 0; i < io_param.feature_nums().size(); ++i) {
    if (i == ctx_->ic_ctx->lctx->Rank()) {
      YACL_ENFORCE(ctx_->io_param.feature_nums[i] == io_param.feature_nums(i));
      continue;
    }
    ctx_->io_param.feature_nums[i] = io_param.feature_nums(i);
  }

  // process op parameters
  auto sigmoid_param_optional = ExtractRspSigmoidParam(response);
  YACL_ENFORCE(sigmoid_param_optional.has_value());
  const auto& sigmoid_param = sigmoid_param_optional.value();
  YACL_ENFORCE(sigmoid_param.sigmoid_mode() == ctx_->sigmoid_mode);

  // process ss protocol parameters
  auto ss_param_optional = ExtractRspSsParam(response);
  YACL_ENFORCE(ss_param_optional.has_value());
  const auto& ss_param = ss_param_optional.value();

  YACL_ENFORCE(ss_param.field_type() == ctx_->ss_param.field_type);
  YACL_ENFORCE(ss_param.trunc_mode().method() == ctx_->ss_param.trunc_mode);
  YACL_ENFORCE(ss_param.shard_serialize_format() ==
               ctx_->ss_param.shard_serialize_format);
  ctx_->ss_param.fxp_bits = ss_param.fxp_fraction_bits();

  YACL_ENFORCE(ss_param.triple_config().version() ==
               ctx_->ttp_config.ttp_server_version);
  ctx_->ttp_config.ttp_server_host = ss_param.triple_config().server_host();
  ctx_->ttp_config.ttp_session_id = ss_param.triple_config().session_id();
  ctx_->ttp_config.ttp_adjust_rank = ss_param.triple_config().adjust_rank();

  return true;
}

HandshakeRequestV2 LrHandler::BuildHandshakeRequest() {
  HandshakeRequestV2 request;
  auto self_rank = ctx_->ic_ctx->lctx->Rank();

  request.set_version(2);
  request.set_requester_rank(self_rank);

  request.add_supported_algos(ctx_->ic_ctx->algo);
  LrHyperparamsProposal lr_param;
  lr_param.add_supported_versions(1);
  lr_param.add_optimizers(ctx_->optimizer.type);
  lr_param.add_last_batch_policies(ctx_->lr_param.last_batch_policy);
  lr_param.set_use_l2_norm(UsePenaltyTerm(
      ctx_->lr_param.l2_norm));  // Currently only support l2 norm
  request.add_algo_params()->PackFrom(lr_param);

  request.add_ops(OP_TYPE_SIGMOID);
  SigmoidParamsProposal sigmoid_param;
  sigmoid_param.add_supported_versions(1);
  sigmoid_param.add_sigmoid_modes(ctx_->sigmoid_mode);
  request.add_op_params()->PackFrom(sigmoid_param);

  for (const auto& family : ctx_->ic_ctx->protocol_families) {
    if (family == PROTOCOL_FAMILY_SS) {
      request.add_protocol_families(family);
      SSProtocolProposal protocol_param;
      protocol_param.add_supported_versions(1);
      protocol_param.add_supported_protocols(ctx_->ss_param.protocol);
      protocol_param.add_field_types(ctx_->ss_param.field_type);
      protocol_param.add_shard_serialize_formats(
          ctx_->ss_param.shard_serialize_format);
      // set truncation mode
      auto* trunc_mode_param = protocol_param.add_trunc_modes();
      trunc_mode_param->add_supported_versions(1);
      trunc_mode_param->set_method(TRUNC_MODE_PROBABILISTIC);
      // set PRG parameters, currently only support AES128_CTR crypto type
      auto prg_param = protocol_param.add_prg_configs();
      prg_param->add_supported_versions(1);
      prg_param->set_crypto_type(CRYPTO_TYPE_AES128_CTR);
      // set TTP parameters
      auto ttp_param = protocol_param.add_triple_configs();
      ttp_param->add_supported_versions(1);
      ttp_param->set_sever_version(ctx_->ttp_config.ttp_server_version);

      request.add_protocol_family_params()->PackFrom(protocol_param);
    } else {
      YACL_THROW("Protocol family {} for LR not implemented", family);
    }
  }

  LrDataIoProposal lr_io;
  lr_io.set_sample_size(ctx_->io_param.sample_size);
  lr_io.set_feature_num(ctx_->io_param.feature_nums.at(self_rank));
  lr_io.set_has_label(ctx_->HasLabel());
  request.mutable_io_param()->PackFrom(lr_io);

  return request;
}

status::ErrorStatus LrHandler::NegotiateHandshakeParams(
    const std::vector<HandshakeRequestV2>& requests) {
  auto status = NegotiateLrAlgoParams(requests);
  if (!status.ok()) {
    return status;
  }

  status = NegotiateOpParams(requests);
  if (!status.ok()) {
    return status;
  }

  status = NegotiateSsParams(requests);
  if (!status.ok()) {
    return status;
  }

  status = NegotiateLrIoParams(requests);
  if (!status.ok()) {
    return status;
  }

  return status::OkStatus();
}

status::ErrorStatus LrHandler::NegotiateLrAlgoParams(
    const std::vector<HandshakeRequestV2>& requests) {
  auto lr_params = ExtractReqLrParams(requests);

  if (lr_params.empty()) {
    return status::InvalidRequestError("certain request has no lr algo params");
  }

  if (!NegotiateOptimizerParams(lr_params)) {
    return status::UnsupportedArgumentError("negotiate optimizer failed");
  }

  if (!NegotiateLastBatchPolicy(lr_params)) {
    return status::UnsupportedArgumentError(
        "negotiate last batch policy failed");
  }

  if (!NegotiatePenaltyTerms(lr_params)) {
    return status::UnsupportedArgumentError("negotiate penalty terms failed");
  }

  return status::OkStatus();
}

status::ErrorStatus LrHandler::NegotiateOpParams(
    const std::vector<HandshakeRequestV2>& requests) {
  auto op_params = ExtractReqSigmoidParams(requests);
  if (op_params.empty()) {
    return status::InvalidRequestError("certain request has no op params");
  }

  if (!NegotiateSigmoidParams(op_params)) {
    return status::UnsupportedArgumentError("negotiate sigmoid mode failed");
  }

  return status::OkStatus();
}

status::ErrorStatus LrHandler::NegotiateSsParams(
    const std::vector<HandshakeRequestV2>& requests) {
  auto ss_params = ExtractReqSsParams(requests);
  if (ss_params.empty()) {
    return status::InvalidRequestError("certain request has no ss params");
  }

  if (!NegotiateProtocol(ss_params)) {
    return status::UnsupportedArgumentError("negotiate ss protocol failed");
  }

  if (!NegotiateFieldType(ss_params)) {
    return status::UnsupportedArgumentError("negotiate filed type failed");
  }

  if (!NegotiateShardSerializeFormat(ss_params)) {
    return status::UnsupportedArgumentError(
        "negotiate shared serialize format failed");
  }

  if (!NegotiateTruncMode(ss_params)) {
    return status::UnsupportedArgumentError("negotiate trunc mode failed");
  }

  if (!NegotiatePrgConfig(ss_params)) {
    return status::UnsupportedArgumentError("negotiate PRG config failed");
  }

  if (!NegotiateTtpConfig(ss_params)) {
    return status::UnsupportedArgumentError("negotiate TTP config failed");
  }

  return status::OkStatus();
}

status::ErrorStatus LrHandler::NegotiateLrIoParams(
    const std::vector<HandshakeRequestV2>& requests) {
  for (const auto& request : requests) {
    LrDataIoProposal io_param;
    if (!request.io_param().UnpackTo(&io_param)) {
      return status::InvalidRequestError(
          "certain request has invalid io param");
    }

    // negotiate same_size
    if (io_param.sample_size() != ctx_->io_param.sample_size) {
      return status::HandshakeRefusedError("sample size inconsistent");
    }

    if (io_param.feature_num() <= 0) {
      return status::InvalidRequestError(
          "certain request has invalid feature_num");
    } else {
      // TODO: check requester_rank in outer scope
      YACL_ENFORCE(static_cast<int32_t>(ctx_->io_param.feature_nums.size()) >
                   request.requester_rank());
      ctx_->io_param.feature_nums[request.requester_rank()] =
          io_param.feature_num();
    }

    // negotiate label_rank
    if (io_param.has_label()) {
      if (ctx_->io_param.label_rank != -1) {
        return status::HandshakeRefusedError("more than one party have label");
      } else {
        ctx_->io_param.label_rank = request.requester_rank();
      }
    }
  }

  if (ctx_->io_param.label_rank == -1) {
    return status::InvalidRequestError("no party has label");
  }

  return status::OkStatus();
}

bool LrHandler::NegotiateOptimizerParams(
    const std::vector<LrHyperparamsProposal>& lr_params) {
  auto optimizers = IntersectOptimizers(lr_params);
  return optimizers.find(ctx_->optimizer.type) != optimizers.end();
}

bool LrHandler::NegotiateLastBatchPolicy(
    const std::vector<LrHyperparamsProposal>& lr_params) {
  auto policies = IntersectLastBatchPolicies(lr_params);
  return policies.find(ctx_->lr_param.last_batch_policy) != policies.end();
}

bool LrHandler::NegotiatePenaltyTerms(
    const std::vector<LrHyperparamsProposal>& lr_params) {
  auto negotiate_norm = [](const std::vector<LrHyperparamsProposal>& lr_params,
                           double& norm, int field_num) {
    auto norm_optional =
        util::AlignParamItem<LrHyperparamsProposal, bool>(lr_params, field_num);

    bool others_use_norm = norm_optional.has_value() && norm_optional.value();
    if (!others_use_norm) {
      DisablePenaltyTerm(norm);
    }
  };

  negotiate_norm(lr_params, ctx_->lr_param.l0_norm,
                 LrHyperparamsProposal::kUseL0NormFieldNumber);

  negotiate_norm(lr_params, ctx_->lr_param.l1_norm,
                 LrHyperparamsProposal::kUseL1NormFieldNumber);

  negotiate_norm(lr_params, ctx_->lr_param.l2_norm,
                 LrHyperparamsProposal::kUseL2NormFieldNumber);

  return true;
}

bool LrHandler::NegotiateSigmoidParams(
    const std::vector<SigmoidParamsProposal>& sigmoid_params) {
  auto sigmoid_modes = IntersectSigmoidModes(sigmoid_params);
  return sigmoid_modes.find(ctx_->sigmoid_mode) != sigmoid_modes.end();
}

bool LrHandler::NegotiateProtocol(
    const std::vector<SSProtocolProposal>& ss_params) {
  auto protocols = IntersectProtocols(ss_params);
  return protocols.find(ctx_->ss_param.protocol) != protocols.end();
}

bool LrHandler::NegotiateFieldType(
    const std::vector<SSProtocolProposal>& ss_params) {
  auto protocols = IntersectFieldTypes(ss_params);
  return protocols.find(ctx_->ss_param.field_type) != protocols.end();
}

bool LrHandler::NegotiateShardSerializeFormat(
    const std::vector<SSProtocolProposal>& ss_params) {
  auto formats = IntersectShardSerializeFormats(ss_params);
  return formats.find(ctx_->ss_param.shard_serialize_format) != formats.end();
}

std::set<int32_t> LrHandler::IntersectTruncModes(
    const std::vector<SSProtocolProposal>& ss_params) {
  std::vector<std::set<int32_t>> trunc_modes;
  for (const auto& ss_param : ss_params) {
    trunc_modes.resize(trunc_modes.size() + 1);

    for (const auto& item : ss_param.trunc_modes()) {
      if (!item.compatible_protocols().empty() &&
          (std::find(item.compatible_protocols().begin(),
                     item.compatible_protocols().end(),
                     ctx_->ss_param.trunc_mode) ==
           item.compatible_protocols().end())) {
        continue;
      } else {
        trunc_modes.back().insert(item.method());
      }
    }
  }

  return util::Intersection(trunc_modes);
}

bool LrHandler::NegotiateTruncMode(
    const std::vector<SSProtocolProposal>& ss_params) {
  auto trunc_modes = IntersectTruncModes(ss_params);
  return trunc_modes.find(ctx_->ss_param.trunc_mode) != trunc_modes.end();
}

bool LrHandler::NegotiatePrgConfig(
    const std::vector<SSProtocolProposal>& ss_params) {
  auto crypto_types = IntersectPrgCryptoTypes(ss_params);
  return crypto_types.find(CRYPTO_TYPE_AES128_CTR) != crypto_types.end();
}

bool LrHandler::NegotiateTtpConfig(
    const std::vector<SSProtocolProposal>& ss_params) {
  auto ttp_server_versions = IntersectTtpVersions(ss_params);
  return ttp_server_versions.find(ctx_->ttp_config.ttp_server_version) !=
         ttp_server_versions.end();
}

HandshakeResponseV2 LrHandler::BuildHandshakeResponse() {
  HandshakeResponseV2 response;
  response.mutable_header()->set_error_code(org::interconnection::OK);

  // set algo params
  response.set_algo(ALGO_TYPE_SS_LR);
  LrHyperparamsResult lr_param;
  lr_param.set_version(1);
  lr_param.set_num_epoch(ctx_->lr_param.num_epoch);
  lr_param.set_batch_size(ctx_->lr_param.batch_size);
  lr_param.set_last_batch_policy(ctx_->lr_param.last_batch_policy);
  if (UsePenaltyTerm(ctx_->lr_param.l0_norm)) {
    lr_param.set_l0_norm(ctx_->lr_param.l0_norm);
  }
  if (UsePenaltyTerm(ctx_->lr_param.l1_norm)) {
    lr_param.set_l1_norm(ctx_->lr_param.l1_norm);
  }
  if (UsePenaltyTerm(ctx_->lr_param.l2_norm)) {
    lr_param.set_l2_norm(ctx_->lr_param.l2_norm);
  }

  lr_param.set_optimizer_name(ctx_->optimizer.type);
  org::interconnection::v2::algos::SgdOptimizer optimizer;
  optimizer.CopyFrom(std::get<org::interconnection::v2::algos::SgdOptimizer>(
      ctx_->optimizer.param));  // Currently only support SGD optimizer
  lr_param.mutable_optimizer_param()->PackFrom(optimizer);

  response.mutable_algo_param()->PackFrom(lr_param);

  // set op params
  response.add_ops(OP_TYPE_SIGMOID);
  SigmoidParamsResult sigmoid_param;
  sigmoid_param.set_sigmoid_mode(ctx_->sigmoid_mode);
  response.add_op_params()->PackFrom(sigmoid_param);

  // set ss params
  response.add_protocol_families(PROTOCOL_FAMILY_SS);
  SSProtocolResult ss_param;
  ss_param.set_protocol(ctx_->ss_param.protocol);
  ss_param.set_field_type(ctx_->ss_param.field_type);
  ss_param.set_fxp_fraction_bits(ctx_->ss_param.fxp_bits);
  ss_param.set_shard_serialize_format(ctx_->ss_param.shard_serialize_format);
  ss_param.mutable_trunc_mode()->set_version(1);
  ss_param.mutable_trunc_mode()->set_method(ctx_->ss_param.trunc_mode);
  ss_param.mutable_triple_config()->set_version(1);
  ss_param.mutable_triple_config()->set_server_host(
      ctx_->ttp_config.ttp_server_host);
  ss_param.mutable_triple_config()->set_version(
      ctx_->ttp_config.ttp_server_version);
  ss_param.mutable_triple_config()->set_session_id(
      ctx_->ttp_config.ttp_session_id);
  ss_param.mutable_triple_config()->set_adjust_rank(
      ctx_->ttp_config.ttp_adjust_rank);
  response.add_protocol_family_params()->PackFrom(ss_param);

  // set io params
  LrDataIoResult io_param;
  io_param.set_version(1);
  io_param.set_sample_size(ctx_->io_param.sample_size);
  io_param.mutable_feature_nums()->Add(ctx_->io_param.feature_nums.begin(),
                                       ctx_->io_param.feature_nums.end());
  io_param.set_label_rank(ctx_->io_param.label_rank);
  response.mutable_io_param()->PackFrom(io_param);

  return response;
}

spu::Value inference(spu::SPUContext* ctx, const spu::Value& x,
                     const spu::Value& weight) {
  auto padding =
      spu::kernel::hal::constant(ctx, 1.0F, spu::DT_F32, {x.shape()[0], 1});
  auto padded_x = spu::kernel::hal::concatenate(
      ctx, {x, spu::kernel::hal::seal(ctx, padding)}, 1);
  return spu::kernel::hal::matmul(ctx, padded_x, weight);
}

float Accuracy(const xt::xarray<float>& y_true,
               const xt::xarray<float>& y_pred) {
  int64_t total_num = 0;
  int64_t accurate_num = 0;
  for (auto y_true_iter = y_true.begin(), y_pred_iter = y_pred.begin();
       y_true_iter != y_true.end() && y_pred_iter != y_pred.end();
       ++y_pred_iter, ++y_true_iter) {
    if ((util::AlmostZero(*y_true_iter) && *y_pred_iter < 0.5) ||
        (util::AlmostOne(*y_true_iter) && *y_pred_iter >= 0.5)) {
      ++accurate_num;
    }
    ++total_num;
  }

  return accurate_num / static_cast<float>(total_num);
}

void ProduceOutput(spu::SPUContext* sctx, const spu::Value& w) {
  // output result shares to the file
  spu::PtType out_pt_type;
  auto w_output = spu::decodeFromRing(
      w.data(), w.dtype(), sctx->config().fxp_fraction_bits(), &out_pt_type);
  YACL_ENFORCE(out_pt_type == spu::PT_F32);

  std::string out_file_name = GetOutputFileName();
  std::ofstream of(out_file_name);
  YACL_ENFORCE(of, "open file={} failed", out_file_name);
  for (auto it = w_output.begin(); it != w_output.end(); ++it) {
    auto* item = reinterpret_cast<float*>(it.getRawPtr());
    of << *item << '\n';
  }
}

void LrHandler::RunAlgo() {
  auto sctx = MakeSpuContext();
  spu::mpc::Factory::RegisterProtocol(sctx.get(), sctx->lctx());
  auto [x, y] = ProcessDataset(sctx.get());

  auto w = Train(sctx.get(), x, y);

  // to delete
  const auto scores = inference(sctx.get(), x, w);

  xt::xarray<float> revealed_labels = spu::kernel::hal::dump_public_as<float>(
      sctx.get(), spu::kernel::hal::reveal(sctx.get(), y));
  xt::xarray<float> revealed_scores = spu::kernel::hal::dump_public_as<float>(
      sctx.get(), spu::kernel::hal::reveal(sctx.get(), scores));

  auto accuracy = Accuracy(revealed_labels, revealed_scores);
  std::cout << "Accuracy = " << accuracy << "\n";

  ProduceOutput(sctx.get(), w);
}

std::unique_ptr<spu::SPUContext> LrHandler::MakeSpuContext() {
  auto lctx = ctx_->ic_ctx->lctx;
  static const std::map<int32_t, spu::ProtocolKind> protocol_map{
      {org::interconnection::v2::protocol::PROTOCOL_KIND_SEMI2K,
       spu::ProtocolKind::SEMI2K},
      {org::interconnection::v2::protocol::PROTOCOL_KIND_ABY3,
       spu::ProtocolKind::ABY3}};

  spu::RuntimeConfig config;
  config.set_protocol(protocol_map.at(ctx_->ss_param.protocol));
  config.set_field(static_cast<spu::FieldType>(
      ctx_->ss_param.field_type));  // Note: 定义映射
  config.set_fxp_fraction_bits(ctx_->ss_param.fxp_bits);
  config.set_experimental_disable_mmul_split(true);
  config.set_experimental_disable_vectorization(true);

  if (ctx_->ss_param.trunc_mode ==
      org::interconnection::v2::protocol::TRUNC_MODE_PROBABILISTIC) {
    config.set_trunc_allow_msb_error(true);
  }

  config.set_sigmoid_mode(
      static_cast<spu::RuntimeConfig::SigmoidMode>(ctx_->sigmoid_mode));

  populateRuntimeConfig(config);

  if (ctx_->ttp_config.use_ttp) {
    config.set_beaver_type(spu::RuntimeConfig_BeaverType_TrustedThirdParty);
    config.mutable_ttp_beaver_config()->set_server_host(
        ctx_->ttp_config.ttp_server_host);
    config.mutable_ttp_beaver_config()->set_adjust_rank(
        ctx_->ttp_config.ttp_adjust_rank);
    config.mutable_ttp_beaver_config()->set_session_id(
        ctx_->ttp_config.ttp_session_id);
  } else {
    config.set_beaver_type(spu::RuntimeConfig_BeaverType_TrustedFirstParty);
  }

  config.set_enable_action_trace(true);
  config.set_enable_type_checker(true);
  return std::make_unique<spu::SPUContext>(config, lctx);
}

spu::Value LrHandler::EncodingDataset(spu::PtBufferView dataset) {
  // encode to ring.
  spu::DataType dtype;
  spu::NdArrayRef encoded =
      encodeToRing(convertToNdArray(dataset),
                   static_cast<spu::FieldType>(ctx_->ss_param.field_type),
                   ctx_->ss_param.fxp_bits, &dtype);

  return spu::Value(encoded, dtype);
}

spu::Value LrHandler::Concatenate(spu::SPUContext* sctx, spu::Value x) {
  size_t self_rank = ctx_->ic_ctx->lctx->Rank();
  size_t world_size = ctx_->ic_ctx->lctx->WorldSize();
  YACL_ENFORCE(self_rank < world_size);
  YACL_ENFORCE(ctx_->io_param.feature_nums.size() == world_size);
  YACL_ENFORCE(x.shape().size() == 2);
  YACL_ENFORCE(x.shape().at(0) == ctx_->io_param.sample_size);
  YACL_ENFORCE(x.shape().at(1) == ctx_->io_param.feature_nums.at(self_rank));

  spu::Type ty;
  if (ctx_->ss_param.protocol == PROTOCOL_KIND_SEMI2K) {
    ty = spu::makeType<spu::mpc::semi2k::AShrTy>(static_cast<spu::FieldType>(
        ctx_->ss_param.field_type));  // TODO: whether set owner_rank?
  } else if (ctx_->ss_param.protocol == PROTOCOL_KIND_ABY3) {
    ty = spu::makeType<spu::mpc::aby3::AShrTy>(
        static_cast<spu::FieldType>(ctx_->ss_param.field_type));
  } else {
    YACL_THROW("Unexpected error");
  }
  x.storage_type() = ty;

  std::vector<spu::Value> x_vec(world_size);
  x_vec[self_rank] = std::move(x);
  for (size_t i = 0; i < world_size; ++i) {
    if (i == self_rank) {
      continue;
    }

    int64_t rows = ctx_->io_param.sample_size;
    int64_t columns = ctx_->io_param.feature_nums.at(i);
    x_vec[i] = spu::kernel::hal::zeros(sctx, spu::DT_F32, {rows, columns});
    x_vec[i].storage_type() = ty;
  }

  return spu::kernel::hal::concatenate(sctx, x_vec, 1);
}

std::pair<spu::Value, spu::Value> LrHandler::ProcessDataset(
    spu::SPUContext* sctx) {
  spu::Value x;
  spu::Value y;

  YACL_ENFORCE(dataset_);
  if (ctx_->HasLabel()) {
    // the last column is label.
    using xt::placeholders::_;  // required for `_` to work
    xt::xarray<float> dx =
        xt::view(*dataset_, xt::all(), xt::range(_, dataset_->shape(1) - 1));
    xt::xarray<float> dy =
        xt::view(*dataset_, xt::all(), xt::range(dataset_->shape(1) - 1, _));
    x = EncodingDataset(spu::PtBufferView(dx));
    y = EncodingDataset(spu::PtBufferView(dy));
  } else {
    x = EncodingDataset(spu::PtBufferView(*dataset_));
    y = spu::kernel::hal::constant(sctx, 0.0F, spu::DT_F32,
                                   {ctx_->io_param.sample_size, 1});
  }

  y.storage_type() = spu::makeType<spu::mpc::semi2k::AShrTy>(
      static_cast<spu::FieldType>(ctx_->ss_param.field_type));  // TODO

  return std::make_pair(Concatenate(sctx, std::move(x)), y);
}

spu::Value LrHandler::Train(spu::SPUContext* ctx, const spu::Value& x,
                            const spu::Value& y) {
  auto w =
      spu::kernel::hal::constant(ctx, 0.0F, spu::DT_F32, {x.shape()[1] + 1, 1});
  int64_t num_batch = ctx_->io_param.sample_size / ctx_->lr_param.batch_size;

  // Run train loop
  for (int64_t epoch = 0; epoch < ctx_->lr_param.num_epoch; ++epoch) {
    for (int64_t batch = 0; batch < num_batch; ++batch) {
      SPDLOG_INFO("Running train iteration {}", batch);

      const int64_t rows_beg = batch * ctx_->lr_param.batch_size;
      const int64_t rows_end = rows_beg + ctx_->lr_param.batch_size;

      const auto x_slice = spu::kernel::hal::slice(
          ctx, x, {rows_beg, 0}, {rows_end, x.shape()[1]}, {});

      const auto y_slice = spu::kernel::hal::slice(
          ctx, y, {rows_beg, 0}, {rows_end, y.shape()[1]}, {});

      w = TrainStep(ctx, x_slice, y_slice, w);
    }
  }

  return w;
}

spu::Value LrHandler::TrainStep(spu::SPUContext* ctx, const spu::Value& x,
                                const spu::Value& y, const spu::Value& w) {
  // Padding x
  auto padding =
      spu::kernel::hal::constant(ctx, 1.0F, spu::DT_F32, {x.shape()[0], 1});
  auto padded_x = spu::kernel::hal::concatenate(
      ctx, {x, spu::kernel::hal::seal(ctx, padding)}, 1);
  auto pred = spu::kernel::hal::logistic(
      ctx, spu::kernel::hal::matmul(ctx, padded_x, w));

  SPDLOG_DEBUG("[SSLR] Err = Pred - Y");
  auto err = spu::kernel::hal::sub(ctx, pred, y);

  SPDLOG_DEBUG("[SSLR] Grad = X.t * Err");
  auto grad = spu::kernel::hal::matmul(
      ctx, spu::kernel::hal::transpose(ctx, padded_x), err);

  SPDLOG_DEBUG("[SSLR] Grad = Grad + W' * l2_norm");
  if (UsePenaltyTerm(ctx_->lr_param.l2_norm)) {
    auto w_with_zero_bias =
        spu::kernel::hal::slice(ctx, w, {0, 0}, {w.shape()[0] - 1, 1}, {});
    auto zeros = spu::kernel::hal::zeros(ctx, spu::DT_F32, {1, 1});
    auto w_superfix = spu::kernel::hal::concatenate(
        ctx, {w_with_zero_bias, spu::kernel::hal::seal(ctx, zeros)}, 0);
    auto l2_norm = spu::kernel::hal::constant(
        ctx, static_cast<float>(ctx_->lr_param.l2_norm), spu::DT_F32);

    grad = spu::kernel::hal::add(
        ctx, grad,
        spu::kernel::hal::mul(
            ctx, spu::kernel::hal::broadcast_to(ctx, l2_norm, grad.shape()),
            w_superfix));
  }

  auto step = this->optimizer_(ctx, grad);

  SPDLOG_DEBUG("[SSLR] W = W - Step");
  auto new_w = spu::kernel::hal::sub(ctx, w, step);

  return new_w;
}

spu::Value LrHandler::CalculateStepWithSgd(spu::SPUContext* ctx,
                                           const spu::Value& grad) {
  SPDLOG_DEBUG("[SSLR] Step = LR / B * Grad");
  const auto optimizer_param =
      std::get_if<org::interconnection::v2::algos::SgdOptimizer>(
          &ctx_->optimizer.param);
  YACL_ENFORCE(optimizer_param);

  auto lr = spu::kernel::hal::constant(
      ctx, static_cast<float>(optimizer_param->learning_rate()), spu::DT_F32);
  auto msize = spu::kernel::hal::constant(
      ctx, static_cast<float>(ctx_->lr_param.batch_size), spu::DT_F32);
  auto p1 =
      spu::kernel::hal::mul(ctx, lr, spu::kernel::hal::reciprocal(ctx, msize));
  auto step = spu::kernel::hal::mul(
      ctx, spu::kernel::hal::broadcast_to(ctx, p1, grad.shape()), grad);

  return step;
}

}  // namespace ic_impl::algo::lr
