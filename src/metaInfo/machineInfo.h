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
namespace meta {
namespace machine {

  class info: public meta::info {
  public:
    info();
    
    virtual bool                  isPersistent() { return true; };

    virtual const std::string     rawData() const { return data; };
    void                          printInfo() const {};
  private:
    std::string                   data;
  };


  // The hostname is set by the user; we rely on him to know what he is doing
  // For docker, the hostname is the container ID, which is new each time a container is recreated
  class hostname: public meta::info {
  public:
    hostname();
    
    virtual bool                  isSemiPersistent() { return true; };

    virtual const std::string     rawData() const { return data; };
    void                          printInfo() const {};
  private:
    std::string                   data;
  };

} // namespace machine
} // namespace meta

