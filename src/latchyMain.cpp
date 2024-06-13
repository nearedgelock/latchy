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
#include "help.h"
#include "helpers/log.h"
#include "metaInfo/metaInfo.h"

#include <memory>
#include <string>
#include <iostream>
#include <stdexcept>
#include <array>

#include <unistd.h>
#include <getopt.h>
#include <cstdlib>

namespace latchy {
  using namespace std::chrono_literals;
  std::string     inputConfiguration;
  bool            dumpHeader = false;

  void processCommandLine(int argc, char** argv) {
    //
    // Process the command line for options and arguments
    //
    // We have the following option
    //
    // -h, --help     Show the help content
    // -c, --cfg {}   JSON formatted configuration string
    // -d, --debug    Enable DEBUG level output (stderr), does include INFO as well
    // -t, --trace    Enable INFO level output (to stderr)
    // --dump         Simply dump the protected header (to stderr)
    //
    
    DEBUG() << "We found " << argc << " arguments, including the process filename." << std::endl;
    if (argc >= 2) {
      int         count = argc;
      char**      vector = argv;
      for (; count--; vector++) {
        DEBUG() << *vector << std::endl;
      }
    }    

    std::string                       shortOptions("hc:");
    std::array<struct option, 6>      longOptions{{
      {"help", no_argument, nullptr, 'h'},
      {"cfg", required_argument, nullptr, 'c'},
      {"debug", no_argument, nullptr, 'd'},
      {"trace", no_argument, nullptr, 't'},
      {"dump", no_argument, nullptr, 1000+'d'},
      {0, 0, 0, 0} },
    };
    while (1) {
      int                             c;
      int                             option_index = 0;
      c = getopt_long(argc, argv, shortOptions.c_str(),longOptions.data(), &option_index);

      if (c == -1) {
        break;
      }

      switch (c) {
      case 'h':
        showHelp();
        exit(0);
        break;

      case 'c':
        if (optarg == nullptr) {
          // This is an error
          USERCOUT() << "Missing Argument to option cfg - We bail out" << std::endl;
          exit(-1);
        } else {
          // Accept the argument. It may be hill formed though.
          inputConfiguration = std::string(optarg);
          LOGTOFILE(std::string("Configuration string: ") + inputConfiguration + "\n");
        }
        break;

      case 'd':
        logger::ISDEBUG = true;
        logger::ISINFO = true;
        break;

      case 't':
        logger::ISINFO = true;
        break;

      case '?':
        // Don't say anything, the library already tells the user
        exit(-1);
        break;

      case 1000+'d':
        dumpHeader = true;
        break;

      default:
        USERMSG() << "Character was " << c << std::endl;
        USERMSG() << "Unexpected result when parsing the command line " << std::endl;
        exit(-1);
        break;

      }
    }
  }

  std::string captureStdIn() {
    // We need to get something from the stdin. If it is a configuration, then it is an explicit mode and simply grab it.
    //
    // If it is NOT a configuration, then we prepare an implicit configuration and grab the JWE from stdin

    std::string     cfg;

    // Peek at the first available character from stdin. A '{' or '[' is presuming a configuration. If not, then it is
    // a JWE.
    std::cin.exceptions(std::ios::failbit | std::ios::badbit);      // No exception on eof

    // This blocks until we get this first character. This is OK since without configuration there is nothing
    // to do yet. The character remains in the whatever input buffer.
    int c = std::cin.peek();

    if (c == EOF) {
      USERMSG() << "We are expecting a configuration or a JWE from stdin but got nothing - We bail out" << std::endl;
    } else if ( (c != '{') and ((c != '[') ) ) {
      // We presume a JWE so we assume the implicit mode. We simply prepare the default configuration as needed.
      cfg = R"({"secrets":[{"iMethod":"STDIN", "lockingMethod":"CLEVIS", "eMethod":"STDOUT"}]})";
    } else {
      // Explicit configuration from stdin. Lets get it!!
      std::stringstream               rdbuffer;
      std::string                     inputData;
      std::cin.exceptions(std::ios::failbit | std::ios::badbit);      // No exception on eof
      std::cin >> rdbuffer.rdbuf();   // Until eof

      inputData = std::move(rdbuffer.str());
      // Check if we have a configuration, which is indicated by an opening '{' and a closing '}' (i.e JSON signature). If not, then it is some error.
      if (inputData.empty() == false) {
        // Check for a JSON signature - The beginning
        if ( (inputData.size() >= 2) and (inputData[0] == '{') ) {
          // Potentially JSON
          // Check if we have an extra line feed at the end (multiple)
          while (inputData.empty() == false) {
            if (inputData.back() == '\n') {
              inputData.pop_back();
            } else {
              break;
            }
          }

        // Check for the end of the JSON signature
        if ( (inputData.size() >= 1) and  (inputData[inputData.size()] == '}') ) {
          cfg = std::move(inputData);
        } else {
          // An error of some sort
          USERMSG() << "We are expecting a JSON value but did not find a closing } - We bail out" << std::endl;
          exit(-1);
        }
      } else {
        // Also an error of some sort...
        USERMSG() << "We are expecting a JSON value but did not find an opening { - We bail out" << std::endl;
        exit(-1);
      }
    } else {
      // Another error of some sort...
      USERMSG() << "No input - We bail out" << std::endl;
      exit(-1);
    }
  }   

  return cfg;
}

