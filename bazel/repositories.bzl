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

SPU_COMMIT_ID  = "9fd70780b4fd9a5ba67140a46b35db76d393c3eb"

IC_COMMIT_ID  = "aa951609c533091ff9ffb38d4bd60f6e2aba7e60"

SPU_REPOSITORY = "SPU"

def ic_impl_deps():
    maybe(
        git_repository,
        name = "spulib",
        commit = SPU_COMMIT_ID,
        remote = "{}/{}.git".format(SECRETFLOW_GIT, SPU_REPOSITORY),
    )

def protocol_deps():
    maybe(
        git_repository,
        name = "org_interconnection",
        commit = IC_COMMIT_ID,
        remote = "{}/interconnection.git".format(SECRETFLOW_GIT),
    )
