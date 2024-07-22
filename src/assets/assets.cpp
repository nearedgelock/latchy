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
#include "assets.h"

#include <chrono>
namespace assets {
  using namespace std::chrono_literals;

  list::list(const secretCfgList_t& list, bool dump) {
    curlWrapper::globalInit();    // Tis is rquired once and it is Ok to call multiple time as a flag prevent redoing it.

    try {
      // We may have more than 1 item to process. Anyone of them may fail and trhow an exception.
      // In such case we will stop all of the successfully created assets and return.
      INFO() << "There are " << list.secrets().size() << " elements in the configuration. We expected to have that many assets" << std::endl;
      processConfiguration(list, dump);
      DEBUG() << "Processing the configuration is complete, we have " << assets.size() << " assets" << std::endl;
      if (dump == false) {
        startAll();
      }

      if ( (dump == false) and (assets.size() != list.secrets().size()) ) {
        DEBUG() << "Something is wrong, we only have " << assets.size() << " assets" << std::endl;
        DEBUG() << configuration::convertMsgToJsonString(list);
        stopAll();
        throw invalid("Inconsistent configured / running asset number");
      }
    }
    catch (std::exception& exc) {
      DEBUG() << "While building the asset list we got an exception, which we will pass up" << std::endl;
      stopAll();
      throw;
    }

    DEBUG() << "Done building the assets, we have " << assets.size() << std::endl;
  }

  void list::processConfiguration(const secretCfgList_t& list, bool dump) {
    // Walk the declaration list and set the assets. On failure we throw an exception.
    for (const auto asset : list.secrets()) {
      //
      // Asset ingress, ie source
      //
      DEBUG() << "Creating an asset source" << std::endl;
      assetSource_p        source = createSource(asset, !dump);
      if (dump == true) {
        // We are simply asked to dump the JWE content and not actually perform the whole decryption operation
        source->dumpInfo();
      } else {
        //
        // Asset egress - ie output - But only if we are not asked to dump the source info (such as JWE content)
        //
        DEBUG() << "Creating a provider (to an external client) to deliver the asset" << std::endl;
        asset_p            provider = createProvider(asset, source);

        // Keep the provider. The source object is own by the provider.
        assets.insert(std::move(provider));
      }
    }
  }

  void list::startAll() {
    DEBUG() << "We are about to start assets in the asset list. We have " << assets.size() << " asset definition in the list" << std::endl;
    try {
      // Start each asset
      for (auto& asset : assets) {
        asset->start();
      }
    } catch (std::exception& exc) {
      std::cout << "Abnormal exception when starting the assets - " <<  exc.what() << std::endl;
    }
  }

  void list::stopAll() {
    try {
      // Loop on all assets and check if they complete. If so, destroy then (i.e remove them from the list)
      DEBUG() << "We are about to stop assets in the asset list. We have " << assets.size() << " asset definition in the list" << std::endl;
      while (assets.size() != 0) {
        for (assetList::iterator asset = assets.begin(); asset != assets.end();) {
          if (*asset != nullptr) {
            if ((*asset)->wait(100ms) == std::future_status::ready) {
              (*asset)->get();    // So that exceptions are handled here.
              asset = assets.erase(asset);
            } else {
              ++asset;
            }
          }
        }
      }

      // The list is now empty

    } catch (std::exception& exc) {
      USERMSG() << "Abnormal exception in one of the asset object - " <<  exc.what() << std::endl;
    }
  }

  list::assetSource_p list::createSource(const secretCfg_t& cfg, bool autostart) {
    //
    // Asset ingress, ie source
    //

    assetSource_p        source = nullptr;

    if ( (cfg.lockingmethod() == model::latchy::secretLockingMethods::UNKNOWNLOCKING) or (cfg.lockingmethod() == model::latchy::secretLockingMethods::CLEVIS) ) {
      if (cfg.in().empty() == false) {
        // We assume that the input method is a file or a named pipe (the processing is the same)
        DEBUG() << "JWE source is file or named pipe" << std::endl;
        source = std::make_shared<assetserver::assetFileClevis>(cfg.in(), metaData, autostart);
      } else if ( (cfg.imethod() == model::latchy::secretIngestionMethods::STDIN) or (cfg.imethod() == model::latchy::secretIngestionMethods::UNKNOWNINGESTION)) {
        // Assume STDIN
        DEBUG() << "JWE source is STDIN" << std::endl;
        source = std::make_shared<assetserver::assetFileClevis>("", metaData, autostart);
      } else  if (cfg.imethod() == model::latchy::secretIngestionMethods::IENVVAR) {
        // Env var - Future
        throw unimplemented("Input asset from environment");
      } else {
        // Error
        throw invalid("asset input method");
      }
    } else {
      // Error
      throw invalid("asset unlocking method");
    }

    return source;
  }

  list::asset_p list::createProvider(const secretCfg_t& cfg, assetSource_p source) {
    //
    // Asset egress - ie output
    //
    asset_p     provider = nullptr;
    if ( (cfg.emethod() == model::latchy::secretEgressMethods::FILE) or (cfg.emethod() == model::latchy::secretEgressMethods::UNKNOWNEGRESS) ) {
      // File output method
      std::size_t     readCount = cfg.outcount();
      if (cfg.out().empty() == true) {
        throw missingParameter("output filename");
      }
      if (readCount == 0) {
        readCount = 1;
      }
      provider = std::make_unique<assetserver::assetProvider>(source, cfg.out(), readCount, false);

    } else if (cfg.emethod() == model::latchy::secretEgressMethods::PIPE) {
      // Pipe output method
      if (cfg.out().empty() == true) {
        throw missingParameter("output pipename");
      }
      provider = std::make_unique<assetserver::assetProvider>(source, cfg.out(), 0, true);

    } else if (cfg.emethod() == model::latchy::secretEgressMethods::STDOUT) {
      // STDOUT method
      provider = std::make_unique<assetserver::assetProviderStdout>(source);
    } else {
      // Error
      throw invalid("asset egress method");
    }

    return provider;
  }
} // namespace assets
