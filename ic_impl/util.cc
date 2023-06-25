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

#include "ic_impl/util.h"

#include <cstdlib>

#include "absl/strings/str_split.h"
#include "spdlog/spdlog.h"
#include "yacl/link/factory.h"

namespace ic_impl::util {

std::shared_ptr<yacl::link::Context> CreateLinkContextForBlackBox() {
  yacl::link::ContextDesc desc;
  desc.brpc_channel_protocol = "http";
  size_t self_rank;
  yacl::link::FactoryBrpcBlackBox::GetPartyNodeInfoFromEnv(desc.parties,
                                                           self_rank);
  return yacl::link::FactoryBrpcBlackBox().CreateContext(desc, self_rank);
}

std::shared_ptr<yacl::link::Context> CreateLinkContextForWhiteBox(
    std::string_view parties, int32_t self_rank) {
  yacl::link::ContextDesc lctx_desc;
  std::vector<std::string> hosts = absl::StrSplit(parties, ',');
  for (size_t rank = 0; rank < hosts.size(); rank++) {
    const auto id = fmt::format("party{}", rank);
    lctx_desc.parties.push_back({id, hosts[rank]});
  }

  return yacl::link::FactoryBrpc().CreateContext(lctx_desc, self_rank);
}

std::shared_ptr<yacl::link::Context> MakeLink(std::string_view parties,
                                              int32_t self_rank) {
  std::shared_ptr<yacl::link::Context> lctx;
  try {
    lctx = CreateLinkContextForBlackBox();
  } catch (const std::exception& e) {
    SPDLOG_INFO("create link context for blackbox failed: {}", e.what());
    lctx = CreateLinkContextForWhiteBox(parties, self_rank);
  }

  lctx->ConnectToMesh();
  return lctx;
}

std::set<int32_t> Intersection(const std::vector<std::set<int32_t>>& sets) {
  if (sets.empty()) {
    return {};
  }
  std::set<int32_t> last_intersection = sets[0];
  std::set<int32_t> cur_intersection;
  for (std::set<int32_t>::size_type i = 1; i < sets.size(); ++i) {
    std::set_intersection(
        last_intersection.begin(), last_intersection.end(), sets[i].begin(),
        sets[i].end(),
        std::inserter(cur_intersection, cur_intersection.begin()));
    std::swap(last_intersection, cur_intersection);
    cur_intersection.clear();
  }

  return last_intersection;
}

int32_t GetFlagValue(const google::protobuf::EnumDescriptor* descriptor,
                     std::string_view prefix, std::string_view name) {
  const auto* value = descriptor->FindValueByName(
      absl::StrCat(prefix, absl::AsciiStrToUpper(name)));
  YACL_ENFORCE(value, "Commandline flag {} is unsupported", name);
  YACL_ENFORCE(value->number() != 0, "Unspecified enum value");

  return value->number();
}

std::vector<int32_t> GetFlagValues(
    const google::protobuf::EnumDescriptor* descriptor, std::string_view prefix,
    std::string_view names) {
  std::vector<int32_t> flag_values;
  std::vector<std::string> name_vec = absl::StrSplit(names, ',');
  for (const auto& name : name_vec) {
    const auto* value = descriptor->FindValueByName(
        absl::StrCat(prefix, absl::AsciiStrToUpper(name)));
    if (value) {
      flag_values.push_back(value->number());
    } else {
      SPDLOG_WARN("Protocol family {} not valid", name);
    }
  }

  return flag_values;
}

bool ToBool(std::string_view str) {
  return absl::AsciiStrToLower(str) == "true";
}

char* GetParamEnv(std::string_view env_name) {
  return std::getenv(
      absl::StrCat("runtime.component.parameter.", env_name).c_str());
}

}  // namespace ic_impl::util
