#!/bin/bash

cd ..
rm -rf build
mkdir build
bazel build ic_impl/ic_main
cp bazel-bin/ic_impl/ic_main build
cp -r ic_impl/data build
cp docker/Dockerfile build
cd build && docker build -f Dockerfile -t ss-lr:v1.0.0 .
