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

#include "ic_impl/algo/lr/optimizer.h"

#include "gflags/gflags.h"

#include "ic_impl/util.h"

DEFINE_string(optimizer, "sgd", "optimization algorithm to speed up training");

DEFINE_double(learning_rate, 0.0001,
              "learning rate parameter of sgd optimizer");

namespace ic_impl::algo::optimizer {

int32_t SuggestedOptimizerType() {
  std::string_view optimizer = FLAGS_optimizer;
  if (char* env = util::GetParamEnv("optimizer")) {
    optimizer = env;
  }

  return util::GetFlagValue(
      org::interconnection::v2::algos::Optimizer_descriptor(), "OPTIMIZER_",
      optimizer);
}

double SuggestedLearningRate() {
  if (char* env = util::GetParamEnv("learning_rate")) {
    return std::stod(env);
  }
  return FLAGS_learning_rate;
}

Optimizer SuggestedOptimizer() {
  Optimizer optimizer;
  optimizer.type = SuggestedOptimizerType();

  switch (optimizer.type) {
    case org::interconnection::v2::algos::OPTIMIZER_SGD: {
      org::interconnection::v2::algos::SgdOptimizer optimizer_param;
      optimizer_param.set_learning_rate(SuggestedLearningRate());
      optimizer.param = optimizer_param;
      break;
    }
    case org::interconnection::v2::algos::OPTIMIZER_MOMENTUM:
    case org::interconnection::v2::algos::OPTIMIZER_ADAGRAD:
    case org::interconnection::v2::algos::OPTIMIZER_ADADELTA:
    case org::interconnection::v2::algos::OPTIMIZER_RMSPROP:
    case org::interconnection::v2::algos::OPTIMIZER_ADAM:
    case org::interconnection::v2::algos::OPTIMIZER_ADAMAX:
    case org::interconnection::v2::algos::OPTIMIZER_NADAM: {
      YACL_THROW("Unimplemented optimizer type");
      break;
    }
    default: {
      YACL_THROW("Unspecified optimizer type");
    }
  }

  return optimizer;
}

}  // namespace ic_impl::algo::optimizer
