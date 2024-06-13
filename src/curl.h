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
#include <stdexcept>
#include <atomic>

namespace curlWrapper {
  void globalInit();
  void globalCleanUp();

  std::string                 keyRecoverViaTang(const std::string& url, const std::string& kid, const std::string& key, const std::string& queryString, const std::atomic_bool& cancelled );

  // Exception classes, including an overall base exception class
  class curlException: public std::runtime_error {
  public:
    curlException(const std::string& msg = ""): runtime_error("Exception using CURL (use --trace to get more info)" + ((msg.empty() == false) ? (" - " + msg) : "")) { };
  };

  // Tang failure - Temporary in nature, such as no network, busy server, etc.
  class failedTangInteraction: public curlException {
  public:
    failedTangInteraction(const std::string& msg = ""): curlException ("Error communicating with tang " + ((msg.empty() == false) ? (" - " + msg) : "")) { };
  };

  class permanentTangFailure: public curlException {
  public:
    permanentTangFailure(const std::string& msg = ""): curlException ("Permanent failure from tang " + ((msg.empty() == false) ? (" - " + msg) : "")) { };
  };
} // namespace curlWrapper

