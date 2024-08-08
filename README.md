# 隐私计算平台互联互通协议参考实现

本仓库存放隐私计算平台互联互通协议参考实现的代码

## 构建

interconnection-impl 引用了 spu 仓库代码，需要根据[ spu 构建前提](https://github.com/secretflow/spu/blob/main/CONTRIBUTING.md#build)在编译环境上安装好依赖库

然后执行以下构建指令：

```shell
bazel build -c opt ic_impl/ic_main
```

## 运行 ECDH-PSI

### 命令行传参

本地同时执行以下两条指令：

```shell
bazel run -c opt ic_impl/ic_main -- -rank=0 -algo=ECDH_PSI -protocol_families=ECC \
        -in_path ic_impl/data/psi_1.csv -field_names id -out_path /tmp/p1.out \
        -parties=127.0.0.1:9530,127.0.0.1:9531
```

```shell
bazel run -c opt ic_impl/ic_main -- -rank=1 -algo=ECDH_PSI -protocol_families=ECC \
        -in_path ic_impl/data/psi_2.csv -field_names id -out_path /tmp/p2.out \
        -parties=127.0.0.1:9530,127.0.0.1:9531
```

### 环境变量传参

为满足北京金融科技产业联盟的调度层互联互通标准对算法组件接口的要求，interconnection-impl 支持 ECDH-PSI 算法从环境变量读取配置参数

当某个参数在环境变量和命令行选项都被指定时，优先选择读取环境变量参数

程序运行需要关闭握手过程：
```shell
bazel run -c opt ic_impl/ic_main -- -disable_handshake=1
```

ECDH-PSI 算法配置的环境变量如下表所示。环境变量设置可参考 [ecdh-psi-env-alice.sh](./ic_impl/env/ecdh-psi-env-alice.sh) 和 [ecdh-psi-env-bob.sh](./ic_impl/env/ecdh-psi-env-bob.sh)

| 环境变量                                                   |                   参考值                    |                          描述                          |
|:-------------------------------------------------------|:----------------------------------------:|:----------------------------------------------------:|
| runtime.component.parameter.algo                       |                 ecdh_psi                 |                      algorithm                       |
| runtime.component.parameter.protocol_families          |                   ecc                    |      comma-separated list of protocol families       |
| runtime.component.parameter.curve_type                 |                curve25519                |                 elliptic curve type                  |
| runtime.component.parameter.hash_type                  |                 sha_256                  |                      hash type                       |
| runtime.component.parameter.hash2curve_strategy        |          direct_hash_as_point_x          |                hash to curve strategy                |
| runtime.component.parameter.point_octet_format         |               uncompressed               |              point Octet-String format               |
| runtime.component.parameter.bit_length_after_truncated |                    -1                    | optimization method: secondary ciphertext truncation |
| system.storage.host.url                                |           file://path/to/root            |            root path of input/output file            |
| runtime.component.input.train_data                     | {"namespace":"data","name":"psi_1.csv"}  |         relative path and name of input file         |
| runtime.component.parameter.field_names                |                    id                    |                     field names                      |
| runtime.component.parameter.result_to_rank             |                    -1                    |              which rank gets the result              |
| runtime.component.output.train_data                    | {"namespace":"output","name":"result_a"} |        relative path and name of output file         |

## 运行 SS-LR

### 启动 Beaver 服务

运行 SS-LR 之前，需要先启动 Beaver 服务。Beaver 服务的代码位于 SPU 仓库中，需要将 SPU 代码克隆到本地并编译：

```shell
git clone git@github.com:secretflow/spu.git
cd spu && bazel build -c opt libspu/mpc/semi2k/beaver/beaver_impl/ttp_server:beaver_server_main
```

然后生成 Beaver 服务的公钥和私钥：

```
bazel-bin/libspu/mpc/semi2k/beaver/beaver_impl/ttp_server/beaver_server_main -gen_key=true
```

最后启动 Beaver 服务，将上一步生成的私钥通过命令行参数传递给 Beaver 服务：

```
bazel-bin/libspu/mpc/semi2k/beaver/beaver_impl/ttp_server/beaver_server_main -port=9449 -server_private_key=LS0tLS1CRUdJTiBQUklWQVRFIEtFWS0tLS0tCk1JR0lBZ0VBTUJRR0NDcUJITTlWQVlJdEJnZ3FnUnpQVlFHQ0xRUnRNR3NDQVFFRUlJVnRVS1JEalVERFptZ3cKL0xUd0dYUmZXVFM5MStTSEhqODAwNnc2SUUxNW9VUURRZ0FFdER5RHNLM0RQN3YyWmdEdjZYNVQySnMzdGtmNQpPYXVBUEdXTHErTlhuMW1HYkd5N3pIZEVaa0FvNERDSGZyRmVuRWFCckxXMFZxUUtUY3QxUzJUYXpnPT0KLS0tLS1FTkQgUFJJVkFURSBLRVktLS0tLQo=
```

### 命令行传参

启动 Beaver 服务后，本地同时执行以下两条指令，命令行参数包括上述 Beaver 服务生成的公钥：

```shell
bazel run -c opt ic_impl/ic_main -- -rank=0 -algo=SS_LR -protocol_families=SS \
        -dataset=ic_impl/data/perfect_logit_a.csv -has_label=true \
        -use_ttp=true -ttp_server_host=127.0.0.1:9449 \
        -parties=127.0.0.1:9530,127.0.0.1:9531 -ttp_asym_crypto_schema=sm2 \
        -ttp_public_key=LS0tLS1CRUdJTiBQVUJMSUMgS0VZLS0tLS0KTUZvd0ZBWUlLb0VjejFVQmdpMEdDQ3FCSE05VkFZSXRBMElBQkxROGc3Q3R3eis3OW1ZQTcrbCtVOWliTjdaSAorVG1yZ0R4bGk2dmpWNTlaaG14c3U4eDNSR1pBS09Bd2gzNnhYcHhHZ2F5MXRGYWtDazNMZFV0azJzND0KLS0tLS1FTkQgUFVCTElDIEtFWS0tLS0tCg==
```

```shell
bazel run -c opt ic_impl/ic_main -- -rank=1 -algo=SS_LR -protocol_families=SS \
        -dataset=ic_impl/data/perfect_logit_b.csv -has_label=false \
        -use_ttp=true -ttp_server_host=127.0.0.1:9449 \
        -parties=127.0.0.1:9530,127.0.0.1:9531 -ttp_asym_crypto_schema=sm2 \
        -ttp_public_key=LS0tLS1CRUdJTiBQVUJMSUMgS0VZLS0tLS0KTUZvd0ZBWUlLb0VjejFVQmdpMEdDQ3FCSE05VkFZSXRBMElBQkxROGc3Q3R3eis3OW1ZQTcrbCtVOWliTjdaSAorVG1yZ0R4bGk2dmpWNTlaaG14c3U4eDNSR1pBS09Bd2gzNnhYcHhHZ2F5MXRGYWtDazNMZFV0azJzND0KLS0tLS1FTkQgUFVCTElDIEtFWS0tLS0tCg==
```

### 环境变量传参

为满足北京金融科技产业联盟的调度层互联互通标准对算法组件接口的要求，interconnection-impl 支持 SS-LR 算法从环境变量读取配置参数

当某个参数在环境变量和命令行选项都被指定时，优先选择读取环境变量参数

程序运行需要关闭握手过程：
```shell
bazel run -c opt ic_impl/ic_main -- -disable_handshake=1
```

SS-LR 算法配置的环境变量如下表所示。环境变量设置可参考 [ss-lr-env-alice.sh](./ic_impl/env/ss-lr-env-alice.sh) 和 [ss-lr-env-bob.sh](./ic_impl/env/ss-lr-env-bob.sh)

| 环境变量                                               |                        参考值                        |                            描述                             |
|:---------------------------------------------------|:-------------------------------------------------:|:---------------------------------------------------------:|
| runtime.component.parameter.algo                   |                       ss_lr                       |                         algorithm                         |
| runtime.component.parameter.protocol_families      |                        ss                         |         comma-separated list of protocol families         |
| runtime.component.parameter.batch_size             |                        21                         |                    size of each batch                     |
| runtime.component.parameter.last_batch_policy      |                      discard                      |  policy to process the partial last batch of each epoch   |
| runtime.component.parameter.num_epoch              |                         1                         |                      number of epoch                      |
| runtime.component.parameter.l0_norm                |                         0                         |                      l0 penalty term                      |
| runtime.component.parameter.l1_norm                |                         0                         |                      l1 penalty term                      |
| runtime.component.parameter.l2_norm                |                        0.5                        |                      l2 penalty term                      |
| runtime.component.parameter.optimizer              |                        sgd                        |        optimization algorithm to speed up training        |
| runtime.component.parameter.learning_rate          |                      0.0001                       |         learning rate parameter in sgd optimizer          |
| runtime.component.parameter.sigmoid_mode           |                     minimax_1                     |               sigmoid approximation method                |
| runtime.component.parameter.protocol               |                      semi2k                       |                     ss protocol type                      |
| runtime.component.parameter.field                  |                        64                         |        field type, 64 for Ring64, 128 for Ring128         |
| runtime.component.parameter.fxp_bits               |                        18                         |       number of fraction bits of fixed-point number       |
| runtime.component.parameter.trunc_mode             |                   probabilistic                   |                      truncation mode                      |
| runtime.component.parameter.shard_serialize_format |                        raw                        | serialization format used for communicating secret shares |
| runtime.component.parameter.use_ttp                |                       true                        |               whether to use beaver service               |
| runtime.component.parameter.ttp_server_host        |                      ip:port                      |   remote ip:port or load-balance uri of beaver service    |
| runtime.component.parameter.ttp_asym_crypto_schema |                        sm2                        |           asym_crypto_schema of beaver service            |
| runtime.component.parameter.ttp_public_key         |                                                   |               public key of beaver service                |
| runtime.component.parameter.ttp_adjust_rank        |                         0                         |      which rank do adjust rpc call to beaver service      |
| system.storage.host.url                            |                file://path/to/root                |              root path of input/output file               |
| runtime.component.input.train_data                 | {"namespace":"data","name":"perfect_logit_a.csv"} |           relative path and name of input file            |
| runtime.component.parameter.skip_rows=1            |                         1                         |            number of skipped rows from dataset            |
| runtime.component.parameter.label_owner            |                      host.0                       |             which party owns the label column             |
| runtime.component.parameter.feature_nums           |            {"host.0":10, "guest.0":10}            |             feature column nums of each party             |
| runtime.component.output.train_data                |     {"namespace":"output","name":"result_a"}      |           relative path and name of output file           |

## FAQ

若构建失败并提示 `Host key verification failed`，解决方式如下:

* 首先，[向github账户添加SSH key](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account)

* 然后在本地执行以下指令(以选择 RSA 作为 SSH key 类型为例)：

```shell
ssh-keyscan -t rsa github.com >> ~/.ssh/known_hosts
```