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
#include <memory>
#include <atomic>
#include <stdexcept>
#include <future>
#include <chrono>

#include <stdio.h>

#include "assetSource.h"
namespace assetserver {
  /// Asset provider
  ///
  /// This provider unlocks access to assets to other processes. The access is abstracted and
  /// enables asset(s) to be fetched, unlocked, or otherwise processed before being handed to
  /// the other process.
  ///
  /// One use case is to unlock a private key, make it available (in its unlocked form) and then
  /// delete the unlocked key once it is been delivered.
  ///
  /// A base delivery mechanism is provided in the form of a named pipe (FIFO) writer. Other
  /// derived class may be built to support alternate delivery mechanism.
  ///
  /// The assetSource provides a pure abstract interface to the assetProvider, enabling different
  /// source to be defined. See assetSource.h

  // Mostly (but not entirely) an abstract class
  template <typename T>
  class assetProviderBase {
  public:
    assetProviderBase(std::shared_ptr<assetSource<T>> p): source(p) { if (source == nullptr) throw std::runtime_error("Missing argument in constructor"); };
    virtual ~assetProviderBase() { if (providerTask.valid() == true) { providerTask.wait(); } };

    virtual void                        start() =0;
    virtual std::future_status          wait(const std::chrono::nanoseconds duration = std::chrono::seconds(0)) const { if (providerTask.valid() == true) { return providerTask.wait_for(duration); } return std::future_status::timeout; };
    virtual void                        get() { if (providerTask.valid() == true) { providerTask.get(); } };

  protected:
    std::shared_ptr<assetSource<T>>     source = nullptr;
    std::future<void>                   providerTask;           // We want the provider to run in its own task / async / thread

    const char*                         getBuffer();
    const std::size_t                   getBufferSize();

    void                                dummyPromise();         // Mostly used when something is wrong and the provider needs to cancel without running.

    std::atomic<bool>                   terminate = false;      // Flag, mostly used to signal the task to terminate
    std::chrono::seconds                stopDelay = 0s;         // Generic delay to extend the processing for example when we want to make sure inotify catches all events
  };

  template<>
  inline const char* assetProviderBase<std::string>::getBuffer() {
    return &source->getAsset()[0];
  }

  template<>
  inline const std::size_t assetProviderBase<std::string>::getBufferSize() {
    return source->getAsset().size();
  }

  template<>
  inline void assetProviderBase<std::string>::dummyPromise() {
    std::promise<void>    dummy;
    providerTask = dummy.get_future();
    dummy.set_value();
  }

  /// Asset provider handling files and pipes
  class assetProvider: public assetProviderBase<std::string> {
  public:
    assetProvider(std::shared_ptr<assetSource<std::string>> p, const std::string& f, std::size_t c, bool fifo);
    virtual ~assetProvider() { stop(); };

    //bool                                isReady() const { return ready; };

    virtual void                        start();
    void                                stop();

    void                                provideWithFifo();           // Setup a provider task which uses a Fifo to deliver the content to the client
    void                                provideWithRegularFile();    // Setup a provider task which uses a regular file to deliver the content to the client. Uses inotify to determine access

  protected:
    std::string                         fileName;
    std::size_t                         allocatedReadEvent;          // Number of file read we count before destroying the file (for regular file mode of operation)
    bool                                useFifo;                     // Output to a named pipe when true

    int                                 descriptor = 0;

    //std::atomic<bool>                   ready = false;               // Flag, mostly used to signal that the provider is ready to deliver to a client. The data may NOT yet be ready.

    std::atomic<bool>                   clientOpened = false;        // Indicates that the client opened (the fifo or the regular file)
    std::size_t                         openEventCount = 0;
    std::size_t                         readEventCount = 0;
    std::size_t                         closeEventCount = 0;



    // Named pipe (or Fifo) helper methods
    void                                prepareFifo();               // Prepare a fifo (named pipe). This completes once the other end ALSO opened (for reading)
    virtual void                        postFifoPreparation() { };   // Additional processing after the named pipe (fifo) is opened. If any....
    void                                deliverDataToFifo();
                      

    // Regular file  helper methods
    void                                createRegularFile();
    std::size_t                         writeToRegularFile();

    // Global monitoring
    bool                                enableNonBindingMonitoring = false;
    void                                monitorFileConsumption(bool autoStop);    // Monitor the file consumption (i.e open and then closed). When autoStop is true, we termiante once allocatedReadEvent is zero
    std::future<void>                   monitorTask;                 // File access monitor task
    std::atomic<bool>                   monitorReady = false;
    void                                printInfo() const;

  public:
    class openError: public std::runtime_error {
    public:
      openError(const std::string &filename, const std::string& msg = ""): runtime_error("Failed to open named pipe or file as " + filename + ((msg.empty() == false) ? (" - " + msg) : "")) { };
    protected:
    };
    class brokenPipe: public std::runtime_error {
    public:
      brokenPipe(const std::string &filename): runtime_error("Other end broke the pipe for " + filename ) { };
    protected:
    };
    class genericError: public std::runtime_error {
    public:
      genericError(const std::string &filename, const std::string& msg = ""): runtime_error("Generic error for " + filename + ((msg.empty() == false) ? (" - " + msg) : "") ) { };
    protected:
    };
  };

  /// Asset provider to output on the stdout
  class assetProviderStdout: public assetProviderBase<std::string> {
  public:
    assetProviderStdout(std::shared_ptr<assetSource<std::string>> p): assetProviderBase<std::string>(p) { logger::USESTDERR = true; };
    virtual ~assetProviderStdout() { terminate = true; };

    virtual void                        start();

  };

  // Few helpers
  inline void logData(const std::string &buffer) {
    //LOGTOFILE(std::string("**************** DO NOT PUT INTO PRODUCTION WITH LOGGING ENABLED *********** \nSecret outputed: ") + jose::toB64(buffer) + "\n");
    std::stringstream o;
    for (unsigned char c : buffer) {
        o << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << static_cast<unsigned int>(c);
    }
    LOGTOFILE(std::string("Secret in hex  : ") + o.str() + "\n");
  }
   
} // namespace assetserver


