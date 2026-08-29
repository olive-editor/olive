

#ifndef _ofxsMultiThread_H_
#define _ofxsMultiThread_H_
// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

/** @file This file contains core code that wraps OFX 'objects' with C++ classes.

This file only holds code that is visible to a plugin implementation, and so hides much
of the direct OFX objects and any library side only functions.
*/

#include "ofxsCore.h"

typedef struct OfxMutex* OfxMutexHandle;

namespace OFX {

  /** @brief Multi thread namespace */
  namespace MultiThread {

    /** @brief Class that wraps up SMP multi-processing */
    class Processor {
    private :
      /** @brief Function to pass to the multi thread suite */
      static void staticMultiThreadFunction(unsigned int threadIndex, unsigned int threadMax, void *customArg);

    public  :
      /** @brief ctor */
      Processor();

      /** @brief dtor */
      virtual ~Processor();            

      /** @brief function that will be called in each thread. ID is from 0..nThreads-1 nThreads are the number of threads it is being run over */
      virtual void multiThreadFunction(unsigned int threadID, unsigned int nThreads) = 0;

      /** @brief call this to kick off multi threading

      The nCPUs is 0, the maximum allowable number of CPUs will be used.
      */
      virtual void multiThread(unsigned int nCPUs = 0);
    };

    /** @brief Has the current thread been spawned from an MP */
    bool isSpawnedThread(void);

    /** @brief The number of CPUs that can be used for MP-ing */
    unsigned int getNumCPUs(void);

    /** @brief The index of the current thread. From 0 to numCPUs() - 1 */
    unsigned int getThreadIndex(void);

    /** @brief An OFX mutex */
    class Mutex {
    protected :
      OfxMutexHandle _handle; /**< @brief The handle */

    public :
      /** @brief ctor */
      Mutex(int lockCount = 0);

      /** @brief dtor */
      virtual ~Mutex(void);

      /** @brief lock it, blocks until lock is gained */
      void lock();

      /** @brief unlock it */
      void unlock();

      /** @brief attempt to lock, non-blocking

      \brief returns
      - true if the lock was achieved
      - false if it could not
      */
      bool tryLock();
    };

    /// a class to wrap around a mutex which is exception safe
    /// it locks the mutex on construction and unlocks it on destruction
    class AutoMutex {
    protected :
      Mutex &_mutex;

    public :
      /// ctor, acquires the lock
      explicit AutoMutex(Mutex &m)
        : _mutex(m)
      {
        _mutex.lock();
      }

      /// dtor, releases the lock
      virtual ~AutoMutex()
      {
        _mutex.unlock();
      }

    };
  };
};

#endif
