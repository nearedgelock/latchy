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
#include "forkExec.h"
#include "log.h"

#include <chrono>
#include <cstring>
#include <thread>
#include <sys/types.h>
#include <sys/wait.h>

namespace os {
namespace launch {
  using namespace std::chrono_literals;

  int largestFD() {
    // Largest FD ID for current process (using info in proc)
    int         largest = 0;

    for (const auto& fileEntry : std::filesystem::directory_iterator("/proc/self/fd")) { 
      try {
          int   fd = std::stoi(fileEntry.path().filename().string());  // This checks that the directory entry represent a process
          if (fd > largest) {
            largest= fd;
          }
      } catch (...) {}
    }

    return largest; 
  }

  int execvpe(const std::string& binaryFilename, const cmdArgList_t arg, const cmdEnvList_t env) {
    // Convert arg to a suitable argument for the underlying execv
    char *            argv[arg.size()+2];
    std::string       command(binaryFilename);
    argv[0] = command.data();
    argv[arg.size()+1] = nullptr;
    for (std::size_t i = 0; i < arg.size(); ++i) {
      if ( (arg[i].empty() == true) and (arg[i].back() != 0) ) {
        throw NotValid();
      }

      argv[i+1] = const_cast<char*>(arg[i].data());
    }

    // Convert env to a suitable argument for the underlying execv
    char *            envp[env.size()+1];
    envp[env.size()] = nullptr;
    for (std::size_t i = 0; i < env.size(); ++i) {
      if ( (env[i].empty() == true) and (env[i].back() != 0) ) {
        throw NotValid();
      }

      envp[i] = const_cast<char*>(env[i].data());
    }

    if (env.empty() == false) {
      return ::execvpe(binaryFilename.data(), argv, envp);
    } else {
      return ::execvp(binaryFilename.data(), argv);
    }
  }

  int launch(const std::string& binaryPath, const cmdArgList_t arg, const std::string initialDirectory, int intFD, int outFD, int errFD, const cmdEnvList_t env) {
    // We must not allocate memory after fork(),
    // therefore allocate all required buffers first.
    int pid = fork();
    if (pid < 0) {
      // Error
      throw std::runtime_error(std::string("Cannot fork process for ") + binaryPath);
    } else if (pid == 0) {
      // We are in the child from this point on. There is no way out
      int       maxFD = largestFD();
      DEBUG() << "In the child, immediatelly after the fork - " << "Maximum number of file descriptor " << sysconf(_SC_OPEN_MAX) << std::endl;
      DEBUG() << "Detected largest FD ID " << maxFD << std::endl;

      // Update the CWD
      if (initialDirectory.empty() == false) {
        if (chdir(initialDirectory.c_str()) != 0) {
          _exit(72);
        }
      }

      //
      // setup redirection
      //
      if ( (intFD != -1) and (intFD != STDIN_FILENO) ) {
        dup2(intFD, STDIN_FILENO);
        close(intFD);
      }

      if ( (outFD != -1) and (outFD != STDOUT_FILENO) ) {
        dup2(outFD, STDOUT_FILENO);
        close(outFD);
      }

      if ( (errFD != -1) and (errFD != STDERR_FILENO) ) {
        dup2(errFD, STDERR_FILENO);
        close(errFD);
      }

      // close all open file descriptors other than stdin, stdout, stderr
      for (int i = 3; i < maxFD; ++i) {
        close(i);
      }

      // Unblock all signals
      sigset_t            signalSet;
      sigfillset(&signalSet);
      sigprocmask(SIG_UNBLOCK, &signalSet, nullptr);

      //
      // Point of no return - Start the new binary (ie replace the memory content...)
      //
      DEBUG() << "In the child,just before the exec" << std::endl;

      execvpe(binaryPath, arg, env);
      _exit(72);
    }

    DEBUG() << "In parent just after the fork" << std::endl;

    // Parent. - Close the end of the pipe we gave to the child
    if ( (intFD != -1) and (intFD != STDIN_FILENO) ) {
      close(intFD);
    }

    if ( (outFD != -1) and (outFD != STDOUT_FILENO) ) {
      close(outFD);
    }

    if ( (errFD != -1) and (errFD != STDERR_FILENO) ) {
      close(errFD);
    }
    return pid;
  }

