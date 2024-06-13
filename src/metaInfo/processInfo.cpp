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
#include "processInfo.h"

#include <filesystem>
#include "fileAccess.h"

namespace meta {
namespace process {

  info::info() {
    populateData();

    // Populate data with info we are sure won't change during the life of the process
    data.str("");
    data << realUser_ID << itemSeparator << realGroup_ID << itemSeparator << process_ID << itemSeparator << process_FullName << itemSeparator << startTime.time_since_epoch().count();
  }

  void info::printInfo() const {
    INFO() << "Basic process information for " << process_FullName << std::endl;
    INFO() << "\tPID                   " << process_ID << std::endl;
    INFO() << "\tParent PID            " << parentProcess_ID << std::endl;
    INFO() << "\tUser ID               " << realUser_ID << std::endl;
    INFO() << "\tEffective User ID     " << effectiveUser_ID << std::endl;
    INFO() << "\tGroup ID              " << realGroup_ID << std::endl;
    INFO() << "\tEffective group PID   " << effectiveGroup_ID << std::endl;

  }

  void info::populateData() {
    realUser_ID = getuid();
    effectiveUser_ID = geteuid();
    realGroup_ID = getgid();
    effectiveGroup_ID = getegid();

    process_ID = getpid();
    parentProcess_ID = getppid();

    process_FullName = std::filesystem::canonical("/proc/self/exe");

    std::size_t   pos;
    pos = process_FullName.find_last_of('/');
    if (pos != std::string::npos) {
      process_Name = process_FullName.substr(pos+1);
      process_Location = process_FullName.substr(0, pos);
    }
  }

} // namespace process
} // namespace meta
