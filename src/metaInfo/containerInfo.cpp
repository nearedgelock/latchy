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

#include "fileAccess.h"

namespace meta {
namespace container {

  info::info() {
    populateGeneric();
    populateDocker();
    populateKubernetes();
  }

  void info::populateGeneric() {
    try {
      const std::string               id = helpers::fileAccess::readAll("/etc/hostname");
      data << id;
    } catch (...) {}
  }

  void info::populateDocker() {
    std::string                       cgroup_ns;

    try {
      cgroup_ns = helpers::fileAccess::getSymlink("/proc/self/ns/cgroup");
    } catch (...) {}

    if (cgroup_ns.empty() == false) {
      data << ((data.str().empty()) ? "" : itemSeparator) << cgroup_ns;
    } else {
      // Lets try the non cgroup namespace mode, ie when info under /proc/self/cgroup is present
      try {
        const std::string             cgroup_info = helpers::fileAccess::readAll("/proc/self/cgroup");
        data << ((data.str().empty()) ? "" : itemSeparator) << cgroup_info;
      } catch (...) {}
    }
  }

  void info::populateKubernetes() {
    //
    // /var/run/secrets/kubernetes.io/serviceaccount contains file
    // thay may be present and readable (permission), some of which is potentially
    // unique to the container / pod. But some other are specific to the cluster. Nonetheless
    // this is useful.
    try {
      const std::string               caCrt = helpers::fileAccess::getSymlink("/var/run/secrets/kubernetes.io/serviceaccount/ca.crt");
      data << ((data.str().empty()) ? "" : itemSeparator) << caCrt;
    } catch (...) {}

    try {
      const std::string               namespaceID = helpers::fileAccess::getSymlink("/var/run/secrets/kubernetes.io/serviceaccount/namespace");
      data << ((data.str().empty()) ? "" : itemSeparator) << namespaceID;
    } catch (...) {}

    try {
      const std::string               token = helpers::fileAccess::getSymlink("/var/run/secrets/kubernetes.io/serviceaccount/token");
      data << ((data.str().empty()) ? "" : itemSeparator) << token;
    } catch (...) {}
  }
  

} // namespace container
} // namespace meta
