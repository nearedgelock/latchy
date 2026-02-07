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
#include "help.h"
#include "helpers/log.h"

void showHelp() {
  USERCOUT() \
    << "latchy is a reimplementation of clevis as a binary with few additional features. But it only supports decryption" << "\n" \
    << "and the tang pin. See https://github.com/latchset/clevis for technical details." << "\n" \

    << "\n" \
    << "latchy adds the following capabilities" << "\n" \
    << "\t- retry the tang /rec API until it succeeds or a fatal error is received" << "\n" \
    << "\t- ability to ouput the decrypted payload to a named pipe" << "\n" \
    << "\t- fully static binary without external dependency, supporting distro-less containers" << "\n" \

    << "\n" \
    << "latchy processes 1 or more clevis formatted JWE and output the corresponding PT using the selected mechanism. In" << "\n" \
    << "its simplest form, use a command similar to " << "\n" \
    << "\n" \
    << "\tcat CIPHERTEXT.jwe | latchy"<< "\n" \
    << "\n" \
    << "The output will be provided on stdout" << "\n"\

    << "\n" \
    << "For more control of the operation, a JSON configuration string may be provided. This string may be provided" << "\n" \
    << "via the stdin (in place of the JWE content), as a command line argument or via the \"LATCHYCFG\" environment" << "\n" \
    << "variable. Do" << "\n" \
    << "\n" \
    << "\tcat CONFIGURATION.json | latchy, or" << "\n" \
    << "\n" \
    << "\tlatchy --cfg '<JSON STRING>', or" << "\n" \
    << "\n" \
    << "\texport LATCHYCFG='<JSON STRING>'; cat CIPHERTEXT.jwe | latchy"<< "\n" \

    << "\n" \
    << "Command line arguments" << "\n" \
    << "\t\"--cfg\"        - JSON configuration string (see below for details)" << "\n" \
    << "\t\"--compatible\" - Support TANG with strict API content" << "\n" \
    << "\t\"--debug\"      - Verbose debugging output (on stderr)" << "\n" \
    << "\t\"--dump\"       - Output the content of the protected header of the JWE and exit. Do not perform decryption" << "\n" \
    << "\t\"--help\"       - This help" << "\n" \
    << "\t\"--trace\"      - Minimal information (on stderr)" << "\n" \

    << "\n" \
    << "JSON configuration string. This is an array providing 1 or more configurations, each pertaining to a single" << "\n" \
    << "JWE. The full format is" << "\n" \
    << "{" << "\n" \
    << "\t\"secrets\": [ { <CONFIGURATIONFORASINGLEJWE> } ]" << "\n" \
    << "}" << "\n" \
    << "\n" \

    << "Two simplified forms are supported" << "\n" \
    << "\n" \
    << "[" << "\n" \
    << "\t{ <CONFIGURATIONFORASINGLEJWE> }" << "\n" \
    << "]" << "\n" \
    << "\n or \n\n" \
    << "{" << "\n" \
    << "\t<CONFIGURATIONFORASINGLEJWE>" << "\n" \
    << "}" << "\n" \

    << "\n" \
    << "The actual configuration is " << "\n" \
    << "{" << "\n" \
    << "\t\"iMethod\": \"STDIN\" | \"IFILE\" | \"IPIPE\", " << "\n" \
    << "\t\"in\": FILENAME, " << "\n" \
    << "\t\"eMethod\": \"STDOUT\" | \"FILE\" | \"PIPE\", " << "\n" \
    << "\t\"out\": FILENAME, " << "\n" \
    << "\t\"outCount\": INTEGER" << "\n" \
    << "}" << "\n" \

    << std::endl;

  return;
};

