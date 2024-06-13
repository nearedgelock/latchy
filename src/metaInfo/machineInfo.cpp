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
#include "machineInfo.h"

#include <filesystem>
#include "fileAccess.h"

namespace meta {
namespace machine {

  info::info() {
    try {
      data = helpers::fileAccess::readAll("/etc/machine-id");
    } catch (...) {}
  }

  hostname::hostname() {
    try {
      data = helpers::fileAccess::readAll("/etc/hostname");;
    } catch (...) {}
  }
} // namespace machine
} // namespace meta
