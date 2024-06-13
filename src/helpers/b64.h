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

#include <array>

#include "botan/base64.h"

//
// Some simply wrapper for base64 operation. The original impetus was to augment Botan to support
// the URL firnedly variant....
//
namespace misc {
  // Both MUST be the same size, we do not check!!
  constexpr std::array<char, 2>    urlFriendly = {'-', '_'};
  constexpr std::array<char, 2>    urlUnfriendly = {'+', '/'};

  inline std::string fromURL(std::string& src) {
    for (int i = 0; i < urlUnfriendly.size(); ++i) {   
      size_t pos = 0;
      while (pos != std::string::npos) {
        pos = src.find(urlFriendly[i], pos);
        if (pos != std::string::npos) {
          src.replace(pos, 1, 1, urlUnfriendly[i]);
        }
      }
    }

    return src;
  }

  inline std::string extractB64(const std::string& encoded, bool urlFriendly = false) {  
    if (urlFriendly == true) {
      std::string                        urlUnfriendly = encoded;
      const Botan::secure_vector<uint8_t>  decoded = Botan::base64_decode(fromURL(urlUnfriendly), true);
      return std::string(decoded.begin(), decoded.end());
    } else {
      const Botan::secure_vector<uint8_t>  decoded = Botan::base64_decode(encoded, true);
      return std::string(decoded.begin(), decoded.end());
    }
  };

  inline std::string extractB64(const char input[], size_t input_length, bool urlFriendly = false) {
    if (urlFriendly == true) {
      std::string                        urlUnfriendly(input, input_length);

      const Botan::secure_vector<uint8_t>  decoded = Botan::base64_decode(fromURL(urlUnfriendly), true);
      return std::string(decoded.begin(), decoded.end());
    } else {
      const Botan::secure_vector<uint8_t>  decoded = Botan::base64_decode(input, input_length, true);
      return std::string(decoded.begin(), decoded.end());
    }
  };
} // namespace misc

