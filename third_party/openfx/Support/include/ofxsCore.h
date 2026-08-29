

#ifndef _ofxsCore_H_
#define _ofxsCore_H_
// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


/** @mainpage OFX Support Library

@section mainpageIntro Introduction

This support library skins the raw OFX C API with a set of C++ classes and functions that makes it easier to understand and write plug-ins to the API. Look at the examples to see how it is done.

<HR>

@section fifteenLineGuide Fifteen Line Plugin Writing Guide

- work from the examples
- you need to write the following functions....
- void OFX::Plugin::getPluginID(OFX::PluginID &id)
- gives the unique name and version numbers of the plug-in
- void OFX::Plugin::loadAction(void)
- called after the plug-in is first loaded, and before any instance has been made,
- void OFX::Plugin::unloadAction(void)
- called before the plug-in is unloaded, and all instances have been destroyed,
- void OFX::Plugin::describe(OFX::ImageEffectDescriptor &desc)
- called to describe the plugin to the host
- void OFX::Plugin::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context)
- called to describe the plugin to the host for a context reported in OFX::Plugin::describe
-  OFX::ImageEffect * OFX::Plugin::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
- called when a new instance of a plug-in needs to be created. You need to derive a class from ImageEffect, new it and return it.

The OFX::ImageEffect class has a set of members you can override to do various things, like rendering an effect. Again, look at the examples.

<HR>

@section license Copyright and License

The library is Copyright OpenFX and contributors to the OpenFX project. and was
written by Bruno Nicoletti (bruno@thefoundry.co.uk).

It has been released under the GNU Lesser General Public License, see the
top of any source file for details.

*/

/** @file This file contains core code that wraps OFX 'objects' with C++ classes.

This file only holds code that is visible to a plugin implementation, and so hides much
of the direct OFX objects and any library side only functions.
*/

#ifdef _MSC_VER
#pragma warning( disable : 4290 )
#endif

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxInteract.h"
#include "ofxKeySyms.h"
#include "ofxMemory.h"
#include "ofxMessage.h"
#include "ofxMultiThread.h"
#include "ofxParam.h"
#include "ofxProperty.h"
#include "ofxPixels.h"

#include <assert.h>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <exception>
#include <stdexcept>
#include <sstream>

#ifdef OFX_CLIENT_EXCEPTION_HEADER
#include OFX_CLIENT_EXCEPTION_HEADER
#endif

/** @brief Defines an integer 3D point

Should migrate this to the ofxCore.h in a v1.1
*/
struct Ofx3DPointI {
  int x, y, z;
};

/** @brief Defines a double precision 3D point

Should migrate this to the ofxCore.h in a v1.1
*/
struct Ofx3DPointD {
  double x, y, z;
};

/** @brief Nasty macro used to define empty protected copy ctors and assign ops */
#define mDeclareProtectedAssignAndCC(CLASS) \
  CLASS &operator=(const CLASS &) {assert(false); return *this;}	\
  CLASS(const CLASS &) {assert(false); }

/** @brief The core 'OFX Support' namespace, used by plugin implementations. All code for these are defined in the common support libraries.
*/
namespace OFX {
  /** forward class declarations */
  class PropertySet;

  /** @brief Enumerates the different types a property can be */
  enum PropertyTypeEnum {
    ePointer,
    eInt,
    eString,
    eDouble
  };

  /** @brief Enumerates the reasons a plug-in instance may have had one of its values changed */
  enum InstanceChangeReason {
    eChangeUserEdit,    /**< @brief A user actively editted something in the plugin, eg: changed the value of an integer param on an interface */
    eChangePluginEdit,  /**< @brief The plugin's own code changed something in the instance, eg: a callback on on param settting the value of another */
    eChangeTime         /**< @brief The current value of a parameter has changed because the param animates and the current time has changed */
  };

  /** @brief maps a status to a string for debugging purposes, note a c-str for printf */
  const char * mapStatusToString(OfxStatus stat);

  /** @brief namespace for OFX support lib exceptions, all derive from std::exception, calling it */
  namespace Exception {

    /** @brief thrown when a suite returns a dud status code
    */
    class Suite : public std::exception {
    protected :
      OfxStatus _status;
    public :
      Suite(OfxStatus s) : _status(s) {}
      OfxStatus status(void) const {return _status;}
      operator OfxStatus() const {return _status;}

      /** @brief reimplemented from std::exception */
      virtual const char * what () const noexcept {return mapStatusToString(_status);}

    };

    /** @brief Exception indicating that a host doesn't know about a property that is should do */
    class PropertyUnknownToHost : public std::exception {
    protected :
      std::string _what;
    public :
      PropertyUnknownToHost(const char *what) : _what(what) {}
      virtual ~PropertyUnknownToHost() noexcept {}

      /** @brief reimplemented from std::exception */
      virtual const char * what () const noexcept
      {
        return _what.c_str();
      }
    };

    /** @brief exception indicating that the host thinks a property has an illegal value */
    class PropertyValueIllegalToHost : public std::exception {
    protected :
      std::string _what;
    public :
      PropertyValueIllegalToHost(const char *what) : _what(what) {}
      virtual ~PropertyValueIllegalToHost() noexcept {}

      /** @brief reimplemented from std::exception */
      virtual const char * what () const noexcept
      {
        return _what.c_str();
      }
    };

