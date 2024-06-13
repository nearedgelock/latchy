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
#include "fileAccess.h"

#include <fstream>

namespace helpers {
namespace fileAccess {

void writeTo(const std::filesystem::path& file, const std::string& data, bool append) {
  try {
    std::ofstream     filehandle(file.string(), std::ios::binary | std::ios::out | (append ? std::ios::app : (std::ios_base::openmode) 0));

    if (filehandle.is_open() == false) {
      throw canNotOpen(file, false);
    }

    filehandle << data;
  } catch (std::exception& exc) {
    throw;
  } 
}
 
const std::string readAll(const std::filesystem::path& file) {
  std::string         retval;

  if (std::filesystem::exists(file) == true) {
    std::ifstream     filehandle(file.string(), std::ios::binary | std::ios::in);

    if (filehandle.is_open() == false) {
      throw canNotOpen(file, false);
    }

    try {
      filehandle.exceptions(std::ios::failbit | std::ios::badbit);
      while (true) {
        std::string   readval;
        std::getline(filehandle, readval);
        retval += readval;
      }
      filehandle.close();
      return retval;  
    } catch (std::exception& exc) {
      if (filehandle.eof() == true) {
        // Normal case of reading up to the end. Just return
        filehandle.close();
        return retval;
      }
      throw;
    }
    
  } else {
    throw fileNotFound(file);
  }
}

const std::string getSymlink(const std::filesystem::path& file) {
  try {
    if (std::filesystem::exists(file) == false) {
      throw fileNotFound(file);
    }

    std::filesystem::path   target = std::filesystem::read_symlink(file);
    if (target.empty() == false) {
      return target.string();
    }

    throw canNotOpen(file, false);
  } catch (const std::exception& exc) {
    throw canNotOpen(file, false);
  }
}

} // namespace fileAccess
} // namespace helpers