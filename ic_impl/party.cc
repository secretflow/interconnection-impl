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

#include "ic_impl/party.h"

#include "spdlog/spdlog.h"

#include "ic_impl/context.h"
#include "ic_impl/factory.h"

namespace ic_impl {

using HandshakeRequestVariant =
    std::variant<std::monostate, HandshakeRequestV1, HandshakeRequestV2>;

namespace {
HandshakeRequestVariant ParseHandshakeReqeust(const yacl::Buffer &buf) {
  org::interconnection::v2::HandshakeVersionCheckHelper helper;
  YACL_ENFORCE(helper.ParseFromArray(buf.data(), buf.size()),
               "handshake: parse from array fail");

  HandshakeRequestVariant request_variant;
  if (helper.version() == 1) {
    request_variant.emplace<HandshakeRequestV1>();
  } else if (helper.version() >= 2) {
    request_variant.emplace<HandshakeRequestV2>();
  } else {
    YACL_THROW("recv invalid HandshakeRequest version {}", helper.version());
  }

  auto visitor = [&buf](auto &request) {
    if constexpr (std::is_convertible_v<decltype(request), std::monostate>) {
      ;  // wouldn't run here
    } else {
      YACL_ENFORCE(request.ParseFromArray(buf.data(), buf.size()),
                   "handshake: parse from array fail");
      SPDLOG_INFO("recv HandshakeRequest:\n{}", request.DebugString());
    }
  };

  std::visit(visitor, request_variant);
  return request_variant;
}

int32_t AlignHandshakeVersion(
    const std::vector<HandshakeRequestVariant> &request_variants) {
  int32_t version = 0;
  auto visitor = [&version](const auto &request) -> bool {
    if constexpr (std::is_convertible_v<decltype(request), std::monostate>) {
      return false;
    } else {
      if constexpr (std::is_convertible_v<decltype(request),
                                          HandshakeRequestV1>) {
        if (request.version() != 1) {
          return false;
        }
      } else if constexpr (std::is_convertible_v<decltype(request),
                                                 HandshakeRequestV2>) {
        if (request.version() < 2) {
          return false;
        }
      }

      if (version != 0 && version != request.version()) {
        return false;
      } else {
        version = request.version();
        return true;
      }
    }
  };

  for (const auto &request_variant : request_variants) {
    if (!std::visit(visitor, request_variant)) {
      return 0;
    }
  }

  return version;
}

void StripVariant(const std::vector<HandshakeRequestVariant> &request_variants,
                  std::vector<HandshakeRequestV1> &requests_v1,
                  std::vector<HandshakeRequestV2> &requests_v2) {
  auto visitor = [&requests_v1, &requests_v2](const auto &request) {
    if constexpr (std::is_convertible_v<decltype(request), std::monostate>) {
      YACL_THROW("Unexpected behavior");
    } else if constexpr (std::is_convertible_v<decltype(request),
                                               HandshakeRequestV1>) {
      requests_v1.push_back(request);
    } else {
      requests_v2.push_back(request);
    }
  };

  for (const auto &request_variant : request_variants) {
    std::visit(visitor, request_variant);
  }
}

}  // namespace

Party::Party(std::shared_ptr<IcContext> ctx) : ctx_(std::move(ctx)) {}

Party::~Party() = default;

void Party::Run() {
  int32_t self_rank = ctx_->lctx->Rank();
  if (ctx_->version == 1) {  // send request from rank 0 to rank 1
                             // int32_t recv_rank = 1;
  } else {                   // send requests from rank 1 ~ n-1 to rank 0
    auto factory = CreateHandlerFactory();
    YACL_ENFORCE(factory);
    auto handler_v2 = factory->CreateAlgoV2Handler(ctx_);

    int32_t recv_rank = 0;
    if (self_rank == recv_rank) {
      // Recv HandshakeRequests from rank 1 ~ n-1
      auto request_variants = RecvHandshakeRequests();

      // Check HandshakeRequest versions
      int32_t version = AlignHandshakeVersion(request_variants);
      if (version != ctx_->version) {
        SPDLOG_WARN("Handshake versions are inconsistent");
        HandshakeResponseV2 response;
        response.mutable_header()->set_error_code(
            org::interconnection::HANDSHAKE_REFUSED);
        response.mutable_header()->set_error_msg(
            "handshake versions inconsistent");
        handler_v2->SendHandshakeResponse(response);
        return;
      }

      std::vector<HandshakeRequestV1> requests_v1;
      std::vector<HandshakeRequestV2> requests_v2;
      StripVariant(request_variants, requests_v1, requests_v2);
      handler_v2->PassiveRun(requests_v2);
    } else {
      handler_v2->ActiveRun(recv_rank);
    }
  }
}

std::unique_ptr<AlgoHandlerFactory> Party::CreateHandlerFactory() {
  if (ctx_->algo == org::interconnection::v2::ALGO_TYPE_ECDH_PSI) {
    YACL_ENFORCE(!ctx_->protocol_families.empty());
    YACL_ENFORCE(ctx_->protocol_families.at(0) ==
                 org::interconnection::v2::PROTOCOL_FAMILY_ECC);
    SPDLOG_INFO("run ECDH-PSI");
    return std::make_unique<PsiV2HandlerFactory>();
  } else if (ctx_->algo == org::interconnection::v2::ALGO_TYPE_SS_LR) {
    YACL_ENFORCE(!ctx_->protocol_families.empty());
    YACL_ENFORCE(ctx_->protocol_families.at(0) ==
                 org::interconnection::v2::PROTOCOL_FAMILY_SS);
    SPDLOG_INFO("run SS-LR");
    return std::make_unique<LrHandlerFactory>();
  }

  SPDLOG_ERROR("Create algo handler failed");
  return nullptr;
}

std::vector<HandshakeRequestVariant> Party::RecvHandshakeRequests() {
  std::vector<HandshakeRequestVariant> request_variants;
  auto lctx = ctx_->lctx;
  size_t self_rank = lctx->Rank();
  size_t word_size = lctx->WorldSize();

  // Recv HandshakeRequest from rank 1 ~ n-1
  for (size_t i = 0; i < word_size; ++i) {
    if (i == self_rank) {
      continue;
    }
    auto buf = lctx->Recv(i, "Handshake");
    request_variants.push_back(ParseHandshakeReqeust(buf));
  }

  return request_variants;
}

}  // namespace ic_impl
