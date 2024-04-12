#!/bin/sh

exec env \
"system.storage.host.url=file:/$(dirname "$PWD")" \
'runtime.component.input.train_data={"namespace":"data", "name":"perfect_logit_b.csv"}' \
'runtime.component.output.train_data={"namespace":"output","name":"result_b"}' \
"runtime.component.parameter.algo=ss_lr" \
"runtime.component.parameter.optimizer=sgd" \
"runtime.component.parameter.learning_rate=0.0001" \
"runtime.component.parameter.num_epoch=1" \
"runtime.component.parameter.batch_size=21" \
"runtime.component.parameter.last_batch_policy=discard" \
"runtime.component.parameter.l0_norm=0" \
"runtime.component.parameter.l1_norm=0" \
"runtime.component.parameter.l2_norm=0.5" \
"runtime.component.parameter.sigmoid_mode=minimax_1" \
"runtime.component.parameter.protocol_families=ss" \
"runtime.component.parameter.protocol=semi2k" \
"runtime.component.parameter.field=64" \
"runtime.component.parameter.fxp_bits=18" \
"runtime.component.parameter.trunc_mode=probabilistic" \
"runtime.component.parameter.shard_serialize_format=raw" \
"runtime.component.parameter.use_ttp=true" \
"runtime.component.parameter.ttp_server_host=127.0.0.1:9449" \
"runtime.component.parameter.ttp_session_id=interconnection-root-1" \
"runtime.component.parameter.ttp_adjust_rank=0" \
"runtime.component.parameter.label_owner=host.0" \
'runtime.component.parameter.feature_nums={"host.0":10, "guest.0":10}' \
"runtime.component.parameter.skip_rows=1" \
"runtime.component.parameter.start_transport=true" \
"system.transport=127.0.0.1:9973" \
"config.self_role=guest.0" \
"config.node_id.host.0=alice" \
"config.node_id.guest.0=bob" \
"config.inst_id.host.0=alice" \
"config.inst_id.guest.0=bob" \
"config.node_ip.host.0=127.0.0.1:9972" \
"config.node_ip.guest.0=127.0.0.1:9973" \
"config.task_id=job-alice-2-bob-ss-lr-13-106be91da17e" \
"config.session_id=session_job-alice-2-bob-ss-lr-13-106be91da17e" \
"config.trace_id=trace_job-alice-2-bob-ss-lr-13-106be91da17e" \
"config.token=token_job-alice-2-bob-ss-lr-13-106be91da17e" \
$SHELL
