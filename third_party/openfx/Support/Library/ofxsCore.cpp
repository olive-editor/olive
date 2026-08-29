

#include "ofxsSupportPrivate.h"
#ifdef DEBUG
#include <iostream>
#if defined(__APPLE__) || defined(linux)
#include <execinfo.h>
#include <stdlib.h>
#endif
#endif

#include "ofxsMemory.h"

namespace OFX {
  /** @brief Throws an @ref OFX::Exception depending on the status flag passed in */
  void throwSuiteStatusException(OfxStatus stat)
  {
    switch (stat) 
    {
    case kOfxStatOK :
    case kOfxStatReplyYes :
    case kOfxStatReplyNo :
    case kOfxStatReplyDefault :
      break;

    case kOfxStatErrMemory :
      throw std::bad_alloc();

    default :
#    ifdef DEBUG
      std::cout << "Threw suite exception!" << std::endl;
#     if defined(__APPLE__) || defined(linux)
      void* callstack[128];
      int i, frames = backtrace(callstack, 128);
      char** strs = backtrace_symbols(callstack, frames);
      for (i = 0; i < frames; ++i) {
          std::cout << strs[i] << std::endl;
      }
      free(strs);
#     endif
#    endif
      throw OFX::Exception::Suite(stat);
    }
  }

  void throwHostMissingSuiteException(std::string name)
  {
#  ifdef DEBUG
    std::cout << "Threw suite exception! Host missing '" << name << "' suite." << std::endl;
#   if defined(__APPLE__) || defined(linux)
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        std::cout << strs[i] << std::endl;
    }
    free(strs);
#   endif
#  endif
    throw OFX::Exception::Suite(kOfxStatErrUnsupported);
  }


  /** @brief maps status to a string */
  const char* mapStatusToString(OfxStatus stat)
  {
    switch(stat) 
    {    
    case kOfxStatOK             : return "kOfxStatOK";
    case kOfxStatFailed         : return "kOfxStatFailed";
    case kOfxStatErrFatal       : return "kOfxStatErrFatal";
    case kOfxStatErrUnknown     : return "kOfxStatErrUnknown";
    case kOfxStatErrMissingHostFeature : return "kOfxStatErrMissingHostFeature";
    case kOfxStatErrUnsupported : return "kOfxStatErrUnsupported";
    case kOfxStatErrExists      : return "kOfxStatErrExists";
    case kOfxStatErrFormat      : return "kOfxStatErrFormat";
    case kOfxStatErrMemory      : return "kOfxStatErrMemory";
    case kOfxStatErrBadHandle   : return "kOfxStatErrBadHandle";
    case kOfxStatErrBadIndex    : return "kOfxStatErrBadIndex";
    case kOfxStatErrValue       : return "kOfxStatErrValue";
    case kOfxStatReplyYes       : return "kOfxStatReplyYes";
    case kOfxStatReplyNo        : return "kOfxStatReplyNo";
    case kOfxStatReplyDefault   : return "kOfxStatReplyDefault";
    case kOfxStatErrImageFormat : return "kOfxStatErrImageFormat";
    }
    return "UNKNOWN STATUS CODE";
  }


  /** @brief namespace for memory allocation that is done via wrapping the ofx memory suite */
  namespace Memory {
    /** @brief allocate n bytes, returns a pointer to it */
    void *allocate(size_t nBytes, ImageEffect *effect)
    {
      void *data = 0;
      OfxStatus stat = OFX::Private::gMemorySuite->memoryAlloc((void *)(effect ? effect->getHandle() : 0), nBytes, &data);
      if(stat != kOfxStatOK)
        throw std::bad_alloc();
      return data;
    }

    /** @brief free n previously allocated memory */
    void free(void *ptr) throw()
    {
      if(ptr)
        // note we are ignore errors, this could be bad, but we don't throw on a destruction
        OFX::Private::gMemorySuite->memoryFree(ptr);            
    }

  };

}; // namespace OFX

