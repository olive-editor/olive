
#ifndef OFX_HOST_H
#define OFX_HOST_H

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

#include <map>
#include <string>
#include <cstdarg>

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxTimeLine.h"
#include "ofxhPropertySuite.h"

namespace OFX {

  namespace Host {

    /// a plugin what we use
    class Plugin;
   
    /// a param descriptor 
    namespace Param {
      class Descriptor;
    }
    
    /// Base class for all objects passed to a plugin by the 'setHost' function 
    /// passed back by any plug-in.
    class Host {
    protected :
      OfxHost       _host;
      Property::Set _properties;

    public:
      Host();
      virtual ~Host() {}

      
      /// get the props on this host
      Property::Set &getProperties() {return _properties; }

      /// fetch a suite
      /// The base class returns the following suites
      ///    PropertySuite
      ///    MemorySuite
      virtual const void *fetchSuite(const char *suiteName, int suiteVersion);
      
      /// get the C API handle that is passed across the API to represent this host
      OfxHost *getHandle();

      /// override this to handle do post-construction initialisation on a Param::Descriptor
      virtual void initParamDescriptor(Param::Descriptor *) { }

      /// is my magic number valid?
      bool verifyMagic() { return true; }

      /// message (called when an exception occurs, calls vmessage)
      OfxStatus message(const char* type,
                        const char* id,
                        const char* format,
                        ...);

      /// vmessage
      virtual OfxStatus vmessage(const char* type,
                                 const char* id,
                                 const char* format,
                                 va_list args) = 0;

      /// setPersistentMessage
      virtual OfxStatus setPersistentMessage(const char* type,
                                             const char* id,
                                             const char* format,
                                             va_list args) = 0;
      /// clearPersistentMessage
      virtual OfxStatus clearPersistentMessage() = 0;
    };
    
  }
}

#endif

