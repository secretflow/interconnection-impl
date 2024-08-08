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
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

SECRETFLOW_GIT = "https://github.com/secretflow"

SPU_COMMIT_ID  = "74c6d54e5bff9e5d35ab5a73dda98cd9ccf0bfc8"

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

def protocol_deps():
    maybe(
        git_repository,
        name = "org_interconnection",
        commit = IC_COMMIT_ID,
        remote = "{}/interconnection.git".format(SECRETFLOW_GIT),
    )

def _com_github_nlohmann_json():
    maybe(
        http_archive,
        name = "com_github_nlohmann_json",
        sha256 = "0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406",
        strip_prefix = "json-3.11.3",
        type = "tar.gz",
        urls = [
            "https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz",
        ],
    )
