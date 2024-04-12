# Copyright 2023 Ant Group Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

SECRETFLOW_GIT = "https://github.com/secretflow"

SPU_COMMIT_ID  = "8bf3c97da503f1cffd1292c8e365ecbc30675400"

PSI_COMMIT_ID  = "c80ac38fd8e0df9860001d46bc064153c9e203c6"

IC_COMMIT_ID  = "30e4220b7444d0bb077a9040f1b428632124e31a"

SPU_REPOSITORY = "SPU"

def ic_impl_deps():
    _com_github_nlohmann_json()

    maybe(
        git_repository,
        name = "spulib",
        commit = SPU_COMMIT_ID,
        remote = "{}/{}.git".format(SECRETFLOW_GIT, SPU_REPOSITORY),
    )

    maybe(
        git_repository,
        name = "psi",
        commit = PSI_COMMIT_ID,
        remote = "{}/psi.git".format(SECRETFLOW_GIT),
    )

def protocol_deps():
    maybe(
        git_repository,
        name = "org_interconnection",
        commit = IC_COMMIT_ID,
        remote = "{}/interconnection.git".format(SECRETFLOW_GIT),
    )

def _com_github_nlohmann_json():
    maybe(
        git_repository,
        name = "com_github_nlohmann_json",
        commit = "5d2754306d67d1e654a1a34e1d2e74439a9d53b3",
        remote = "git@github.com:nlohmann/json.git",
    )
