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

#include "ic_impl/algo/psi/v2/psi_context_v2.h"
#include "ic_impl/handler.h"

#include "interconnection/handshake/algos/psi.pb.h"
#include "interconnection/handshake/protocol_family/ecc.pb.h"

namespace psi::psi {
class BucketPsi;
}

namespace ic_impl::algo::psi::v2 {

class EcdhPsiV2Handler : public AlgoV2Handler {
 public:
  explicit EcdhPsiV2Handler(std::shared_ptr<EcdhPsiContext> ctx);

  ~EcdhPsiV2Handler() override;

 private:
  bool ProcessHandshakeResponse(const HandshakeResponseV2 &) override;

  HandshakeRequestV2 BuildHandshakeRequest() override;

  HandshakeResponseV2 BuildHandshakeResponse() override;

  bool PrepareDataset() override;

  void RunAlgo() override;

  status::ErrorStatus NegotiateHandshakeParams(
      const std::vector<HandshakeRequestV2> &) override;

  bool NegotiateEcSuits(
      const std::vector<org::interconnection::v2::protocol::EccProtocolProposal>
          &ecc_params);

  bool NegotiatePointOctetFormats(
      const std::vector<org::interconnection::v2::protocol::EccProtocolProposal>
          &ecc_params);

  bool NegotiateBitLengthAfterTruncated(
      const std::vector<org::interconnection::v2::protocol::EccProtocolProposal>
          &ecc_params);

  bool NegotiateResultToRank(
      const std::vector<org::interconnection::v2::algos::PsiDataIoProposal>
          &io_params);

  status::ErrorStatus NegotiateEccParams(
      const std::vector<HandshakeRequestV2> &requests);

  status::ErrorStatus NegotiatePsiIoParams(
      const std::vector<HandshakeRequestV2> &requests);

  std::shared_ptr<EcdhPsiContext> ctx_;

  std::unique_ptr<::psi::BucketPsi> bucket_psi_;
};

}  // namespace ic_impl::algo::psi::v2
