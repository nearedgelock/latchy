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
#include <memory>

#include "curl.h"
#include "assetProvider.h"
#include "assetSource.h"

namespace assets {

  /// List of assets - Both a source (ingress) and an output (provider to others, ie egress).
  ///
  /// An asset consists in having a source (ingress) and a provider (egress to a client). The source may
  /// of may not include processing (such as unlocking the asset). The provider provides to the (external)
  /// client the asset and then signal the source to destroy it.
  ///
  /// The list is built using a list of declarations.

  class list {
  public:
    using secretCfg_t =         configuration::secretCfg_t;
    using secretCfgList_t =     configuration::secretCfgList_t;

    using assetSource_t =       assetserver::assetSource<std::string>;
    using assetSource_p =       std::shared_ptr<assetSource_t>;
   
    using asset_p =             std::unique_ptr<assetserver::assetProviderBase<std::string>>;
    using assetList =           std::set<asset_p>;

  public:
    list() { };
    list(const secretCfgList_t& list, bool compatibleMode = false, bool dump = false);
    virtual ~list() { stopAll(); curlWrapper::globalCleanUp(); };   // curlWrapper::globalCleanUp should only be called once but there is only 1 list object to destroy anyway

    void                        processConfiguration(const secretCfgList_t& list, bool compatibleMode, bool dump);
    void                        startAll();
    void                        stopAll();
  protected:
    assetList                   assets;
    meta::composition           metaData;

    virtual assetSource_p       createSource(const secretCfg_t&, bool autostart, bool compatibleMode);
    virtual asset_p             createProvider(const secretCfg_t&, assetSource_p src);

  public:
    class invalid: public std::runtime_error {
    public:
      invalid(const std::string& msg = ""): runtime_error("Configuration is invalid " + ((msg.empty() == false) ? (" - " + msg) : "")) { };
    };
    class unimplemented: public std::runtime_error {
    public:
      unimplemented(const std::string& msg = ""): runtime_error("This mode is unimplemented " + ((msg.empty() == false) ? (" - " + msg) : "")) { };
    };
    class missingParameter: public std::runtime_error {
    public:
      missingParameter(const std::string& msg = ""): runtime_error("Missing an argument " + ((msg.empty() == false) ? (" - " + msg) : "")) { };
    };

  };

} // namespace assets


