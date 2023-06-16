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

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include "ic_impl/context.h"
#include "ic_impl/party.h"

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  // spdlog::set_level(spdlog::level::debug);
  try {
    auto ctx = ic_impl::CreateIcContext();
    if (ctx) {
      ic_impl::Party party(ctx);
      party.Run();
      ctx->lctx->WaitLinkTaskFinish();
    }
  } catch (const std::exception& e) {
    SPDLOG_ERROR("run failed: {}", e.what());
    return -1;
  }

  return 0;
}
