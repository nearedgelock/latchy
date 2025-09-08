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
#include "configuration.h"
#include "helpers/log.h"

#include <sstream>

#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/util/json_util.h>
namespace configuration {

  const std::string prettyPrintJson(const std::string& json) {
    // This is just a crude JSON pretty printer. We do it ourselves to avoid another dependency
    std::string         indent = "";
    std::stringstream   out;

    for (const auto& letter : json) {
      if ( (letter == '}') or (letter == ']') ) {
        indent.pop_back();
        out << std::endl << indent;
      }

      out << letter;

      if ( (letter == '{') or (letter == '[') ) {
        indent += ' ';
        out << std::endl << indent;
      }
      if ( (letter == ',')  ) {
        out << std::endl << indent;
      }
    }

    return out.str();
  }

  std::string convertMsgToJsonString(const google::protobuf::Message& m) {
    std::string retval;
    absl::Status result = google::protobuf::util::MessageToJsonString(m, &retval);
     return retval;
  }

  secretCfgList_t parseStringToMsg(std::string& inputConfiguration) {
    // The provided configuration string may not be a valid protoBuf message. A message is
    // ALWAYS a JSON object. But since the configuration is actually an object with a single element,
    // which is a list, we check if we are passed a list, in such case we adjust the string
    // to match our protobuf message definition.
    std::size_t       firstBrace = inputConfiguration.find('{', 0);
    std::size_t       firstSquarebracket = inputConfiguration.find('[', 0);

    DEBUG() << "First brace character is at position " << firstBrace << std::endl;
    DEBUG() << "First square bracket character is at position " << firstSquarebracket << std::endl;
    if (firstSquarebracket == std::string::npos) {
      // Ok, we presume that we are given a single configuration. And that the
      // message is hill formed if substring \"secrets\":" is missing
      if (inputConfiguration.find("\"secrets\":", 0) == std::string::npos) {
        // No secrets JSON keyword
        inputConfiguration.insert(0, "{\"secrets\":[");
        inputConfiguration.append("]}");
      } else {
        // The correct JSON keyword is present. But the [] are missing
        // ATM we simply raise an error!
        throw std::runtime_error("Invalid configuration string. We expect the secrets value to be an array / list");
      }
    } else {
      if (firstSquarebracket <  firstBrace) {
        DEBUG() << "Modifying the configuration string to match the protobuf model" << std::endl;
        inputConfiguration.insert(0, "{\"secrets\":");
        inputConfiguration.push_back('}');
      }
    }
    LOGTOFILE(std::string("Configuration string: ") + inputConfiguration + "\n");  

    // Now, the actual protobuf handling
    google::protobuf::util::JsonParseOptions      jsonOpts;
    model::latchy::secretList                     message;

    jsonOpts.ignore_unknown_fields = true;
    jsonOpts.case_insensitive_enum_parsing = false;

    absl::Status  result = google::protobuf::util::JsonStringToMessage(inputConfiguration, (google::protobuf::Message*) &message, jsonOpts);
    if (result.ok() == true) {
      return message;
    }

    USERMSG() << "Failed to parse the configuration JSON, we bail out - " << result.message() << std::endl;
    DEBUG() << "Configuration string was - " << inputConfiguration << std::endl;

    throw std::runtime_error("Failed to parse a JSON string into a protobuf Message");
  }

  void printSecret(const secretCfg_t& secret) {
    std::cout << "A secret declaration" << std::endl;

    std::cout << " INGRESTION" << std::endl;
    std::cout << "   Type       " << secretIngestionMethods_Name(secret.imethod()) << std::endl;
    std::cout << "   Locking    " << secretLockingMethods_Name(secret.lockingmethod()) << std::endl;
    std::cout << "   File name  " << secret.in() << std::endl;
    std::cout << "   Var name   " << secret.var() << std::endl;

    std::cout << " EGRESS" << std::endl;
    std::cout << "   Type       " << secretEgressMethods_Name(secret.emethod()) << std::endl;
    std::cout << "   File name  " << secret.out() << std::endl;
    std::cout << "   Read count " << secret.outcount() << std::endl;

  }
} // namespace configuration
