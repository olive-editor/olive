

#ifndef _ofxsMemory_H_
#define _ofxsMemory_H_
// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

/** @file This file contains core code that wraps the ofx memory allocator with C++ functions.

This file only holds code that is visible to a plugin implementation, and so hides much
of the direct OFX objects and any library side only functions.
*/

/** @brief The core 'OFX Support' namespace, used by plugin implementations. All code for these are defined in the common support libraries.
*/
namespace OFX {

  // forward declaration of class
  class ImageEffect;

  /** @brief Namespace for general purpose memory allocation */
  namespace Memory {
    /** @brief Allocate memory.

    \arg \e nBytes        - the number of bytes to allocate
    \arg \e handle	      - effect instance to assosciate with this memory allocation, or NULL

    This function has the host allocate memory using it's own memory resources
    and returns that to the plugin. This memory is distinct to any image memory allocation.

    Succeeds or throws std::bad_alloc
    */   
    void *allocate(size_t nBytes,
      ImageEffect *handle = 0);

    /** @brief release memory

    \arg \e ptr	      - pointer previously returned by OFX::Memory::allocate
    */
    void free(void *ptr) noexcept;
  };

};

#endif