  int run(std::string configuration) {
    //
    // Now, launch the actual processing
    //
    try {
      // Just to show some basic process info to user
      meta::composition           metaData;
      metaData.printInfo();

      INFO() << "Starting overall processing of the given configuration" << std::endl;
      if (configuration.empty() == false) { 
        DEBUG() << "The configuration string is " << configuration << std::endl;  
        assets::list    assets(configuration::parseStringToMsg(configuration), dumpHeader);    // This also starts all the providers.
        DEBUG() << "The assets were created and we should be fully running" << std::endl;
      } else {
        USERMSG() << "Missing configuration" << std::endl;
        return -1;
      }
    } catch(std::exception& exc) {
      USERMSG() << "Unexpected exception - " << exc.what() << std::endl;
      return -1;
    }

    // At this point, all of the assets are enabled and running. assets will be deconstructed. The deconstruction returns when all is done and said.
    // Said otherwise, we may hang here long....
    return 0;
  }

  //
  // Real process main.
  //
  int main(int argc, char** argv) {
    //
    // Top level execution principles
    //
    // 1 - We check the command line arguments for a configuration (JSON formatted) or a --help request. But not both (help takes over)
    // 2 - If not command line configuration is available we check an environment variable to find one.
    // 3 - If no configuration was found, then we check the stdin, where we grab a JWE, a JSON encoded configuration, or nothing.
    //
    // At this point, we decide if a use an implicit or explicit configuration. If implicit, this is what we use
    //   A - Input JWE is from the std input
    //   B - Output of the secret is to stdout
    //   C - We terminate immediately once the stdout is complete
    //
    //  Log outputs appear on stderr
    //
    // There is no timeout. Only a success or a signal terminates us.
    //
    // For explicit, then we follow the configuration we were given. A JWE from the stdin is still possible but if so, a
    // configured asset taking it must be defined. Otherwise a configuration error is declared.
    //

    // This may cause secret to leak when any of the log facility is enabled.
    //ACTIVATELOG();

    try {
      //
      // Check the command line arguments.
      //
      bool                implicit = false;
      LOGTOFILE(std::string("Number of command line arguments: ") + std::to_string(argc) + "\n");
      if (argc >= 2) {
        processCommandLine(argc, argv);   // This may not return in case of an error. Of if help was asked.
      } else if (std::getenv("LATCHYCFG") != nullptr) {
          inputConfiguration = std::string(std::getenv("LATCHYCFG"));
          LOGTOFILE(std::string("Configuration string: ") + inputConfiguration + "\n");        
      } else {
        implicit = true;
      }

      // Process the stdin. May be a configuration or a JWE
      if (inputConfiguration.empty() == true) {
        inputConfiguration = captureStdIn();
      }     

    } catch(std::exception& exc) {
      USERMSG() << "Unexpected exception - " << exc.what() << std::endl;
      exit (-1);
    }

    return 0;
  }
} // namespace latchy