  exec::exec(const cmdLine_t& cmdline, bool block, bool openStdin, bool captureStdout, bool captureStderr) {
    if (cmdline.empty() == true) {
      throw std::runtime_error(std::string("Mssing command to start a child "));
    }

    // Setup the pipes, if requested
    int         childInputPipe = -1;
    int         childOutputPipe = -1;
    int         childErrorPipe = -1;

    // STDIN of the child
    if (openStdin == true) {
      int fds[2];
      int rc = pipe(fds);
      if (rc == 0) {
        childInputPipe  = fds[0];
        inputPipe = fds[1];
      } else {
        throw std::runtime_error(std::string("Can not create pipe ") + std::strerror(errno));
      }
    }

    // STDOUT of the child
    if (captureStdout == true) {
      int fds[2];
      int rc = pipe(fds);
      if (rc == 0) {
        outputPipe  = fds[0];
        childOutputPipe = fds[1];
      } else {
        throw std::runtime_error(std::string("Can not create pipe ") + std::strerror(errno));
      }
    }

    // STDERR of the child
    if (captureStderr == true) {
      int fds[2];
      int rc = pipe(fds);
      if (rc == 0) {
        errorPipe  = fds[0];
        childErrorPipe = fds[1];
      } else {
        throw std::runtime_error(std::string("Can not create pipe ") + std::strerror(errno));
      }
    }

    for (const auto& item : cmdline) {
      //DEBUG() << item << std::endl;
    }

    exitCodeTask = std::async([&, cmdline, childInputPipe, childOutputPipe, childErrorPipe]() { return execute(cmdline, childInputPipe, childOutputPipe, childErrorPipe); } );

    if (captureStdout == true) {
      captureStdoutTask = std::async([&]() { return serviceOutputPipe(false); } );
    }
    if (captureStderr == true) {
      captureStderrTask = std::async([&]() { return serviceOutputPipe(true); } );
    }

    if (block == true) {
      exitCodeTask.wait();

      if (captureStdoutTask.valid() == true) {
        captureStdoutTask.wait();
      }
      if (captureStderrTask.valid() == true) {
        captureStderrTask.wait();
      }
    }

  };

  bool exec::isTerminate(seconds waitFor) const {
    // We think the std::future methods are thread safe by nature... std::scoped_lock lock(_mutex);

    // There may be a rare condition where we could end up waiting 3x the waitFor delay.

    if (exitCodeTask.valid() == false) {
      throw std::future_error(std::future_errc::no_state);
    }

    DEBUG() << "   Check if child terminated " << std::endl;
    if (exitCodeTask.wait_for(waitFor) != std::future_status::ready) {
      return false;
    }

    DEBUG() << "   Check if child's output grabbing is complete (if configured to grab) " << std::endl;
    if (captureStdoutTask.valid() == true) {
      if (captureStdoutTask.wait_for(waitFor) != std::future_status::ready) {
        return false;
      }
    }

    DEBUG() << "   Check if child's error output grabbing is complete (if configured to grab) " << std::endl;
    if (captureStderrTask.valid() == true) {
      if (captureStderrTask.wait_for(waitFor) != std::future_status::ready) {
        return false;
      }
    }

   return true;
  }

  void exec::sendBuffer(const std::string& data, bool close) {
    if (isTerminate(0s) == true) {
      throw std::runtime_error(std::string("Sending data to a child that already terminated"));
    }
    if (inputPipe < 0) {
      // Invalid pipe
      throw std::runtime_error(std::string("No pipe to send data to"));
    }

    // Add data to the internal buffer
    {
      std::scoped_lock lock(_mutex);
      inputBuffer.append(data);
      mustClose = close;
    }

    // Start the service task if needed
    if (stdinTask.valid() == false) {
      stdinTask = std::async([&]() { return serviceInputPipe(); } );
    }
  }

