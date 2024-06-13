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
#include "assetProvider.h"
#include "configuration.h"
#include "assets.h"
#include "helpers/forkExec.h"
#include "helpers/log.h"

#include <memory>
#include <thread>
#include <fstream>
#include <cstring>
#include <chrono>

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>

namespace assetserver {
  using namespace std::chrono_literals;

  assetProvider::assetProvider(std::shared_ptr<assetSource<std::string>> p, const std::string& f, std::size_t c, bool fifo): assetProviderBase<std::string>(p), fileName(f), allocatedReadEvent(c), useFifo(fifo) {
    enableNonBindingMonitoring = true;
    stopDelay = 10s;
  }

  void assetProvider::start() {
    if (useFifo == false) {
      provideWithRegularFile();
    } else {
      provideWithFifo();
    }
  }

  void assetProvider::stop() {
    if (enableNonBindingMonitoring == true) {
      USERMSG() << "Monitor access to " << fileName << " for " << stopDelay.count() << "s" << std::endl;
      std::this_thread::sleep_for(stopDelay);
    }
    terminate = true;
    if (monitorTask.valid() == true) {
      monitorTask.wait();
    }
    printInfo();
  }

  void assetProvider::provideWithFifo() {
    // Create the fifo, if it does not already exists
    int                retval = mkfifo(fileName.c_str(), (unsigned int)0600);

    if ( (retval == 0 ) or (errno == EEXIST) ) {
      providerTask = std::async([&]() {
        // This runs in its own thread
        INFO() << "Starting provider thread for " << fileName << std::endl;
        try {
          // The FIFO is ready to be opened by the client
          //ready = true;

          if (enableNonBindingMonitoring == true) {
            monitorTask = std::async([&]() { monitorFileConsumption(false); });
            while ( (terminate == false) and (monitorReady == false) ) {}    // We want to wait until the monitor thread is ready
          }

          prepareFifo();
          postFifoPreparation();
          clientOpened = true;

          INFO() << "Named pipe is open and ready, we deliver to " << fileName << std::endl;

          deliverDataToFifo();
          source->destroy();

          // Normal ending
          close(descriptor);
        } catch(std::exception &exc) {
          source->destroy();
          throw;
        }

        INFO() << "Stoping provider thread for " << fileName << std::endl;
      });

    } else {
      source->destroy();
      throw openError(fileName, "Failed to create the fifo - " + std::string(strerror(errno)));
    }
  }

  void assetProvider::provideWithRegularFile() {
    // We use a task / thread
    providerTask = std::async([&]() {
      // This runs in its own thread
      try {
        // Create and write the target regular file
        createRegularFile();
        std::size_t written = writeToRegularFile();

        DEBUG() << "We wrote " << written << " bytes to " << fileName.c_str() << std::endl;

        // Destroy the data at the source
        source->destroy();
        close(descriptor);

        //ready = true;

        // Monitor the file access by the end-application / client
        monitorTask = std::async([&]() { monitorFileConsumption(true); });
        monitorTask.wait();

        // Normal ending
        USERMSG() << "Clear-text secret \"" << fileName.c_str() << "\" was entirely consumed, destroying it" << std::endl;
        if (unlink(fileName.c_str()) != 0) {
          throw genericError(fileName, "Fatal error on deleting file - " + std::string(strerror(errno)));
        }
      } catch(std::exception &exc) {
        USERMSG() << "Unexpected error processing plain-text secret " << fileName.c_str() << std::endl;
        source->destroy();
        throw;
      }
    });
  }

  void assetProvider::prepareFifo() {
    // First step is to open the fifo. This waits until the other end opens it too (for reading)
    while ( (terminate == false) and (descriptor <= 0) ) {
       descriptor = open(fileName.c_str(), O_CLOEXEC | O_NOFOLLOW | O_WRONLY | O_NONBLOCK);
      if (descriptor > 0) {
        // The fifo is open, The implication is that the other end also opened the fifo (for reading)
        USERMSG() << "Fifo successfully opened at " << fileName << std::endl;
        break;
      } else {
        // It is an error. Some may be fatal but other not so...
        if ( (errno == EINTR) or (errno == ENXIO) ) {
          // Not fatal. We will try again shortly
          std::this_thread::sleep_for(250ms);
        } else {
          // Anything else is fatal!
          throw openError(fileName, "Fatal error - " + std::string(strerror(errno)));
        }

      }
    }
  }