    /** @brief exception indicating a request for a named thing exists (eg: a param), but is of the wrong type, should never make it back to the main entry
    indicates a logical error in the code. Asserts are raised in debug code in these situations.
    */
    class TypeRequest : public std::exception {
    protected :
      std::string _what;
    public :
      TypeRequest(const char *what) : _what(what) {}
      virtual ~TypeRequest() noexcept {}

      /** @brief reimplemented from std::exception */
      virtual const char * what () const noexcept
      {
        return _what.c_str();
      }
    };

    ////////////////////////////////////////////////////////////////////////////////
    // These exceptions are to be thrown by the plugin if it hits a problem, the
    // code managing the main entry will trap the exception and return a suitable
    // status code to the host.

    /** @brief exception indicating a required host feature is missing */
    class HostInadequate : public std::exception {
    protected :
      std::string _what;
    public :
      HostInadequate(const char *what) : _what(what) {}
      virtual ~HostInadequate() noexcept {}

      /** @brief reimplemented from std::exception */
      virtual const char * what () const noexcept
      {
        return _what.c_str();
      }
    };

  }; // end of Exception namespace

  /** @brief Throws an @ref OFX::Exception::Suite depending on the status flag passed in */
  void
    throwSuiteStatusException(OfxStatus stat);

  void
    throwHostMissingSuiteException(std::string name);

  /** @brief This struct is used to return an identifier for the plugin by the function @ref OFX:Plugin::getPlugin.
  The members correspond to those in the OfxPlugin struct defined in ofxCore.h.
  */

  class ImageEffectDescriptor;
  class ImageEffect;

  /** @brief This class wraps up an OFX property set */
  class PropertySet {
  protected :
    /** @brief The raw property handle */
    OfxPropertySetHandle _propHandle;

    /** @brief Class static, whether we are logging each property action */
    static int _gPropLogging;

    /** @brief Do not throw an exception if a host returns 'unsupported' when setting a property */
    static bool _gThrowOnUnsupported;

  public :
    /** @brief turns on logging of property access functions */
    static void propEnableLogging(void)  {++_gPropLogging;}

    /** @brief turns off logging of property access functions */
    static void propDisableLogging(void) {--_gPropLogging;}

    /** @brief Do we throw an exception if a host returns 'unsupported' when setting a property. Default is true */
    static void setThrowOnUnsupportedProperties(bool v) {_gThrowOnUnsupported = v;}

    /** @brief Do we throw an exception if a host returns 'unsupported' when setting a property. Default is true */
    static bool getThrowOnUnsupportedProperties(void) {return _gThrowOnUnsupported;}

    /** @brief construct a property set */
    PropertySet(OfxPropertySetHandle h = 0) : _propHandle(h) {}
    virtual ~PropertySet();

    /** @brief set the handle to use for this set */
    void propSetHandle(OfxPropertySetHandle h) { _propHandle = h;}

    /** @brief return the handle for this property set */
    OfxPropertySetHandle propSetHandle(void) const {return _propHandle;}

    int  propGetDimension(const char* property, bool throwOnFailure = true) const;
    void propReset(const char* property);

    // set single values
    void propSetPointer(const char* property, void *value, int idx, bool throwOnFailure = true);
    void propSetString(const char* property, const std::string &value, int idx, bool throwOnFailure = true);
    void propSetDouble(const char* property, double value, int idx, bool throwOnFailure = true);
    void propSetInt(const char* property, int value, int idx, bool throwOnFailure = true);

    // set multiple values
    void propSetDoubleN(const char* property, const double *value, int count, bool throwOnFailure = true);

    void propSetPointer(const char* property, void *value, bool throwOnFailure = true)
    {propSetPointer(property, value, 0, throwOnFailure);}

    void propSetString(const char* property, const std::string &value, bool throwOnFailure = true)
    {propSetString(property, value, 0, throwOnFailure);}

    void propSetDouble(const char* property, double value, bool throwOnFailure = true)
    {propSetDouble(property, value, 0, throwOnFailure);}

    void propSetInt(const char* property, int value, bool throwOnFailure = true)
    {propSetInt(property, value, 0, throwOnFailure);}


    /// get a pointer property
    void       *propGetPointer(const char* property, int idx, bool throwOnFailure = true) const;

    /// get a string property
    std::string propGetString(const char* property, int idx, bool throwOnFailure = true) const;
    /// get a double property
    double      propGetDouble(const char* property, int idx, bool throwOnFailure = true) const;

    /// get an int property
    int propGetInt(const char* property, int idx, bool throwOnFailure = true) const;

    /// get a pointer property with index 0
    void* propGetPointer(const char* property, bool throwOnFailure = true) const
    {
      return propGetPointer(property, 0, throwOnFailure);
    }

    /// get a string property with index 0
    std::string propGetString(const char* property, bool throwOnFailure = true) const
    {
      return propGetString(property, 0, throwOnFailure);
    }

    /// get a double property with index 0
    double propGetDouble(const char* property, bool throwOnFailure = true) const
    {
      return propGetDouble(property, 0, throwOnFailure);
    }

    /// get an int property with index 0
    int propGetInt(const char* property, bool throwOnFailure = true) const
    {
      return propGetInt(property, 0, throwOnFailure);
    }

    std::list<std::string> propGetNString(const char* property, bool throwOnFailure = true) const;

  };

  // forward decl of the image effect
  class ImageEffect;
};

// undeclare the protected assign and CC macro
#undef mDeclareProtectedAssignAndCC

#endif
