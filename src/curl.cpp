
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
#include <cstring>
#include "curl.h"
#include <curl/curl.h>      // From libcurl

#include "helpers/forkExec.h"
#include "helpers/stringSplit.h"
#include "helpers/log.h"
#include "helpers/fileAccess.h"

namespace curlWrapper {
  bool        isGlobalInit = false;

  void globalInit() {
    if (isGlobalInit == false ) {
      curl_global_init(CURL_GLOBAL_ALL);
      isGlobalInit = true;
    }
  }

  void globalCleanUp() {
    curl_global_cleanup();
  }
  
  size_t write_callback(char *ptr, size_t size, size_t nmemb, void* userdata) {
    try {
    if (userdata != nullptr) {
      std::stringstream*  recoveredKey = static_cast<std::stringstream*>(userdata);
      recoveredKey->write(ptr, nmemb);
    }
    } catch (std::exception& exc) {
      DEBUG() << "Erroro in our libcurl wrote_callback - " << exc.what() << std::endl;
    }

    return nmemb;
  }

  const char* find_ca_bundle() {
    const char* ca_paths[] = {
        "/etc/pki/tls/certs/ca-bundle.crt",     // RHEL/CentOS/Rocky/Fedora
        "/etc/ssl/certs/ca-certificates.crt",   // Debian/Ubuntu
        "/etc/ssl/ca-bundle.pem",               // openSUSE
        "/etc/ssl/cert.pem",                    // Alpine Linux
        "/usr/local/share/certs/ca-root-nss.crt", // FreeBSD
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", // Modern RHEL
        nullptr
    };
    
    for (int i = 0; ca_paths[i]; i++) {
        if (access(ca_paths[i], R_OK) == 0) {
            return ca_paths[i];
        }
    }
    return nullptr;
}

  std::string keyRecoverViaTang(const std::string& url, const std::string& kid, const std::string& key, const std::string& queryString, const std::atomic_bool& cancelled ) {
    globalInit();

    CURL*                 curlSession = curl_easy_init();
    std::string           completeUrl(url +"/rec/" + kid + (queryString.empty() ? "" : std::string("?") + queryString));
    struct curl_slist*    headerList = nullptr;
    CURLcode              result = CURLcode::CURLE_OK;

    std::stringstream     ss;

    DEBUG() << completeUrl << std::endl;

    if (curlSession != nullptr) {
      const char*         ca_bundle = find_ca_bundle();
      if (ca_bundle) {
        curl_easy_setopt(curlSession, CURLOPT_CAINFO, ca_bundle);
      } else {
        throw permanentTangFailure(url + " - " + "No CA certificates, this is non recoverable");
     }

      // We are not checking error codes since we essentially stuff libcurl with static data (constants) or
      // data we just created. The only exception is kid, which we presume is Ok. Anyway, it does not appear
      // that curl will fail on a too long kid

      // Global behavior options
      curl_easy_setopt(curlSession, CURLOPT_HEADER, 1);       // The response body will include the header. See the doc was a discussion about transfer size
      curl_easy_setopt(curlSession, CURLOPT_NOPROGRESS, 1);
      curl_easy_setopt(curlSession, CURLOPT_NOSIGNAL, 0);     // We allow CURL to use signals. We may need to adjust this later (but see the doc regarding DNS)

      // Callback related
      curl_easy_setopt(curlSession, CURLOPT_WRITEFUNCTION, write_callback);
      curl_easy_setopt(curlSession, CURLOPT_WRITEDATA, (void *)&ss);

      curl_easy_setopt(curlSession, CURLOPT_URL, (char *)completeUrl.c_str());

      // POST data (and required Header)
      headerList = curl_slist_append(headerList, "Content-Type: application/jwk+json");
      curl_easy_setopt(curlSession, CURLOPT_HTTPHEADER, headerList);
      curl_easy_setopt(curlSession, CURLOPT_POST, 1);
      curl_easy_setopt(curlSession, CURLOPT_POSTFIELDS, (char*)key.data());     // This the data, i.e. the payload
      curl_easy_setopt(curlSession, CURLOPT_POSTFIELDSIZE, (long)key.size());

      char                error_buffer[CURL_ERROR_SIZE];
      curl_easy_setopt(curlSession, CURLOPT_ERRORBUFFER, error_buffer);

      result = curl_easy_perform(curlSession);

      curl_easy_cleanup(curlSession);

      if (result != CURLcode::CURLE_OK) {
        INFO() << "Curl reported an error - " << curl_easy_strerror(result) << std::endl;
        if (strlen(error_buffer) != 0) {
          DEBUG() << "Curl detailed error - " << error_buffer << std::endl;
        }
        throw failedTangInteraction(url);
      } else {

        // We are receiving both the headers and content because we want to extract the HTTP response. The
        // alternative could have been to use curl option --fail but the man page says it is not
        // fail safe.... :(
        std::string         responseLine;
        std::getline(ss, responseLine);

        std::string         line;
        std::streampos      contentPostion;
        while (std::getline(ss, line)) {
          // All we really want to do is to skip the headers and get to the content.
          // So we continue until we find the line that starts with Content-Lenth
          // Lets make sure we are case insensitive.
          std::for_each(line.begin(), line.end(), [](char & c){ c = ::tolower(c); });

          if (line.find("content-length") != std::string::npos) {
            //Found the last header - Remember where the content starts
            contentPostion = ss.tellg();
            break;
          }
        }

        if (responseLine.empty() == false) {
          std::deque<std::string>    decomposed = misc::split(responseLine, ' ');
          if (decomposed.size() >= 2) {
            if (decomposed[1] == "200") {
              return ss.str().substr(contentPostion);    // Remaining data in ss is the content. There may be empty lines at the beginning
            }

            if ( (decomposed[1] == "406") or (decomposed[1] == "418") ) {
              // The server will NEVER response positively
              throw permanentTangFailure(url + "-" + ss.str().substr(contentPostion));
            }
          }
        }

        throw failedTangInteraction(url);
      }
    }
    throw failedTangInteraction(url);
  }

}
