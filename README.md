# 隐私计算平台互联互通协议参考实现

---
本仓库存放隐私计算平台互联互通协议参考实现代码。

---

run ECDH-PSI:

cd interconnection-impl/ic_impl

bazel run -c dbg --define tracelog=on ic_main -- -rank=0 -in_path ic_impl/data/psi_1.csv -field_names id -out_path /tmp/p1.out

bazel run -c dbg --define tracelog=on ic_main -- -rank=1 -in_path ic_impl/data/psi_2.csv -field_names id -out_path /tmp/p2.out

---

run SS-LR:

cd interconnection-impl/ic_impl

bazel run -c dbg --define tracelog=on ic_main -- -rank=0 -algo=SS_LR -protocol_families=SS --dataset=ic_impl/data/perfect_logit_a.csv --has_label=true --use_ttp=true -ttp_server_host=127.0.0.1:9449 -parties=127.0.0.1:9530,127.0.0.1:9531

bazel run -c dbg --define tracelog=on ic_main -- -rank=1 -algo=SS_LR -protocol_families=SS --dataset=ic_impl/data/perfect_logit_b.csv --has_label=false --use_ttp=true -ttp_server_host=127.0.0.1:9449 -parties=127.0.0.1:9530,127.0.0.1:9531

---

run Beaver service:

git clone git@github.com:secretflow/spu.git

cd spu && bazel run libspu/mpc/semi2k/beaver/ttp_server:beaver_server_main