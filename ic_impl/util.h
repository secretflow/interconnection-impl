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

#pragma once

#include <optional>
#include <set>
#include <string_view>

#include "google/protobuf/reflection.h"
#include "yacl/base/exception.h"

#include "google/protobuf/any.pb.h"

namespace yacl::link {
class Context;
}

namespace ic_impl::util {

std::shared_ptr<yacl::link::Context> MakeLink(std::string_view parties,
                                              int32_t self_rank);

int32_t GetFlagValue(const google::protobuf::EnumDescriptor *descriptor,
                     std::string_view prefix, std::string_view name);

std::vector<int32_t> GetFlagValues(
    const google::protobuf::EnumDescriptor *descriptor, std::string_view prefix,
    std::string_view names);

template <typename T, size_t item_num>
bool IsFlagSupported(const std::array<T, item_num> &flag_list, T flag) {
  auto it = std::find(flag_list.begin(), flag_list.end(), flag);
  return it != flag_list.end();
}

template <typename T>
std::set<T> Intersection(const std::vector<std::set<T>> &sets) {
  if (sets.empty()) {
    return {};
  }
  std::set<T> last_intersection = sets[0];
  std::set<T> cur_intersection;
  for (typename std::set<T>::size_type i = 1; i < sets.size(); ++i) {
    std::set_intersection(
        last_intersection.begin(), last_intersection.end(), sets[i].begin(),
        sets[i].end(),
        std::inserter(cur_intersection, cur_intersection.begin()));
    std::swap(last_intersection, cur_intersection);
    cur_intersection.clear();
  }

  return last_intersection;
}

// Extract any type sub-message from message
template <typename MessageType, typename ParamType>
std::optional<ParamType> ExtractParam(const MessageType &message,
                                      int enum_field_num, int param_field_num,
                                      int32_t enumeration) {
  const google::protobuf::Descriptor *descriptor = MessageType::GetDescriptor();
  const google::protobuf::Reflection *reflection = MessageType::GetReflection();

  const auto *enum_field = descriptor->FindFieldByNumber(enum_field_num);
  YACL_ENFORCE(enum_field, "No field field number {} in message {}",
               enum_field_num, descriptor->name());
  const auto *param_field = descriptor->FindFieldByNumber(param_field_num);
  YACL_ENFORCE(param_field, "No field field number {} in message {}",
               param_field_num, descriptor->name());

  const auto &enums =
      reflection->GetRepeatedFieldRef<int32_t>(message, enum_field);
  const auto params = reflection->GetRepeatedFieldRef<google::protobuf::Any>(
      message, param_field);

  int i = 0;
  for (; i < enums.size(); ++i) {
    if (enums.Get(i) == enumeration) {
      break;
    }
  }

  if (i >= enums.size() || i >= params.size()) {
    return std::nullopt;
  }

  ParamType param;
  if (params.Get(i, nullptr).UnpackTo(&param)) {
    return std::make_optional(param);
  }

  return std::nullopt;
}

template <typename MessageType, typename ParamType>
std::vector<ParamType> ExtractParams(const std::vector<MessageType> &messages,
                                     int enum_field_num, int param_field_num,
                                     int32_t enumeration) {
  std::vector<ParamType> params;
  for (const auto &message : messages) {
    auto param = ExtractParam<MessageType, ParamType>(
        message, enum_field_num, param_field_num, enumeration);
    if (!param) {
      return std::vector<ParamType>();
    }

    params.push_back(param.value());
  }

  return params;
}

template <typename MessageType, typename ParamType>
std::vector<ParamType> ExtractParams(const std::vector<MessageType> &messages,
                                     int field_num) {
  const google::protobuf::Descriptor *descriptor = MessageType::GetDescriptor();
  const google::protobuf::Reflection *reflection = MessageType::GetReflection();
  const auto *param_field = descriptor->FindFieldByNumber(field_num);
  YACL_ENFORCE(param_field, "No field number {} in message {}", field_num,
               descriptor->name());

  std::vector<ParamType> params;
  for (const auto &message : messages) {
    const auto &any_param = static_cast<const google::protobuf::Any &>(
        reflection->GetMessage(message, param_field));
    ParamType param;
    if (any_param.UnpackTo(&param)) {
      params.push_back(param);
    } else {
      return std::vector<ParamType>();
    }
  }

  return params;
}

template <typename ParamType, typename ItemType = int32_t>
std::set<ItemType> IntersectParamItems(const std::vector<ParamType> &params,
                                       int field_num) {
  std::vector<std::set<ItemType>> result;

  const google::protobuf::Descriptor *descriptor = ParamType::GetDescriptor();
  const google::protobuf::Reflection *reflection = ParamType::GetReflection();
  const auto *item_field = descriptor->FindFieldByNumber(field_num);
  YACL_ENFORCE(item_field, "No field number {} in message {}", field_num,
               descriptor->name());

  for (const auto &param : params) {
    const auto &items =
        reflection->GetRepeatedFieldRef<ItemType>(param, item_field);
    result.emplace_back(items.begin(), items.end());
  }

  return Intersection(result);
}

template <typename ParamType1, typename ParamType2>
std::set<int32_t> IntersectParamItems(const std::vector<ParamType1> &params,
                                      int field_num_1, int field_num_2) {
  std::vector<std::set<int32_t>> result;

  const google::protobuf::Descriptor *descriptor = ParamType1::GetDescriptor();
  const google::protobuf::Reflection *reflection = ParamType1::GetReflection();
  const auto *item_field = descriptor->FindFieldByNumber(field_num_1);
  YACL_ENFORCE(item_field, "No field number {} in message {}", field_num_1,
               descriptor->name());

  const google::protobuf::Descriptor *descriptor2 = ParamType2::GetDescriptor();
  const google::protobuf::Reflection *reflection2 = ParamType2::GetReflection();
  const auto *item_field_2 = descriptor2->FindFieldByNumber(field_num_2);
  YACL_ENFORCE(item_field_2, "No field number {} in message {}", field_num_2,
               descriptor2->name());

  for (const auto &param : params) {
    const auto &items =
        reflection->GetRepeatedFieldRef<ParamType2>(param, item_field);
    result.emplace_back();
    for (const auto &item : items) {
      auto value = reflection2->GetInt32(item, item_field_2);
      result.back().insert(value);
    }
  }

  return Intersection(result);
}

template <typename ParamType, typename ItemType>
std::optional<ItemType> AlignParamItem(const std::vector<ParamType> &params,
                                       int field_num) {
  const google::protobuf::Descriptor *descriptor = ParamType::GetDescriptor();
  const google::protobuf::Reflection *reflection = ParamType::GetReflection();
  const auto *item_field = descriptor->FindFieldByNumber(field_num);
  YACL_ENFORCE(item_field, "No field number {} in message {}", field_num,
               descriptor->name());

  std::optional<ItemType> last_value;
  for (const auto &param : params) {
    ItemType value;
    if constexpr (std::is_same_v<ItemType, bool>) {
      value = reflection->GetBool(param, item_field);
    } else if constexpr (std::is_same_v<ItemType, int32_t>) {
      value = reflection->GetInt32(param, item_field);
    } else if constexpr (std::is_same_v<ItemType, int64_t>) {
      value = reflection->GetInt64(param, item_field);
    } else {
      YACL_THROW("Not implemented");
    }

    if (!last_value.has_value()) {
      last_value = std::make_optional(value);
    } else {
      if (last_value.value() != value) {
        return std::nullopt;
      }
    }
  }

  return last_value;
}

template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
AlmostEqual(T x, T y, int ulp) {
  // the machine epsilon has to be scaled to the magnitude of the values used
  // and multiplied by the desired precision in ULPs (units in the last place)
  return std::fabs(x - y) <=
             std::numeric_limits<T>::epsilon() * std::fabs(x + y) * ulp
         // unless the result is subnormal
         || std::fabs(x - y) < std::numeric_limits<T>::min();
}

template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
AlmostZero(T x) {
  return AlmostEqual(x, static_cast<T>(0), 2);
}

bool ToBool(std::string_view str);

char *GetParamEnv(std::string_view env_name);

template <typename T>
T GetParamEnv(std::string_view env_name, const T &default_value) {
  if (char *env = GetParamEnv(env_name)) {
    if constexpr (std::is_convertible_v<T, std::string>) {
      return env;
    } else if constexpr (std::is_same_v<T, bool>) {
      return ToBool(env);
    } else if constexpr (std::is_same_v<T, int32_t>) {
      return std::stoi(env);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      return std::stoll(env);
    } else if constexpr (std::is_same_v<T, double>) {
      return std::stod(env);
    } else {
      YACL_THROW("Not implemented");
    }
  }

  return default_value;
}

}  // namespace ic_impl::util
