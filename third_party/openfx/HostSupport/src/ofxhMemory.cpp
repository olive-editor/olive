

// ofx host

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"

// ofx host
#include "ofxhMemory.h"

namespace OFX {

  namespace Host {

    namespace Memory {

      Instance::Instance() : _ptr(0), _locked(0) {}

      Instance::~Instance() {
        delete [] _ptr;
      }

      bool Instance::alloc(size_t nBytes) {
        if(!_locked){
          if(_ptr)
            freeMem();
          _ptr = new char[nBytes];
          return true;
        }
        else
          return false;
      }

      OfxImageMemoryHandle Instance::getHandle(){
        return (OfxImageMemoryHandle)this;
      }

      void Instance::freeMem(){
        delete [] _ptr;
        _ptr = 0;
        _locked = 0;
      }

      void* Instance::getPtr() {
        return _ptr;
      }

      void Instance::lock() {
        ++_locked;
      }

      void Instance::unlock() {
        if (_locked > 0) {
          --_locked;
        }
      }

    } // Memory

  } // Host

} // OFX

