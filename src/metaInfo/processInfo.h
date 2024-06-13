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

#include "metaInfo.h"

#include <string>
#include <sstream>
#include <unistd.h>
#include <chrono>
namespace meta {
namespace process {

  /// @brief Collection of information about the process, including parent, user, etc.
  ///
  /// From the context of meta data, the information is volatile since is changes at
  /// every restart of the process. Some of the information, though, is static within
  /// the scope of the process itself. Other, such as effective UID, may change during
  /// course of the life of the process.
  class info: public meta::info {
  public:
    using timepoint_t =           std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
  public:
    info();
    
    virtual bool                  isVolatile() { return true; };

    virtual const std::string     rawData() const { return data.str(); };

    void                          printInfo() const;
  private:
    std::stringstream             data;   // Static data, established at start time.

    // Extra
    void                          populateData();
    uid_t                         realUser_ID = 0;
    uid_t                         effectiveUser_ID = 0;
    gid_t                         realGroup_ID = 0;
    gid_t                         effectiveGroup_ID = 0;

    pid_t                         process_ID = 0;
    pid_t                         parentProcess_ID = 0;

    timepoint_t                   startTime = std::chrono::system_clock::now();
    
    std::string                   process_Name;       // Does NOT include the path name
    std::string                   process_FullName;   // As it was available when we were instantiated. In canonical form
    std::string                   process_Location;   // Process pathname
  };

} // namespace process
} // namespace meta
