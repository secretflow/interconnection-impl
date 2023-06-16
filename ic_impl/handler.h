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

#include "ic_impl/status.h"
#include "ic_impl/util.h"

#include "interconnection/algos/psi.pb.h"
#include "interconnection/v2(rfc)/handshake/entry.pb.h"

namespace ic_impl {

using HandshakeRequestV1 = org::interconnection::algos::psi::HandshakeRequest;
using HandshakeResponseV1 = org::interconnection::algos::psi::HandshakeResponse;

using HandshakeRequestV2 = org::interconnection::v2::HandshakeRequest;
using HandshakeResponseV2 = org::interconnection::v2::HandshakeResponse;

struct IcContext;

class AlgoV1Handler {
 public:
  AlgoV1Handler() = default;
  ~AlgoV1Handler() = default;
};

class AlgoV2Handler {
  friend class Party;

 public:
  explicit AlgoV2Handler(std::shared_ptr<IcContext> ctx)
      : ctx_(std::move(ctx)) {}

  virtual ~AlgoV2Handler() = default;

  void PassiveRun(const std::vector<HandshakeRequestV2> &requests);

  void ActiveRun(int32_t recv_rank);

 protected:
  virtual bool ProcessHandshakeResponse(const HandshakeResponseV2 &);

 private:
  void SendHandshakeRequest(const HandshakeRequestV2 &request,
                            int32_t dst_rank);

  void SendHandshakeResponse(const HandshakeResponseV2 &response);

  HandshakeResponseV2 RecvHandshakeResponse(int32_t src_rank);

  bool PassiveHandshake(const std::vector<HandshakeRequestV2> &requests);

  bool ActiveHandshake(int32_t dst_rank);

  HandshakeResponseV2 ProcessHandshakeRequests(
      const std::vector<HandshakeRequestV2> &);

  virtual HandshakeRequestV2 BuildHandshakeRequest() = 0;

  virtual HandshakeResponseV2 BuildHandshakeResponse() = 0;

  virtual status::ErrorStatus NegotiateHandshakeParams(
      const std::vector<HandshakeRequestV2> &requests) = 0;

  virtual bool PrepareDataset() = 0;

  virtual void RunAlgo() = 0;

  std::shared_ptr<IcContext> ctx_;
};

// Extract algo parameters from requests
template <typename ParamType>
std::vector<ParamType> ExtractReqAlgoParams(
    const std::vector<HandshakeRequestV2> &requests, int32_t algo_type) {
  int enum_field_num = HandshakeRequestV2::kSupportedAlgosFieldNumber;
  int param_field_num = HandshakeRequestV2::kAlgoParamsFieldNumber;
  return util::ExtractParams<HandshakeRequestV2, ParamType>(
      requests, enum_field_num, param_field_num, algo_type);
}

// Extract op parameters from requests
template <typename ParamType>
std::vector<ParamType> ExtractReqOpParams(
    const std::vector<HandshakeRequestV2> &requests, int32_t op_type) {
  int enum_field_num = HandshakeRequestV2::kOpsFieldNumber;
  int param_field_num = HandshakeRequestV2::kOpParamsFieldNumber;
  return util::ExtractParams<HandshakeRequestV2, ParamType>(
      requests, enum_field_num, param_field_num, op_type);
}

// Extract protocol family parameters from requests
template <typename ParamType>
std::vector<ParamType> ExtractReqPfParams(
    const std::vector<HandshakeRequestV2> &requests, int32_t protocol_family) {
  int enum_field_num = HandshakeRequestV2::kProtocolFamiliesFieldNumber;
  int param_field_num = HandshakeRequestV2::kProtocolFamilyParamsFieldNumber;
  return util::ExtractParams<HandshakeRequestV2, ParamType>(
      requests, enum_field_num, param_field_num, protocol_family);
}

// Extract io parameters from requests
template <typename ParamType>
std::vector<ParamType> ExtractReqIoParams(
    const std::vector<HandshakeRequestV2> &requests) {
  int field_num = HandshakeRequestV2::kIoParamsFieldNumber;
  return util::ExtractParams<HandshakeRequestV2, ParamType>(requests,
                                                            field_num);
}

// Extract op parameter from response
template <typename ParamType>
std::optional<ParamType> ExtractRspOpParam(const HandshakeResponseV2 &response,
                                           int32_t op_enum) {
  int enum_filed_num = HandshakeResponseV2::kOpsFieldNumber;
  int param_filed_num = HandshakeResponseV2::kOpParamsFieldNumber;
  return util::ExtractParam<HandshakeResponseV2, ParamType>(
      response, enum_filed_num, param_filed_num, op_enum);
}

// Extract protocol family parameter from response
template <typename ParamType>
std::optional<ParamType> ExtractRspPfParam(const HandshakeResponseV2 &response,
                                           int32_t pf_enum) {
  int enum_filed_num = HandshakeResponseV2::kProtocolFamilyFieldNumber;
  int param_filed_num = HandshakeResponseV2::kProtocolFamilyParamsFieldNumber;
  return util::ExtractParam<HandshakeResponseV2, ParamType>(
      response, enum_filed_num, param_filed_num, pf_enum);
}

}  // namespace ic_impl
