# Copyright 2023 Ant Group Co., Ltd.
# Copyright 2023 Tsing Jiao Information Science
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from anyconn_core import AppConfig
from router import RC
import logging

logger = logging.getLogger("anyconn")

router_table = {
    "rs_01": {"rs": ["rs_02"], "rc": ["rc_01", "rc_03"]},
    "rs_02": {"rs": ["rs_01"], "rc": ["rc_02", "rc_04"]},
}

nodes = [
    {"id": "rs_01", "tag": "RS", "address": "localhost:50051"},
    {"id": "rs_02", "tag": "RS", "address": "localhost:50052"},
    {"id": "rc_01", "tag": "RC01", "address": "localhost:50061"},
    {"id": "rc_02", "tag": "RC02", "address": "localhost:50062"},
    {"id": "rc_03", "tag": "RC", "address": "localhost:50063"},
    {"id": "rc_04", "tag": "RC", "address": "localhost:50064"},
]


def run_ss_rc(_id):
    rc = RC()
    with rc.run_in_thread(AppConfig(node_id=_id, nodes=nodes)):
        pack = rc.recv()[0]
        share = [d for d in pack.data]
        logger.info(f"{_id} share:{share}")
        return share


def main():
    run_ss_rc("rc_02")


if __name__ == "__main__":
    main()
