

#include <limits.h>
#include <math.h>
#include <float.h>
#include <string.h>

// ofx
#include "ofxCore.h"
#include "ofxProperty.h"
#include "ofxMultiThread.h"
#include "ofxMemory.h"

#include "ofxhHost.h"

typedef OfxPlugin* (*OfxGetPluginType)(int);

namespace OFX {
  
  namespace Host {

    ////////////////////////////////////////////////////////////////////////////////
    /// simple memory suite 
    namespace Memory {
      static OfxStatus memoryAlloc(void */*handle*/, size_t bytes, void **data)
      {
        *data = malloc(bytes);
        if (*data) {
          return kOfxStatOK;
        } else {
          return kOfxStatErrMemory;
        }
      }
      
      static OfxStatus memoryFree(void *data)
      {
        free(data);
        return kOfxStatOK;
      }
      
      static const struct OfxMemorySuiteV1 gMallocSuite = {
        memoryAlloc,
        memoryFree
      };
    }

  }

}

namespace OFX {
  namespace Host {
    /// our own internal property for storing away our private pointer to our host descriptor
#define kOfxHostSupportHostPointer "sf.openfx.net.OfxHostSupportHostPointer"

    static const Property::PropSpec hostStuffs[] = {
      { kOfxPropAPIVersion, Property::eInt, 0, false, "" },
      { kOfxPropType, Property::eString, 1, false, "Host" },
      { kOfxPropName, Property::eString, 1, false, "UNKNOWN" },
      { kOfxPropLabel, Property::eString, 1, false, "UNKNOWN" },
      { kOfxPropVersion, Property::eInt, 0, false, "0" },
      { kOfxPropVersionLabel, Property::eString, 1, false, "" },
      { kOfxHostSupportHostPointer,    Property::ePointer,    0,    false,    NULL },
      Property::propSpecEnd
    };    

    static const void *fetchSuite(OfxPropertySetHandle hostProps, const char *suiteName, int suiteVersion)
    {      
      Property::Set* properties = reinterpret_cast<Property::Set*>(hostProps);
      
      Host* host = (Host*)properties->getPointerProperty(kOfxHostSupportHostPointer);
      
      if(host)
        return host->fetchSuite(suiteName,suiteVersion);
      else
        return 0;
    }

    // Base Host
    Host::Host() : _properties(hostStuffs) 
    {
      _host.host = _properties.getHandle();
      _host.fetchSuite = OFX::Host::fetchSuite;

      // record the host descriptor in the propert set
      _properties.setPointerProperty(kOfxHostSupportHostPointer,this);
    }

    OfxHost *Host::getHandle() {
      return &_host;
    }

    OfxStatus Host::message(const char* type,
                            const char* id,
                            const char* format,
                            ...) {
      try {
        OfxStatus stat;
        va_list args;
        va_start(args,format);
        stat = vmessage(type,id,format,args);
        va_end(args);
        return stat;
      } catch (...) {
        return kOfxStatFailed;
      }
    }

    const void *Host::fetchSuite(const char *suiteName, int suiteVersion)
    {
      if (strcmp(suiteName, kOfxPropertySuite)==0  && suiteVersion == 1) {
        return Property::GetSuite(suiteVersion);
      }
      else if (strcmp(suiteName, kOfxMemorySuite)==0 && suiteVersion == 1) {
        return (void*)&Memory::gMallocSuite;
      }  
    
      ///printf("fetchSuite failed with host = %p, name = %s, version = %i\n", this, suiteName, suiteVersion);
      return NULL;
    }

  } // Host

} // OFX 
