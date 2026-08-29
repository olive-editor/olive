

#ifndef _ofxMemory_h_
#define _ofxMemory_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


#ifdef __cplusplus
extern "C" {
#endif

#define kOfxMemorySuite "OfxMemorySuite"

/** @brief The OFX suite that implements general purpose memory management.

Use this suite for ordinary memory management functions, where you would normally use malloc/free or new/delete on ordinary objects.

For images, you should use the memory allocation functions in the image effect suite, as many hosts have specific image memory pools.

\note C++ plugin developers will need to redefine new and delete as skins ontop of this suite.
 */
typedef struct OfxMemorySuiteV1 {
  /** @brief Allocate memory.
      
  \arg \c handle	- effect instance to assosciate with this memory allocation, or NULL.
  \arg \c nBytes        number of bytes to allocate
  \arg \c allocatedData pointer to the return value. Allocated memory will be alligned for any use.

  This function has the host allocate memory using its own memory resources
  and returns that to the plugin.

  @returns
  - ::kOfxStatOK the memory was sucessfully allocated
  - ::kOfxStatErrMemory the request could not be met and no memory was allocated

  */   
  OfxStatus (*memoryAlloc)(void *handle, 
			   size_t nBytes,
			   void **allocatedData);
	
  /** @brief Frees memory.
      
  \arg \c allocatedData pointer to memory previously returned by OfxMemorySuiteV1::memoryAlloc

  This function frees any memory that was previously allocated via OfxMemorySuiteV1::memoryAlloc.

  @returns
  - ::kOfxStatOK the memory was sucessfully freed
  - ::kOfxStatErrBadHandle \e allocatedData was not a valid pointer returned by OfxMemorySuiteV1::memoryAlloc

  */   
  OfxStatus (*memoryFree)(void *allocatedData);
 } OfxMemorySuiteV1;


/** @file ofxMemory.h
    This file contains the API for general purpose memory allocation from a host.
*/



#ifdef __cplusplus
}
#endif

#endif
