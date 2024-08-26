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

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "yacl/link/factory.h"
#include "yacl/link/transport/blackbox_interconnect/mock_transport.h"

namespace ic_impl::util {

namespace {

void StartTransport() {
  static yacl::link::transport::blackbox_interconnect::MockTransport transport;

  brpc::ChannelOptions options;
  {
    options.protocol = "http";
    options.connection_type = "";
    options.connect_timeout_ms = 20000;
    options.timeout_ms = 1e4;
    options.max_retry = 3;
  }

  transport.StartFromEnv(options);
}

}  // namespace

std::shared_ptr<yacl::link::Context> CreateLinkContextForBlackBox() {
  yacl::link::ContextDesc desc;
  desc.brpc_channel_protocol = "http";
  size_t self_rank;
  yacl::link::FactoryBrpcBlackBox::GetPartyNodeInfoFromEnv(desc.parties,
                                                           self_rank);

  if (util::GetParamEnv("start_transport", false)) {
    StartTransport();
  }

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

namespace {

std::optional<std::string> GetIoFileNameFromEnv(bool input) {
    char* host_url = std::getenv("system.storage");
    if (!host_url || !absl::StartsWith(host_url, "file://")) {
        host_url = std::getenv("system.storage.host.url");
        if (!host_url) {
            host_url = std::getenv("system.storage.localfile");
        }
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

}  // namespace

inline std::optional<std::string> GetInputFileNameFromEnv() {
  return GetIoFileNameFromEnv(true);
}

inline std::optional<std::string> GetOutputFileNameFromEnv() {
  return GetIoFileNameFromEnv(false);
}

std::string GetInputFileName(std::string_view default_file) {
  std::string_view input_file_name = default_file;
  auto input_optional = util::GetInputFileNameFromEnv();
  if (input_optional.has_value()) {
    input_file_name = input_optional.value();
  }

  return input_file_name.data();
}

std::string GetOutputFileName(std::string_view default_file) {
  std::string_view output_file_name = default_file;
  auto output_optional = util::GetOutputFileNameFromEnv();
  if (output_optional.has_value()) {
    output_file_name = output_optional.value();
  }

  return output_file_name.data();
}

}  // namespace ic_impl::util
