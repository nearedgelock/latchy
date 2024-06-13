/*
 * Copyright 2024 NearEDGE, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <string>

#include "latchyConfiguration.pb.h"

#include <google/protobuf/message.h>

namespace configuration {

  const std::string                 prettyPrintJson(const std::string& json);
  std::string                       convertMsgToJsonString(const google::protobuf::Message& m);
  model::latchy::secretList         parseStringToMsg(const std::string& s);
  void                              printSecret(const model::latchy::secretDeclaration& secret);

  using secretCfg_t =               model::latchy::secretDeclaration;
  using secretCfgList_t =           model::latchy::secretList;

  secretCfgList_t                   parseStringToMsg(std::string& inputConfiguration);
} // namespace configuration

