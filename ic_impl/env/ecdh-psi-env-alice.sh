#!/bin/sh

exec env \
"system.storage.host.url=file:/$(dirname "$PWD")" \
'runtime.component.input.train_data={"namespace":"data","name":"psi_1.csv"}' \
'runtime.component.output.train_data={"namespace":"output","name":"result_a"}' \
"runtime.component.parameter.algo=ecdh_psi" \
"runtime.component.parameter.protocol_families=ecc" \
"runtime.component.parameter.curve_type=curve25519" \
"runtime.component.parameter.hash_type=sha_256" \
"runtime.component.parameter.hash2curve_strategy=direct_hash_as_point_x" \
"runtime.component.parameter.point_octet_format=uncompressed" \
"runtime.component.parameter.bit_length_after_truncated=-1" \
"runtime.component.parameter.result_to_rank=-1" \
"runtime.component.parameter.field_names=id" \
"runtime.component.parameter.start_transport=true" \
"system.transport=127.0.0.1:9972" \
"config.self_role=host.0" \
"config.inst_id.host.0=alice" \
"config.inst_id.guest.0=bob" \
"config.node_id.host.0=alice" \
"config.node_id.guest.0=bob" \
"config.node_ip.host.0=127.0.0.1:9972" \
"config.node_ip.guest.0=127.0.0.1:9973" \
"config.task_id=job-alice-2-bob-ss-lr-13-106be91da17e" \
"config.session_id=session_job-alice-2-bob-ss-lr-13-106be91da17e" \
"config.trace_id=trace_job-alice-2-bob-ss-lr-13-106be91da17e" \
"config.token=token_job-alice-2-bob-ss-lr-13-106be91da17e" \
$SHELL
