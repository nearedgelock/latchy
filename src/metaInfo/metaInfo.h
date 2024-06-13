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

///
/// The meta namespace goal is to provide information to the
/// an external agent (server) thaty can be used to distinguish
/// instances of:
///   - latchy process
///   - the "machine" where is it running
///   - container instance
///   - cluster instance
///   - Etc.
///
/// The goal is NOT to identify the instance ID but to distinguish
/// between them so most of the data is obfuscated using crypto
/// strong hash.
///
/// Conceptually, the source of data may be:
///   - Persistent (eg machine ID)
///   - semi-persistent (eg MAC addresses, hostname)
///   - somewhat volatile (eg container ID or equivalent, or hostname inside a container)
///   - fully volatile (eg process PID)
///
/// Depending on the context, a data type that is normally considered
/// persistent may be considered semi-persistent instead. An example is
/// the machine hosting containers. In a cluster, the workloads may
/// move from one host to another but still relate to the same
/// workload where the machine ID varies.
///

#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include "helpers/log.h"
namespace meta {

  ///
  /// info - This is for the most part a pure abstract base for info source derived classes
  ///
  class info {
  public:
    // Type, category
    virtual bool                  isPersistent() { return false; };
    virtual bool                  isSemiPersistent() { return false; };
    virtual bool                  isSemiVolatile() { return false; };
    virtual bool                  isVolatile() { return false; };    

    // Data and operation
    virtual const std::string     rawData() const =0;   // This is raw data not modfied in any way (so not hased). It is assumed to be static within the life of the process. 

    virtual void                  printInfo() const =0;
  protected:
    const std::string             itemSeparator = "::";
  };

  ///
  /// Composition - Aggregate multiple source of data.
  ///
  class composition {
  public:
    using info_p =                std::unique_ptr<info>;
    using sourceList_t =          std::vector<info_p>;
  public:
    composition();

    const std::string             getComposedHash() const;

    const std::string             getPersistentHash() const { return getHash(persistentData); };
    const std::string             getSemiPersistentHash() const { return getHash(semiPersistentData); };
    const std::string             getSemiVolatileHash() const { return getHash(semiVolatileData); };
    const std::string             getVolatileHash() const { return getHash(volatileData); };

    const std::string             persistentDigest() const { return persistentData.str(); };
    const std::string             semiPersistentDigest() const { return semiPersistentData.str(); };
    const std::string             semiVolatileDigest() const { return semiVolatileData.str(); };
    const std::string             volatileDigest() const { return volatileData.str(); };

    void                          printInfo() const;
  private:
    sourceList_t                  sources;
    const std::string             itemSeparator = "~~";


    std::stringstream             persistentData;
    std::stringstream             semiPersistentData;    
    std::stringstream             semiVolatileData;
    std::stringstream             volatileData;

    std::stringstream             data;

    const std::string             getHash(const std::stringstream&) const;

  };

} // namespace meta


