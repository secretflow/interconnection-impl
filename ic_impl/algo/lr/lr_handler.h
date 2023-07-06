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

#include "libspu/core/pt_buffer_view.h"
#include "libspu/core/value.h"
#include "xtensor/xarray.hpp"

#include "ic_impl/algo/lr/lr_context.h"
#include "ic_impl/handler.h"

#include "interconnection/handshake/algos/lr.pb.h"
#include "interconnection/handshake/op/sigmoid.pb.h"
#include "interconnection/handshake/protocol_family/ss.pb.h"

namespace spu {
class SPUContext;
}

namespace ic_impl::algo::lr {

class LrHandler : public AlgoV2Handler {
 public:
  explicit LrHandler(std::shared_ptr<LrContext> ctx);

  ~LrHandler() override;

 private:
  bool ProcessHandshakeResponse(const HandshakeResponseV2&) override;

  HandshakeRequestV2 BuildHandshakeRequest() override;

  HandshakeResponseV2 BuildHandshakeResponse() override;

  bool PrepareDataset() override;

  void RunAlgo() override;

  status::ErrorStatus NegotiateHandshakeParams(
      const std::vector<HandshakeRequestV2>&) override;

  status::ErrorStatus NegotiateLrAlgoParams(
      const std::vector<HandshakeRequestV2>& requests);

  status::ErrorStatus NegotiateOpParams(
      const std::vector<HandshakeRequestV2>& requests);

  status::ErrorStatus NegotiateSsParams(
      const std::vector<HandshakeRequestV2>& requests);

  status::ErrorStatus NegotiateLrIoParams(
      const std::vector<HandshakeRequestV2>& requests);

  bool NegotiateOptimizerParams(
      const std::vector<org::interconnection::v2::algos::LrHyperparamsProposal>&
          lr_params);

  bool NegotiateLastBatchPolicy(
      const std::vector<org::interconnection::v2::algos::LrHyperparamsProposal>&
          lr_params);

  bool NegotiatePenaltyTerms(
      const std::vector<org::interconnection::v2::algos::LrHyperparamsProposal>&
          lr_params);

  bool NegotiateSigmoidParams(
      const std::vector<org::interconnection::v2::op::SigmoidParamsProposal>&
          params);

  bool NegotiateProtocol(
      const std::vector<org::interconnection::v2::protocol::SSProtocolProposal>&
          ss_params);

  bool NegotiateFieldType(
      const std::vector<org::interconnection::v2::protocol::SSProtocolProposal>&
          ss_params);

  bool NegotiateShardSerializeFormat(
      const std::vector<org::interconnection::v2::protocol::SSProtocolProposal>&
          ss_params);

  std::set<int32_t> IntersectTruncModes(
      const std::vector<org::interconnection::v2::protocol::SSProtocolProposal>&
          ss_params);

  bool NegotiateTruncMode(
      const std::vector<org::interconnection::v2::protocol::SSProtocolProposal>&
          ss_params);

  bool NegotiatePrgConfig(
      const std::vector<org::interconnection::v2::protocol::SSProtocolProposal>&
          ss_params);

  bool NegotiateTtpConfig(
      const std::vector<org::interconnection::v2::protocol::SSProtocolProposal>&
          ss_params);

  std::unique_ptr<spu::SPUContext> MakeSpuContext();

  spu::Value EncodingDataset(spu::PtBufferView dataset);

  spu::Value Concatenate(spu::SPUContext* sctx, spu::Value x);

  std::pair<spu::Value, spu::Value> ProcessDataset(spu::SPUContext* sctx);

  spu::Value Train(spu::SPUContext* ctx, const spu::Value& x,
                   const spu::Value& y);

  spu::Value TrainStep(spu::SPUContext* ctx, const spu::Value& x,
                       const spu::Value& y, const spu::Value& w);

  spu::Value CalculateStepWithSgd(spu::SPUContext* ctx, const spu::Value& grad);

  std::shared_ptr<LrContext> ctx_;

  std::unique_ptr<xt::xarray<float>> dataset_;

  std::function<spu::Value(spu::SPUContext*, const spu::Value&)> optimizer_;
};

}  // namespace ic_impl::algo::lr
