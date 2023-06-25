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

#include "ic_impl/algo/lr/lr_context.h"

#include "gflags/gflags.h"

#include "ic_impl/op/sigmoid/sigmoid.h"
#include "ic_impl/util.h"

#include "interconnection/v2(rfc)/handshake/algos/lr.pb.h"

DEFINE_bool(has_label, false, "if true, label is the last column of dataset");
DEFINE_int64(batch_size, 21, "size of each batch");
DEFINE_string(last_batch_policy, "discard",
              "policy to process the partial last batch of each epoch");
DEFINE_int64(num_epoch, 1, "number of epoch");
DEFINE_double(l0_norm, 0.0, "l0 norm");
DEFINE_double(l1_norm, 0.0, "l1 norm");
DEFINE_double(l2_norm, 0.5, "l2 norm");

namespace ic_impl::algo::lr {

namespace {

bool SuggestedHasLabel() {
  return util::GetParamEnv("has_label", FLAGS_has_label);
}

int64_t SuggestedNumEpoch() {
  return util::GetParamEnv("num_epoch", FLAGS_num_epoch);
}

int64_t SuggestedBatchSize() {
  return util::GetParamEnv("batch_size", FLAGS_batch_size);
}

int32_t SuggestedLastBatchPolicy() {
  return util::GetFlagValue(
      org::interconnection::v2::algos::LastBatchPolicy_descriptor(),
      "LAST_BATCH_POLICY_",
      util::GetParamEnv("last_batch_policy", FLAGS_last_batch_policy));
}

double SuggestedL0Norm() { return util::GetParamEnv("l0_norm", FLAGS_l0_norm); }

double SuggestedL1Norm() { return util::GetParamEnv("l1_norm", FLAGS_l1_norm); }

double SuggestedL2Norm() { return util::GetParamEnv("l2_norm", FLAGS_l2_norm); }

LrHyperParam SuggestedLrHyperParam() {
  LrHyperParam lr_param;
  lr_param.num_epoch = SuggestedNumEpoch();
  lr_param.batch_size = SuggestedBatchSize();
  lr_param.last_batch_policy = SuggestedLastBatchPolicy();
  lr_param.l0_norm = SuggestedL0Norm();
  lr_param.l1_norm = SuggestedL1Norm();
  lr_param.l2_norm = SuggestedL2Norm();

  return lr_param;
}

}  // namespace

std::shared_ptr<LrContext> CreateLrContext(std::shared_ptr<IcContext> ic_ctx) {
  auto ctx = std::make_shared<LrContext>();
  ctx->lr_param = SuggestedLrHyperParam();

  ctx->io_param.label_rank = SuggestedHasLabel() ? ic_ctx->lctx->Rank() : -1;

  ctx->sigmoid_mode = op::sigmoid::SuggestedSigmoidMode();

  ctx->ttp_config = protocol_family::ss::SuggestedTtpConfig();

  ctx->ss_param = protocol_family::ss::SuggestedSsProtocolParam();

  ctx->optimizer = optimizer::SuggestedOptimizer();

  ctx->ic_ctx = std::move(ic_ctx);

  return ctx;
}

}  // namespace ic_impl::algo::lr
