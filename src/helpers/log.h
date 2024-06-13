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

#include <iostream>
#include <unistd.h>
#include "fileAccess.h"

namespace logger {

  // Enable disable
  extern bool                     ISDEBUG;
  extern bool                     ISINFO;

  // Operational parameters
  extern bool                     USESTDERR;
  extern std::filesystem::path    LOGFILE;
  extern std::string              PREFIX;   // Prefix added in fron of each line, usually set to application name

// Unconditionally send an output to stdout
#define USERCOUT() std::cout

// Unconditionally send an output to stdout or stderr
#define USERMSG() ((false == logger::USESTDERR) ? std::cout << logger::PREFIX : std::cerr << logger::PREFIX)

// Log to file if LOGFILE is not empty
#define LOGTOFILE(m) if (false == logger::LOGFILE.empty()) helpers::fileAccess::writeTo(logger::LOGFILE, logger::PREFIX + m, true)

// Conditionally output if ISDEBUG is true. 
#define DEBUG() if (true == logger::ISDEBUG) ((false == logger::USESTDERR) ? std::cout << logger::PREFIX : std::cerr << logger::PREFIX)
// Conditionally output if ISINFO is true. 
#define INFO() if (true == logger::ISINFO) ((false == logger::USESTDERR) ? std::cout << logger::PREFIX : std::cerr << logger::PREFIX )

// Set the logfile to something in /tmp
#define ACTIVATELOG() logger::LOGFILE = "/tmp/latchy." + std::to_string(getpid()) + ".log";

} // namespace logger