  void exec::clearBuffer() {
    // Used when sensitive data is potentially present in buffers
    inputBuffer.assign(inputBuffer.size(), (char) 0);
    outputBuffer.assign(outputBuffer.size(), (char) 0);
    errorBuffer.assign(errorBuffer.size(), (char) 0);
  }

  int exec::execute(const cmdLine_t& cmdline, int intFD, int outFD, int errFD) {
    std::string                        binary = cmdline[0];
    cmdArgList_t                       arg;
    if (cmdline.size() >= 2) {
      arg.insert(arg.begin(), cmdline.begin()+1, cmdline.end());
    }

    DEBUG() << "Launching a child " << binary << std::endl;
    pid = launch(binary, arg, "", intFD, outFD, errFD);
    DEBUG() << "PID is " << pid << std::endl;

    int                                status = 0;
    int                                rc = 0;

    do {
      DEBUG() << "Waiting for child, which pid is " << pid << std::endl;
      rc = waitpid(pid, &status, (int)0);
      DEBUG() << "Waited for child, rc is " << rc << std::endl;
    }
    while (rc < 0 && errno == EINTR);

    DEBUG() << "Done waiting for child" << std::endl;
    if (rc != pid) {
      throw std::runtime_error("Cannot wait for process " + std::to_string(pid));
    }

    // Close the stdin pipe. This makes sure the service thread unblocks / terminates
    if (inputPipe >= 0) {
      close (inputPipe);
      inputPipe = -1;
    }

    DEBUG() << "Done waiting for child, returning status 0x" << std::hex << status << std::dec << std::endl;
    if (WIFEXITED(status)) {
      return WEXITSTATUS(status);       // normal termination
    } else {
      return 256 + WTERMSIG(status);    // termination by a signal
    }
  }

  void exec::serviceInputPipe() {
    // This executes in its own thread. We can therefore use blocking IO....
    // We copy, again, the data from the buffer. Not efficient but we do not expect that
    // much data anyway. To be optimized if needed

    std::string       buf;
    while(inputPipe != -1) {
      // Grab the data
      {
        std::scoped_lock lock(_mutex);
        buf.append(inputBuffer);
        inputBuffer.clear();
      }

      if (buf.empty() == false) {
        int n;
        do
        {
          n = write(inputPipe, buf.data(), buf.size());
        }
        while (n < 0 && errno == EINTR);
        if (n < 0) {
          // An error is reported. We do not really care why so we just return.
          close (inputPipe);
          inputPipe = -1;
          return;
        }

        // Erase the data we successfully sent
        buf.replace(0, n, n, 0);  // Make sure to invalidate the data, which may be sensitive
        buf.erase(0, n);
      } else {
        std::this_thread::sleep_for (100ms);
      }

      // We were told that we should be done
      if ( (mustClose == true) and (buf.empty() == true) ) {
        std::scoped_lock lock(_mutex);
        if (inputBuffer.empty() == true) {
          close (inputPipe);
          inputPipe = -1;
        }
      }

    }

    return;
  }

  void exec::serviceOutputPipe(bool stderr) {
    // This executes in its own thread. We can therefore use blocking IO....
    int&       fd = (stderr ? errorPipe : outputPipe);

    while(fd != -1) {
      int     n = 0;
      char    buffer[4096];
      do
      {
        n = read(fd, buffer, 4096);
      }
      while (n < 0 && errno == EINTR);

      if (n <= 0) {
        // An error is reported. We do not really care why so we just return.
        // 0 means eof
        close (fd);
        fd = -1;
        return;
      }

      if (n > 0) {
        //Move the data into the final buffer return n;
        std::scoped_lock lock(_mutexOutput);
        std::string&    serviceBuffer = (stderr ? errorBuffer : outputBuffer);
        serviceBuffer.append(buffer, n);
      }
    }
  }


} // namespace launch
} // namespace os
