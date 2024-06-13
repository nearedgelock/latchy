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
#include "containerInfo.h"
#include "metaInfo.h"
#include "machineInfo.h"
#include "processInfo.h"

#include <botan/hash.h>
#include <botan/hex.h>

#include <iostream>

namespace meta {

composition::composition() {
  sources.emplace_back(std::make_unique<meta::machine::info>());
  sources.emplace_back(std::make_unique<meta::machine::hostname>());
  sources.emplace_back(std::make_unique<meta::process::info>());
  sources.emplace_back(std::make_unique<meta::container::info>());

  // Gather the persistent data
  for (const auto& item : sources) {
    if (item->isPersistent()) {
      persistentData << ((persistentData.str().empty()) ? "" : itemSeparator) << item->rawData();
    }
  }

  // Gather the semi-persistent data
  for (const auto& item : sources) {
    if (item->isSemiPersistent()) {
      semiPersistentData << ((semiPersistentData.str().empty()) ? "" :itemSeparator) << item->rawData();
    }
  }

  // Gather the semi-volatile data
  for (const auto& item : sources) {
    if (item->isSemiVolatile()) {
      semiVolatileData << ((semiVolatileData.str().empty()) ? "" : itemSeparator) << item->rawData();
    }
  }

  // Gather the volatile data
  for (const auto& item : sources) {
    if (item->isVolatile()) {
      volatileData << ((volatileData.str().empty()) ? "" : itemSeparator) << item->rawData();
    }
  }
}

const std::string composition::getComposedHash() const {
  std::stringstream                       retval;

  retval << ((retval.str().empty()) ? "" : itemSeparator) << getPersistentHash();
  retval << ((retval.str().empty()) ? "" : itemSeparator) << getSemiPersistentHash();
  retval << ((retval.str().empty()) ? "" : itemSeparator) << getSemiVolatileHash();
  retval << ((retval.str().empty()) ? "" : itemSeparator) << getVolatileHash();

  return retval.str();
}

void composition::printInfo() const {
  for (const auto& source : sources) {
    source->printInfo();
  }
}

const std::string composition::getHash(const std::stringstream& data) const {
  std::unique_ptr<Botan::HashFunction>    hash = Botan::HashFunction::create_or_throw("SHA-512");
  hash->update(data.str());

  return Botan::hex_encode(hash->final());
}

} // namespace meta