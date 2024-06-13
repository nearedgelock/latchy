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

#include <deque>
#include <thread>

#include "helpers/log.h"

#include "clevisEncrypt.h"
#include "jose/joseCommon.h"
#include "jose/joseClevisDecrypt.h"

namespace assetserver {
  using namespace std::chrono_literals;

  //
  // This is the an asset source that extract the secret (asset) from a clevis formatted JWE.
  //
  // We an external tool, JOSE, to do the actually processing (similar to what the clevis
  // tang pins is doing.
  //

  assetFileClevis::assetFileClevis(const std::string& f, const meta::composition& m, bool autoStart): assetFile(f), meta(m) {
    // The base class already makes sure that the input JWE file is there and readable.
    // All is left is to
    // - perform the base processing, which includes validation of the JWE
    // - extract the secret from the JWE. We do this via an async activity

    // Basic validation (and decomposition of the JWE). 
    baseJWEProcessing();

    // Actual secret extration. If requested....
    if (autoStart == true) {
      startUnsealing();
    }
  }

  void assetFileClevis::startUnsealing() {
    jweExtractTask = std::async([&]() { jweExtract(); });
  }

  bool assetFileClevis::isReady() const {
    if (isDone == true) {
      return true;
    }

    if ( (jweExtractTask.valid()) and (jweExtractTask.wait_for(0s) == std::future_status::ready) ) {
      jweExtractTask.get();   // We do this so that exceptions can percolate up
      isDone = true;
      return true;
    }

    return false;
  }

  void assetFileClevis::baseJWEProcessing() {
    // First, lets get the JWE
    const std::string&      jwe = assetFile::getAsset();
    jwe_j = joseLibWrapper::decrypt::decomposeCompactJWE(jwe);

    // Check the validity of the JWE and extract references to the various
    // components we need from the protected header
      
    INFO() << "Check validity of input JWE " << filePath.string() << std::endl;
    joseLibWrapper::decrypt::checkJWE   checker(jwe_j);
    jweProtectedHeaders_j = checker.getHeader();

    // ATM we assume a tang pin

    epk_j = checker.getEpk();
    epkCurve_j = checker.getEpkCurve();
    kid_j = checker.getKid();
    allKeys_j = checker.getKeys();
    activeServerKey_j = checker.getActiveKey();

    extractedUrl = checker.getUrl();

    checker.printProtectedHeader();
    checker.printEPK();
    checker.printSelectedServerKey();
  }

  void assetFileClevis::jweExtract() {
    try {
      // Extract the secret. First by recovering the encryption key, using tang. And then decryting the payload 
      INFO() << "Recover private key" << (filePath.string().empty() ? "" : " for " + filePath.string()) << std::endl;
      recoverPrivateKey();

      INFO() << "Finally, recover the payload / secret" <<   (filePath.string().empty() ? "" : " from " + filePath.string()) << std::endl;
      buffer = joseLibWrapper::decrypt::recoverPayload(unwrappingJWK_j, jwe_j);
      jwk.assign(jwk.size(), (char) 0);

      DEBUG() << "Recovered clear-text secret" << std::endl;
    } catch (std::exception& exc) {
      // Just rethrow after alerting the user
      failedPrint();
      throw;
    }
  }

  void assetFileClevis::freeJson() {
    if (jwe_j != nullptr) { json_decref(jwe_j); }
    if (jweProtectedHeaders_j != nullptr) { json_decref(jweProtectedHeaders_j); }
    if (unwrappingJWK_j != nullptr) { json_decref(unwrappingJWK_j); }
  }

  void assetFileClevis::recoverPrivateKey() {
    // Recover the private key used to encrypt the payload and produced the original JWE. This
    // is done via an interaction with the Tang server.
    json_auto_t*                  ephemeralKey_j = nullptr;
    std::string                   ephemeralKey1_pub;

    try {
    // This series of action may throw an exception
    ephemeralKey_j = joseLibWrapper::generateKey(epkCurve_j);   // This is a full pairwise key, i.e. the private part is present
    DEBUG() << "Ephemeral Key, before exchange1: " << joseLibWrapper::prettyPrintJson(ephemeralKey_j) << std::endl;

    json_auto_t*                  ex_j = joseLibWrapper::keyExchange(epk_j, ephemeralKey_j);    // Perform a key extract between the EPK and the new ephemeral key
    ephemeralKey1_pub = joseLibWrapper::prettyPrintJson(ex_j);
    DEBUG() << "Ephemeral Key, after exchange1: " << ephemeralKey1_pub << std::endl;
 
    std::string                   recoveredKeyFromTang;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>   giveUpTime(std::chrono::steady_clock::now() + 5h);

    while (!isCancelled) {
      try {
        recoveredKeyFromTang = curlWrapper::keyRecoverViaTang(extractedUrl, json_string_value(kid_j), ephemeralKey1_pub, queryString(), isCancelled);
        ephemeralKey1_pub.assign(ephemeralKey1_pub.size(), (char) 0);  // Clear the memory
        break;
      } catch (joseLibWrapper::decrypt::permanentTangFailure& exc) {
        // Permanent error, just rethrow to higher up
        throw;
      } catch (std::exception& exc) {
        // Other errors are temporary. Authorization may come later so we retry in a little while, unless
        // we need to give up!!
        if (std::chrono::steady_clock::now() > giveUpTime) {
          throw  unavailable("Waited too long for Tang access, we gave up");
        }
        std::this_thread::sleep_for (requestInterval);
      }
    }

    json_auto_t*                  exchangedKey2 = nullptr;
    json_auto_t*                  recoveredKey = nullptr;

    recoveredKey = joseLibWrapper::extractB64ToJson(recoveredKeyFromTang, true);

    DEBUG() << "Recovering key from server: " << joseLibWrapper::prettyPrintJson(recoveredKey) << std::endl;

    exchangedKey2 = joseLibWrapper::keyExchange(ephemeralKey_j, activeServerKey_j);
    DEBUG() << "Ephemeral Key, after exchange2: " << joseLibWrapper::prettyPrintJson(exchangedKey2) << std::endl;

    joseLibWrapper::removePrivate(recoveredKey);
    unwrappingJWK_j = joseLibWrapper::keyExchange(recoveredKey, exchangedKey2, true);
    //Probably not a good idea to show this, even for debug DEBUG() << "Unwrapping JWK: " << joseLibWrapper::prettyPrintJson(unwrappingJWK_j) << std::endl;

    // Just make sure things get properly destroyed, even if they are on the stack
    recoveredKeyFromTang.assign(recoveredKeyFromTang.size(), (char) 0);
    //exchangedKey2.assign(exchangedKey2.size(), (char) 0);   // Clear the memory
    //ephemeralKey.assign(ephemeralKey.size(), (char) 0);     // Clear the memory
    } catch (std::exception& exc) {
      throw;
    }
  }

  const std::string assetFileClevis::queryString() {
    std::string       qs;   // Does not include the '?' opening character

    std::string       idValue = meta.getComposedHash();
    if (idValue.empty() == false) {
      qs += "id=" + idValue;
    }

    return qs;
  }

  void assetFileClevis::failedPrint() const {
    USERMSG() << "Failed to extract secret from " << (useCin ? " stdin " : filePath.string()) << " using server at " << extractedUrl << std::endl;
  }

} // namespace assetserver


