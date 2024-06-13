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
#include <string_view>
#include <stdexcept>
#include <vector>
#include <future>
#include <mutex>
#include <chrono>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

/// Collection of fork and exec C++ wrappers
///
/// The intent here is to create a portable low level abstraction that is RAII compatible, modern C++,
/// do not have external dependency but not portable to the point of supporting Windows... In other
/// this only works with POSIX compliant platforms.
///

namespace os {
namespace launch {
  using namespace std::chrono;
  using cmdLineItem_t =                  std::string;
  using cmdLine_t =                      std::vector<cmdLineItem_t>;
  using cmdArgList_t =                   cmdLine_t;
  using cmdEnvList_t =                   cmdArgList_t;

  class NotValid : public std::logic_error {
  public:
    NotValid() : std::logic_error("An error was found in the provided argument or environment variable list") { };
  };

  //template
  int        execvpe(const std::string& binaryFilename, const cmdArgList_t arg, const cmdEnvList_t env);
  int        launch(const std::string& binaryPath, const cmdArgList_t arg, const std::string initialDirectory = "", int intFD = -1, int outFD = -1, int errFD = -1, const cmdEnvList_t env = cmdEnvList_t());

  class exec {
  public:
  public:
    exec(const cmdLine_t& cmdline, bool block = true, bool stdin = false, bool captureStdout = false, bool captureStderr = false);
    virtual ~exec () {};

    bool                                 isTerminate(seconds waitFor = 0s) const;
    int                                  exitCode() { exitCodeTask.wait(); return exitCodeTask.get(); };

    void                                 sendBuffer(const std::string& data, bool close = true);    // To send to the child's STDIN
    const std::string&                   getOutput() const { return outputBuffer; };   // Need to fix this, only ok AFTER the child exited due to race condition
    const std::string&                   getError() const { return errorBuffer; };   // Need to fix this, only ok AFTER the child exited due to race condition

    void                                 clearBuffer();
  private:
    //
    // Input and output are from the child's perspective
    //

    int                                  inputPipe = -1;     // To connect to the child's STDIN
    int                                  outputPipe = -1;    // To connect to the chidl's STDOUT
    int                                  errorPipe = -1;     // To connect to the chidl's STDERR

    std::string                          outputBuffer;       // From child's STDOUT
    std::string                          errorBuffer;        // From child's STDERR

    int                                  execute(const cmdLine_t& cmdline, int intFD, int outFD, int errFD);
    pid_t                                pid = 0;
    std::future<int>                     exitCodeTask;

    void                                 serviceInputPipe();
    std::future<void>                    stdinTask;
    std::atomic<bool>                    mustClose;
    std::string                          inputBuffer;
    mutable std::recursive_mutex         _mutex;

    void                                 serviceOutputPipe(bool stderr = false);
    std::future<void>                    captureStdoutTask;
    std::future<void>                    captureStderrTask;
    mutable std::recursive_mutex         _mutexOutput;

    int                                  getCurrentPipeSpace(int pipe) { return fcntl(pipe, F_GETPIPE_SZ); };

  };
} // namespace launch
} // namespace os

