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
#include <future>
#include <chrono>

#include <iostream>
#include <fstream>

#include <jansson.h>

#include "configuration.h"
#include "helpers/log.h"
#include "metaInfo/metaInfo.h"

#include "curl.h"
#include "joseCommon.h"
#include "joseClevisDecrypt.h"

namespace assetserver {
  using namespace std::chrono_literals;

  /// Asset source
  ///

  /// Pure (almost) abstract asset source interface
  template <typename T>
  class assetSource {
  public:
    virtual ~assetSource() {};

    virtual void                    cancel() { isCancelled = true; }

    virtual bool                    isReady() const =0;        /// The underlying asset is available.
    virtual const T&                getAsset() =0;             /// Get the underlying asset.
    virtual void                    destroy() =0;              /// Delete or otherwise destroy the underlying asset. An example of destruction is to write 0 to memory space occupied by a secret.

    virtual void                    dumpInfo(bool all = false) const =0;       /// Output significant data, mostly upon user request, such as dumping the JWE content
    virtual void                    printInfo() const =0;      /// Output operational information, typically for debugging purpose 
  protected:
    std::atomic<bool>               isCancelled = false;
    bool                            destroyed = false;
  public:
    class error: public std::runtime_error {
    public:
      error(const std::string& msg = ""): runtime_error("Source error" + ((msg.empty() == false) ? (" - " + msg) : "")) { };
    protected:
    };

    class unavailable: public error {
    public:
      unavailable(const std::string& msg = ""): error("Source data is unavailable" + ((msg.empty() == false) ? (" - " + msg) : "")) { };
    };
  };

  //
  // Some derived asset source classes
  //

  // A simple string based source. Mostly used for testing
  class assetStaticString: public assetSource<std::string> {
  public:
    assetStaticString(const std::string& d): buffer(d) {};
    virtual ~assetStaticString() {};

    virtual bool                    isReady() const { return !destroyed; };
    virtual const std::string&      getAsset() { return buffer; };
    virtual void                    destroy() { buffer.assign(buffer.size(), (char) 0); destroyed = true; };

  private:
    mutable std::string             buffer;
  };

  // The source is a file. Or STDIN (when no filename path is provided)
  // We use a synchronous method. In other words, we expect the data to be immediately readable from the filesystem
  class assetFile: public assetSource<std::string> {
  public:
    assetFile(const std::string& f);
    virtual ~assetFile() {};

    virtual bool                    isReady() const { return !destroyed; };   // The object is constructed, which implies that the file exists and is readable (can be open)
    virtual const std::string&      getAsset();
    virtual void                    destroy();

    virtual void                    dumpInfo(bool all = false) const { };
    virtual void                    printInfo() const { };
  protected:
    std::filesystem::path           filePath;
    std::ifstream                   inputStream;
    bool                            useCin = false;

  protected:
    mutable std::string             buffer;
  };

  // A source using clevis/tang metod to recover the passphrase and then unseal the secret
  //
  // The base class, assetFile provides the JWE. An async thread will extract the secret from it.
  //
  class assetFileClevis: public assetFile {
  public:
    assetFileClevis(const std::string& f, const meta::composition& m, bool autoStart, bool compatibleMode);
    virtual ~assetFileClevis() { cancel(); freeJson(); };

    void                            startUnsealing();

    virtual bool                    isReady() const;   // The asset is only available after the unlocking step is complete (using Tang)
    virtual const std::string&      getAsset() { return buffer; };

    void                            dumpInfo(bool all = false) const { if (all) { printJWE(true); }printProtectedHeader(true); };
    void                            printInfo() const { printEPK(); printEPKCurve(); printKID(); printAllKeys(); printSelectedKey(); printUnwrappingJWK(); printSelectedServerKey(); printProtectedHeader(); };
  protected:
    bool                            compatibleMode = false;
    const meta::composition&        meta;
    mutable std::future<void>       jweExtractTask;    // We will be using an std::async to extract from the JWE...
    mutable std::atomic<bool>       isDone = false;

    void                            baseJWEProcessing();
    void                            jweExtract();      // Actual secret extraction. This typically runs in its own thread (see startUnsealing())
    void                            recoverPrivateKey();

    // JSON object pointer, from the jansson C library.
    // New references, which must be freed
    void                            freeJson();
    mutable json_t*                 jwe_j = nullptr;
    mutable json_t*                 jweProtectedHeaders_j = nullptr;
    mutable json_t*                 unwrappingJWK_j = nullptr;

    // Borrowed references, which must NOT be freed
    mutable json_t*                 epk_j = nullptr;
    mutable json_t*                 epkCurve_j = nullptr;
    mutable json_t*                 kid_j = nullptr;
    mutable json_t*                 allKeys_j = nullptr;
    mutable json_t*                 activeServerKey_j = nullptr;
  
    virtual const std::string       queryString();

    // JSON string, used with the external JOSE binary / binary
    mutable std::string             epk;
    mutable std::string             epkCurve;
    mutable std::string             kid;
    mutable std::string             allKeys;
    mutable std::string             activeServerKey;
    mutable std::string             extractedUrl;

    mutable std::string             jwk;

    const std::chrono::seconds      requestInterval = 15s;

    void                            printEPK() const { DEBUG() << "EPK: " << joseLibWrapper::prettyPrintJson(epk_j) << std::endl; };
    void                            printEPKCurve() const { DEBUG() << "EPK Curve: " << joseLibWrapper::prettyPrintJson(epkCurve_j) << std::endl; };
    void                            printKID() const { DEBUG() << "KID: " << joseLibWrapper::prettyPrintJson(kid_j) << std::endl; };
    void                            printAllKeys() const { DEBUG() << "Keys: " << joseLibWrapper::prettyPrintJson(allKeys_j) << std::endl; };
    void                            printSelectedKey() const { DEBUG() << "Selected key: " << joseLibWrapper::prettyPrintJson(activeServerKey_j) << std::endl; };
    void                            printUnwrappingJWK() const { DEBUG() << "Unwrapping JWK: " << joseLibWrapper::prettyPrintJson(unwrappingJWK_j) << std::endl; };
    void                            printSelectedServerKey() const { DEBUG() << "Active server key: " << joseLibWrapper::prettyPrintJson(activeServerKey_j) << std::endl; };
    void                            printProtectedHeader(bool force = false) const {
      if (force == true)
        USERMSG() << "Protected header: \n" << joseLibWrapper::prettyPrintJson(jweProtectedHeaders_j) << std::endl;
      else
        DEBUG() << "Protected header: \n" << joseLibWrapper::prettyPrintJson(jweProtectedHeaders_j) << std::endl;
    };

    void                            printJWE(bool force = false) const {
      if (force == true)
        USERMSG() << "JWE: \n" << joseLibWrapper::prettyPrintJson(jwe_j) << std::endl;
      else
        DEBUG() << "JWE: \n" << joseLibWrapper::prettyPrintJson(jwe_j) << std::endl;
    };
    void                            failedPrint() const;
  };

} // namespace assetserver

