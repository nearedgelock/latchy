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
#include "latchyMain.h"
#include "helpers/log.h"

int main(int argc, char** argv) {
  // This simply jumps to the real main in the given namespace
  try {
    latchy::main(argc, argv);
    int       returncode = latchy::run(latchy::inputConfiguration);
    INFO() << "Return code (which we use as the exit code) from run " << returncode << std::endl;
    return returncode;
  }
  catch (std::exception& exc) {
    INFO() << "Exception in main" << std::endl;
  }
}


