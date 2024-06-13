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
namespace meta {
namespace container {

  /// @brief Collection of information about the container environment
  ///
  /// From within the container we are collecting signifcant information.
  /// In most cases, the means for collecting the information are not
  /// directly docker or kubernetes made since there are no provision
  /// for that in either framework. So, most of the means are generic
  /// and could be applicable for non container use cases. 

  class info: public meta::info {
  public:
    info();
    
    virtual bool                  isSemiVolatile() { return true; };

    virtual const std::string     rawData() const { return data.str(); };
    void                          printInfo() const {};
  private:
    std::stringstream             data;

    void                          populateGeneric();      // Things such as the hostname
    void                          populateDocker();       // In fact, this is mostly about namespaces and/or cgroups
    void                          populateKubernetes();   // Really specific to kubernetes, such as looking into /var/run/secrets/kubernetes.io/serviceaccount
  };

} // namespace container
} // namespace meta