  void assetProvider::deliverDataToFifo() {
    // Now that the fifo is successfully opened we just need to write the data (using the assetProvider)
    // We do not use anything fancy since we expect the data size to be relatively small and the other end
    // quick at grabbing the data. So, instead of using select/poll/epoll, we presume the best (all the
    // data fits in one go) and if not use a relatively small sleep delay.
    std::size_t  writtenSoFar = 0;
    while (terminate == false) {
      if (source->isReady() == true) {
        // Write the data (using non blocking IO)
        int      retval = write(descriptor, getBuffer() + writtenSoFar, getBufferSize() - writtenSoFar);
        if (retval > 0) {
          // We wrote some data. May be we are done!
          writtenSoFar += retval;
          if (writtenSoFar >= getBufferSize()) {
            // We are done!!
            break;
          }
        } if (retval == 0) {
          // No data written, probably the IO Buffer is simply full. Lets wait a little...
          std::this_thread::sleep_for(100ms);
        } else {
          //
          // It is an error. Some may be fatal but other not so...
          //
          if (errno == EINTR) {
            // Just an interrupt. Lets go at writing again immediately since this does not indicate a buffer full situation
          } else if ( (errno == EWOULDBLOCK) or (errno == EAGAIN) ) {
            // Not fatal but IO buffer full. We will try again shortly
            std::this_thread::sleep_for(100ms);
          } if (errno == EPIPE) {
            // The other unexpectedly closed the pipe while we were still writting to it!!!
            source->destroy();
            close(descriptor);
            throw brokenPipe(fileName);
          } else {
            // Anything else is fatal!
            source->destroy();
            close(descriptor);
            throw openError(fileName, "Fatal error - " + std::string(strerror(errno)));
          }
        }
      } else {
        // Wait for the data. Just wait a little
        std::this_thread::sleep_for(250ms);
      }
    }

  }

  void assetProvider::createRegularFile() {
    // We create a regular file to write the actual content to it.
    while ( (terminate == false) and (descriptor <= 0) ) {
      descriptor = open(fileName.c_str(), O_CLOEXEC | O_WRONLY | O_NONBLOCK | O_CREAT | O_TRUNC, (unsigned int)0600);
      if (descriptor > 0) {
        // The file is open
        break;
      } else {
        // It is an error. Some may be fatal but other not so...
        if ( errno == EINTR ) {
          // Not fatal. We will try again shortly
          std::this_thread::sleep_for(250ms);
        } else {
          // Anything else is fatal!
          USERMSG() << "Failed to create secret file " << fileName << " (permissions?)" << std::endl;
          throw openError(fileName, "Fatal error - " + std::string(strerror(errno)));
        }

      }
    }
  }

  std::size_t assetProvider::writeToRegularFile() {
    // Actual writing to the file
    std::size_t  writtenSoFar = 0;
    while (terminate == false) {
      if (source->isReady() == true) {
        // Write the data (using non blocking IO)
        int      retval = write(descriptor, getBuffer() + writtenSoFar, getBufferSize() - writtenSoFar);
        if (retval > 0) {
          // We wrote some data. May be we are done!
          writtenSoFar += retval;
          if (writtenSoFar >= getBufferSize()) {
            // We are done!!
            break;
          }
        } if (retval == 0) {
          // No data written, probably the IO Buffer is simply full. Lets wait a little...
          std::this_thread::sleep_for(100ms);
        } else {
          //
          // It is an error. Some may be fatal but other not so...
          //
          if (errno == EINTR) {
            // Just an interrupt. Lets go a writing again immediately since this does not indicate a buffer full situation
          } else if ( (errno == EWOULDBLOCK) or (errno == EAGAIN) ) {
            // Not fatal but IO buffer full. We will try again shortly
            std::this_thread::sleep_for(100ms);
          } else {
            // Anything else is fatal!
            USERMSG() << "Failed to write (maybe some) to secret file " << fileName << std::endl;
            source->destroy();
            close(descriptor);
            if (unlink(fileName.c_str()) != 0) {
              throw genericError(fileName, "Fatal error on deleting file - " + std::string(strerror(errno)));
            }
            throw openError(fileName, "Fatal error - " + std::string(strerror(errno)));
          }
        }
      } else {
        // Wait for the data. Just wait a little
        std::this_thread::sleep_for(250ms);
      }
    }

    return writtenSoFar;
  }

  void assetProvider::monitorFileConsumption(bool autoStop) {
    // Now, we just need to wait for the client to open and then close the regular file. We use inotify to track
    // the client operations.
    int          inotifyFD = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotifyFD == 0) {
      // Error with inotify. Bail out
      close(inotifyFD);
      throw genericError(fileName, "Fatal error creating the inotify object- " + std::string(strerror(errno)));
    }

