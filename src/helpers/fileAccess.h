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
#include <filesystem>

namespace helpers {
namespace fileAccess {
  using namespace std;

 void                           writeTo(const std::filesystem::path& file, const std::string& data, bool append);
 const std::string              readAll(const std::filesystem::path& file);
 const std::string              getSymlink(const std::filesystem::path& file);


  class error: public std::runtime_error {
  public:
    error(const std::string& msg): std::runtime_error(msg) {}  
  };

  class fileNotFound: public error {
  public:
    fileNotFound(const std::filesystem::path& file ): error ("File is missing"s + file.string()) {}
  };

  class canNotOpen: public error {
  public:
    canNotOpen(const std::filesystem::path& file, bool write): error ("Can't open "s + file.string() + " for "s + ((write) ? " writing" : " reading")) {}
  };

} // namespace fileAccess
} // namespace helpers
