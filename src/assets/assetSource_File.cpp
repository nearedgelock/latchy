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
#include "assetSource.h"

namespace assetserver {

  assetFile::assetFile(const std::string& f): filePath(f) {
    // Verify that the file exists and that we can open it. We are reading from it!
    if (filePath.empty() == false) {
      useCin = false;

      if ( (std::filesystem::exists(filePath) == true) and ( (std::filesystem::is_regular_file(filePath) == true) or (std::filesystem::is_fifo(filePath) == true) ) ) {
        inputStream.open(filePath.string(), std::ios::binary | std::ios::in);
        if (inputStream.is_open() == false) {
          throw  unavailable(filePath.string() + " can't be open, check permissions");
        }
      } else {
        throw  unavailable(filePath.string() + " - FIle is missing or incorrect type");
      }
    } else {
      // stdin case
      useCin = true;
    }
  }

  const std::string& assetFile::getAsset() {

    // Read the file, but only once (until destroyed)
    std::stringstream               rdbuffer;

    if (buffer.empty() == true) {
      if (useCin == true) {
        std::cin.exceptions(std::ios::failbit | std::ios::badbit);      // No exception on eof
        std::cin >> rdbuffer.rdbuf();     // Until eof
      } else {
        inputStream.exceptions(std::ios::failbit | std::ios::badbit);   // No exception on eof

        inputStream >> rdbuffer.rdbuf();  // Until eof
      }
      buffer = std::move(rdbuffer.str());

      // Clear the memory content.
      rdbuffer.str("");   // Reset buffer to start
      for (std::size_t i = 0; i < buffer.size(); ++i) {
        rdbuffer << 0;    // Overwrite
      }
    }

    // There may or may not be trailing '\n', remote them!!
    while (buffer.empty() == false) {
      if (buffer.back() == '\n') {
        buffer.pop_back();
      } else {
        break;
      }
    }
    return buffer;
  }

  void assetFile::destroy() {
    buffer.assign(buffer.size(), (char) 0);
    destroyed = true;
  }

} // namespace assetserver