    uint32_t     mask = IN_OPEN | IN_ACCESS | IN_CLOSE;
    if ( inotify_add_watch(inotifyFD, fileName.c_str(), mask) < 0) {
      // Error with inotify. Bail out
      close(inotifyFD);
      throw genericError(fileName, "Fatal error adding a watch to the inotify object- " + std::string(strerror(errno)));
    }

    bool isDone = false;
    monitorReady = true;
    while ( (terminate == false) and (isDone == false) ) {
      int                       retval;
      std::size_t               bufferSize = 10*(sizeof(struct inotify_event) + NAME_MAX) + 1;
      unsigned char             buffer[bufferSize];

      retval = read (inotifyFD, (void*) buffer, bufferSize);
      if (retval > 0) {
        // Successful event - We now need to scan the chain of events
        unsigned char           *current = (unsigned char*)buffer;
        struct inotify_event    *event = reinterpret_cast<struct inotify_event*> (current);

        while ( (event != nullptr) and (isDone == false) ) {
          if ( event->mask & IN_ACCESS ) {
            // Read event, we do not anything just yet with this since this does not
            // indicate how much data was read. NOt very usefulll
            ++readEventCount;
            INFO() << "File " << fileName << " was accessed" << std::endl;
          }          
          if ( event->mask & IN_OPEN ) {
            // Opening vent
            INFO() << "File " << fileName << " was opened" << std::endl;
            clientOpened = true;
            ++openEventCount;
          }
          if ( (event->mask & IN_CLOSE_WRITE ) or ( event->mask & IN_CLOSE_NOWRITE ) ){
            INFO() << "File " << fileName << " was closed, count is " << allocatedReadEvent << std::endl;
            ++closeEventCount;
            if (allocatedReadEvent != 0) {
              --allocatedReadEvent;
            }
            if ( (autoStop == true) and (allocatedReadEvent == 0) ) {
              isDone = true;
            }
          }

          // Goto next event
          current += sizeof(struct inotify_event) + event->len;
          event = reinterpret_cast<struct inotify_event*> (current);

          if (event->wd <= 0) {
            break;
          }
        }

        std::this_thread::sleep_for(50ms);
      } else {
        // An error occurred. Some may be fatal and some are not...
        if (errno == EINTR) {
          // Just an interrupt. Try again...
        } else if ( (errno == EAGAIN) or (errno == EWOULDBLOCK) ) {
          // Can not process yet the read so wait a little and try again
          std::this_thread::sleep_for(50ms);
        } else {
          // Anything else is fatal!
          close(inotifyFD);
          USERMSG() << "Error while watching access to " << fileName.c_str() << std::endl;
          if (unlink(fileName.c_str()) != 0) {
            throw genericError(fileName, "Fatal error on deleting file - " + std::string(strerror(errno)));
          }
          throw genericError(fileName, "Fatal error on reading inotify for events - " + std::string(strerror(errno)));
        }
      }
    }

    DEBUG() << "Completed monitoring file " << fileName << std::endl;
    close(inotifyFD);
  }

  void  assetProvider::printInfo() const {
    USERMSG() << "Completed providing " << fileName << " to client." << std::endl;
    USERMSG() << "\t Number of open events                " << openEventCount << std::endl;
    USERMSG() << "\t Number of close events (1 may be us) " << closeEventCount << std::endl;
  }

  void assetProviderStdout::start() {
    providerTask = std::async([&]() {
      DEBUG() << "Starting the provider, feeding the stdout" << std::endl;
      try {
        // All we need to do, once the source is ready, is to output the data on stdout. The user requested it so we leave it to him
        // to do what ever is needed with the data. In all likelihood, the user will either redirect to a file (unrecommended but can
        // not do anything about it) or pipe to another process.

        // std::cout writes to the buffer, which we will flush, may block. It is Ok. Although, we would prefer to not and delete
        // or destroy the source data as soon as possible.

        while (terminate == false) {
          if (source->isReady() == true) {
            // Write the data, destroy and flush
            INFO() << "Providing unsealed secret on stdout" << std::endl;
            std::cout << source->getAsset();
            //logData(source->getAsset());
            source->destroy();
            std::cout.flush();
            terminate = true;
          } else {
            // Wait for the data. Just wait a little
            std::this_thread::sleep_for(100ms);
          }
        }

        // Destroy the data at the source, just to be sure...
        source->destroy();
      } catch(std::exception &exc) {
        USERMSG() << "Unexpected error while output to stdout - " << exc.what() << std::endl;
        source->destroy();
        throw;
      }
    });
  }

} // namespace assetserver
