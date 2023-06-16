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

#include "ic_impl/handler.h"

#include "spdlog/spdlog.h"

#include "ic_impl/context.h"

namespace ic_impl {

void AlgoV2Handler::PassiveRun(
    const std::vector<HandshakeRequestV2> &requests) {
  if (!PrepareDataset()) {
    return;
  }

  if (!PassiveHandshake(requests)) {
    return;
  }

  RunAlgo();
}

void AlgoV2Handler::ActiveRun(int32_t recv_rank) {
  if (!PrepareDataset()) {
    return;
  }

  if (!ActiveHandshake(recv_rank)) {
    return;
  }

  RunAlgo();
}

bool AlgoV2Handler::PassiveHandshake(
    const std::vector<HandshakeRequestV2> &requests) {
  auto response = ProcessHandshakeRequests(requests);

  SendHandshakeResponse(response);

  return response.header().error_code() == org::interconnection::OK;
}

bool AlgoV2Handler::ActiveHandshake(int32_t dst_rank) {
  auto request = BuildHandshakeRequest();
  SendHandshakeRequest(request, dst_rank);

  auto response = RecvHandshakeResponse(dst_rank);
  return ProcessHandshakeResponse(response);
}

HandshakeResponseV2 AlgoV2Handler::ProcessHandshakeRequests(
    const std::vector<HandshakeRequestV2> &requests) {
  auto status = NegotiateHandshakeParams(requests);
  if (!status.ok()) {
    SPDLOG_WARN("negotiate handshake params failed, {}", status.message());
    HandshakeResponseV2 response;
    response.mutable_header()->set_error_code(status.code());
    response.mutable_header()->set_error_msg(status.message());

    return response;
  }

  return BuildHandshakeResponse();
}

bool AlgoV2Handler::ProcessHandshakeResponse(
    const HandshakeResponseV2 &response) {
  if (response.header().error_code() != org::interconnection::ErrorCode::OK) {
    SPDLOG_WARN("Handshake response, error_code: {}, error_msg: {}",
                response.header().error_code(), response.header().error_msg());
    return false;
  }

  return true;
}

void AlgoV2Handler::SendHandshakeRequest(const HandshakeRequestV2 &request,
                                         int32_t dst_rank) {
  ctx_->lctx->Send(dst_rank, request.SerializeAsString(), "Handshake");

  SPDLOG_INFO("send HandshakeRequest:\n{}", request.DebugString());
}

void AlgoV2Handler::SendHandshakeResponse(const HandshakeResponseV2 &response) {
  size_t self_rank = ctx_->lctx->Rank();
  size_t world_size = ctx_->lctx->WorldSize();
  for (size_t i = 0; i < world_size; ++i) {
    if (i == self_rank) {
      continue;
    }
    ctx_->lctx->SendAsync(i, response.SerializeAsString(),
                          "Handshake_response");
    SPDLOG_INFO("send HandshakeResponse:\n{}", response.DebugString());
  }
}

HandshakeResponseV2 AlgoV2Handler::RecvHandshakeResponse(int32_t src_rank) {
  HandshakeResponseV2 response;
  auto buf = ctx_->lctx->Recv(src_rank, "Handshake_response");
  YACL_ENFORCE(response.ParseFromArray(buf.data(), buf.size()),
               "handshake: parse from array fail");
  SPDLOG_INFO("recv HandshakeResponse:\n{}", response.DebugString());

  return response;
}

}  // namespace ic_impl
