

/** @file

This file contains headers for classes that are used to validate property sets and make sure they have the right members and default values.

*/

#include "ofxsSupportPrivate.h"
#include <stdarg.h>
#ifdef OFX_SUPPORTS_OPENGLRENDER
#include "ofxGPURender.h"
#endif

/** @brief Null pointer definition */
#define NULLPTR ((void *)(0))

// #define  kOfxsDisableValidation

// disable validation if not a debug build
#ifndef DEBUG
#define  kOfxsDisableValidation
#endif

//#define kOfxsDisableValidation
/** @brief OFX namespace
*/
namespace OFX {

  /** @brief The validation code has its own namespace */
  namespace Validation {

#ifndef kOfxsDisableValidation
    /** @brief Set the vector by getting dimension things specified by ilk from the argp list, used by PropertyDescription ctor */
    static void
      setVectorFromVarArgs(OFX::PropertyTypeEnum ilk,
      int dimension,
      va_list &argp,
      std::vector<ValueHolder> &vec)
    {
      char  *vS;
      int    vI;
      double vD;
      void  *vP;
      for(int i = 0; i < dimension; i++) {
        switch (ilk) 
        {
        case eString :
          vS = va_arg(argp, char *);
          vec.push_back(vS);
          break;

        case eInt :
          vI = va_arg(argp, int);
          vec.push_back(vI);
          break;

        case ePointer :
          vP = va_arg(argp, void *);
          vec.push_back(vP);
          break;

        case eDouble :
          vD = va_arg(argp, double);
          vec.push_back(vD);
          break;
        }
      }
    }


    /** @brief PropertyDescription var-args constructor */
    PropertyDescription::PropertyDescription(const char *name, OFX::PropertyTypeEnum ilk, int dimension, ...)
      : _name(name)
      , _exists(false) // only set when we have validated it
      , _dimension(dimension)
      , _ilk(ilk)
    {      
      // go through the var args to extract defaults to check against and values to set to
      va_list argp;
      va_start(argp, dimension);

      bool going = true;
      while(going) {
        // what is being set ?
        DescriptionTag tag = DescriptionTag(va_arg(argp, int));

        switch (tag) 
        {
        case eDescDefault : // we are setting default values to check against
          setVectorFromVarArgs(ilk, dimension, argp, _defaultValue);
          break;

        case eDescFinished : // we are finished
        default :
          going = false;
          break;
        }
      }

      va_end(argp);
    }


    /** @brief See if the property exists in the containing property set and has the correct dimension */
    void 
      PropertyDescription::validate(bool checkDefaults,
      PropertySet &propSet)
    {
      // see if it exists by fetching the dimension, 

      try {
        int hostDimension = propSet.propGetDimension(_name.c_str());
        _exists = true;  

        if(_dimension != -1) // -1 implies variable dimension
          OFX::Log::error(hostDimension != _dimension, "Host reports property '%s' has dimension %d, it should be %d;", _name.c_str(), hostDimension, _dimension); 
        // check type by getting the first element, the property getting will print any failure messages to the log
        if(hostDimension > 0) {
          switch(_ilk) 
          {
          case OFX::ePointer : { void *vP = propSet.propGetPointer(_name.c_str()); (void)vP; }break;
          case OFX::eInt :     { int vI = propSet.propGetInt(_name.c_str()); (void)vI; } break;
          case OFX::eString  : { std::string vS = propSet.propGetString(_name.c_str()); (void)vS; } break;
          case OFX::eDouble  : { double vD = propSet.propGetDouble(_name.c_str()); (void)vD; } break;
          }
        }

        // check the defaults are OK, if there are any
        int nDefs = (int)_defaultValue.size();
        if(checkDefaults && nDefs > 0) {
          OFX::Log::error(hostDimension != nDefs, "Host reports default dimension of '%s' as %d, which is different to the default dimension size of %d;", 
            _name.c_str(), hostDimension, nDefs);

          int N = hostDimension < nDefs ? hostDimension : nDefs;

          for(int i = 0; i < N; i++) {
            switch(_ilk) 
            {
            case OFX::ePointer : {
              void *vP = propSet.propGetPointer(_name.c_str(), i);
              OFX::Log::error(vP != (void *) _defaultValue[i], "Default value of %s[%d] = %p, it should be %p;",
                _name.c_str(), i, vP, (void *) _defaultValue[i]);
                                 }
                                 break;
            case OFX::eInt : {
              int vI = propSet.propGetInt(_name.c_str(), i);
              OFX::Log::error(vI != (int) _defaultValue[i], "Default value of %s[%d] = %d, it should be %d;",
                _name.c_str(), i, vI, (int) _defaultValue[i]);
                             }
                             break;
            case OFX::eString  : {
              std::string vS = propSet.propGetString(_name.c_str(), i);
              OFX::Log::error(vS != _defaultValue[i].vString, "Default value of %s[%d] = '%s', it should be '%s';",
                _name.c_str(), i, vS.c_str(), _defaultValue[i].vString.c_str());
                                 }
                                 break;
            case OFX::eDouble  : {
              double vD = propSet.propGetDouble(_name.c_str(), i);
              OFX::Log::error(vD != (double) _defaultValue[i], "Default value of %s[%d] = %g, it should be %g;",
                _name.c_str(), i, vD, (double) _defaultValue[i]);
                                 }
                                 break;
            }
          }
        }
      }
      catch (OFX::Exception::Suite &e)
      {
        // just catch it, the error will be reported
        _exists = false;
      }
      catch (OFX::Exception::PropertyUnknownToHost &e)
      {
        // just catch it, the error will be reported
        _exists = false;
      }
      catch (OFX::Exception::PropertyValueIllegalToHost &e)
      {
        // just catch it, the error will be reported
        _exists = false;
      }
    }


    /** @brief This macro is used to short hand passing the var args to @ref OFX::Validation::PropertySetDescription::PropertySetDescription */    
#define mPropDescriptionArg(descs) descs, sizeof(descs)/sizeof(PropertyDescription)

    /** @brief A set of property descriptions, constructor 

    Passed in as a zero terminated pairs of (PropertyDescription *descArray, int nDescriptions)

    */
    PropertySetDescription::PropertySetDescription(const char *setName, ...) // PropertyDescription *v, int nV)
      : _setName(setName)
    {

      // go through the var args to extract defaults to check against and values to set to
      va_list argp;
      va_start(argp, setName);

      while(1) {
        // get a pointer 
        PropertyDescription *descs = (PropertyDescription *) va_arg(argp, PropertyDescription *);

        // have we finished
        if(!descs) break;

        // get the count
        int nDescs = (int) va_arg(argp, int);

        // and set it up
        for(int i = 0; i < nDescs; i++) {
          _descriptions.push_back(descs + i);
        }

      }

      va_end(argp);
    }

    /** @brief destructor */
    PropertySetDescription::~PropertySetDescription()
    {
      int nToDelete  = (int)_deleteThese.size();
      for(int i = 0; i < nToDelete; i++) {
        delete _deleteThese[i];
      }
    }

    /** @brief Add more properties into the property vector */
    void
      PropertySetDescription::addProperty(PropertyDescription *desc,
      bool deleteOnDestruction)
    {
      _descriptions.push_back(desc);
      if(deleteOnDestruction)
        _deleteThese.push_back(desc);
    }

    /** @brief Validate all the properties in the set */
    void
      PropertySetDescription::validate(PropertySet &propSet, 
      bool checkDefaults,
      bool logOrdinaryMessages)
    {
      OFX::Log::print("START validating properties of %s.", _setName.c_str());
      OFX::Log::indent();

      // don't print ordinary messages whilst we are checking them
      if(!logOrdinaryMessages) PropertySet::propDisableLogging();

      // check each property in the description
      int n = (int)_descriptions.size();
      for(int i = 0; i < n; i++) 
        _descriptions[i]->validate(checkDefaults, propSet);

      if(!logOrdinaryMessages) PropertySet::propEnableLogging();

      OFX::Log::outdent();
      OFX::Log::print("STOP property validation of %s.", _setName.c_str());
    }


    /** @brief A list of properties that all hosts must have, and will be validated against. None of these has a default, but they must exist. */
    static PropertyDescription gHostProps[ ] =
    {
      // single dimensional string properties
      PropertyDescription(kOfxPropType,  OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropName,  OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropLabel, OFX::eString, 1, eDescFinished),

      // single dimensional int properties
      PropertyDescription(kOfxImageEffectHostPropIsBackground,           OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsOverlays,           OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsMultiResolution,    OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsTiles,              OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropTemporalClipAccess,         OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsMultipleClipDepths, OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsMultipleClipPARs,   OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSetableFrameRate,           OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSetableFielding,            OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropSupportsStringAnimation,      OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropSupportsCustomInteract,       OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropSupportsChoiceAnimation,      OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropSupportsStrChoiceAnimation,   OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropSupportsBooleanAnimation,     OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropSupportsCustomAnimation,      OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropMaxParameters,                OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamHostPropMaxPages,                     OFX::eInt, 1, eDescFinished),

      // variable multi dimensional string properties
      PropertyDescription(kOfxImageEffectPropSupportedComponents,        OFX::eString, -1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportedContexts,          OFX::eString, -1, eDescFinished),

      // multi dimensional int properities
      PropertyDescription(kOfxParamHostPropPageRowColumnCount,           OFX::eInt, 2, eDescFinished),
    };

    /** @brief the property set for the global host pointer */
    static PropertySetDescription gHostPropSet("Host Property", 
      gHostProps, sizeof(gHostProps)/sizeof(PropertyDescription),
      NULLPTR);


    /** @brief A list of properties to validate the effect descriptor against */
    static PropertyDescription gPluginDescriptorProps[ ] =
    {
      // string props that have no defaults that can be checked against
      PropertyDescription(kOfxPropLabel,                      OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropShortLabel,                 OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropLongLabel,                  OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPluginPropGrouping,  OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPluginPropFilePath,             OFX::eString, 1, eDescFinished),

      // string props with defaults that can be checked against
      PropertyDescription(kOfxPropType,                             OFX::eString, 1, eDescDefault, kOfxTypeImageEffect, eDescFinished),
      PropertyDescription(kOfxImageEffectPluginRenderThreadSafety,  OFX::eString, 1, eDescDefault, kOfxImageEffectRenderFullySafe, eDescFinished),

      // int props with defaults that can be checked against
      PropertyDescription(kOfxImageEffectPluginPropSingleInstance,         OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageEffectPluginPropHostFrameThreading,     OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsMultiResolution,      OFX::eInt, 1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsTiles,                OFX::eInt, 1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropTemporalClipAccess,           OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageEffectPluginPropFieldRenderTwiceAlways, OFX::eInt, 1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsMultipleClipDepths,   OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsMultipleClipPARs,     OFX::eInt, 1, eDescDefault, 0, eDescFinished),

      // Pointer props with defaults that can be checked against
      PropertyDescription(kOfxImageEffectPluginPropOverlayInteractV1,      OFX::ePointer, 1, eDescDefault, (void *)(0), eDescFinished),

      // string props that have variable dimension, and can't be checked against for defaults
      PropertyDescription(kOfxImageEffectPropSupportedContexts,  OFX::eString, -1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportedPixelDepths,  OFX::eString, -1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropClipPreferencesSlaveParam,  OFX::eString, -1, eDescFinished),
    };

    /** @brief the property set for the global plugin descriptor */
    static PropertySetDescription gPluginDescriptorPropSet("Plugin Descriptor", 
      gPluginDescriptorProps, sizeof(gPluginDescriptorProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief A list of properties to validate the plugin instance */
    static PropertyDescription gPluginInstanceProps[ ] =
    {
      // string props with defaults that can be checked against
      PropertyDescription(kOfxPropType,                                OFX::eString,  1, eDescDefault, kOfxTypeImageEffectInstance, eDescFinished),

      // int props with defaults that can be checked against
      PropertyDescription(kOfxImageEffectInstancePropSequentialRender, OFX::eInt,     1, eDescDefault, 0, eDescFinished),

      // Pointer props with defaults that can be checked against
      PropertyDescription(kOfxPropInstanceData,                        OFX::ePointer, 1, eDescDefault, (void *)(0), eDescFinished),
      PropertyDescription(kOfxImageEffectPropPluginHandle,             OFX::ePointer, 1, eDescFinished),

      // string props that have no defaults that can be checked against
      PropertyDescription(kOfxImageEffectPropContext,                  OFX::eString,  1, eDescFinished),

      // int props with not defaults that can be checked against
      PropertyDescription(kOfxPropIsInteractive,                       OFX::eInt,     1, eDescFinished),

      // double props that can't be checked against for defaults
      PropertyDescription(kOfxImageEffectPropProjectSize,              OFX::eDouble,  2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropProjectExtent,            OFX::eDouble,  2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropProjectOffset,            OFX::eDouble,  2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropProjectPixelAspectRatio,  OFX::eDouble,  1, eDescFinished),
      PropertyDescription(kOfxImageEffectInstancePropEffectDuration,   OFX::eDouble,  1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropFrameRate,                OFX::eDouble,  1, eDescFinished),
    };

    /** @brief the property set for a plugin instance */
    static PropertySetDescription gPluginInstancePropSet("Plugin Instance", 
      gPluginInstanceProps, sizeof(gPluginInstanceProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief A list of properties to validate a clip descriptor */
    static PropertyDescription gClipDescriptorProps[ ] =
    {
      // string props with checkable defaults
      PropertyDescription(kOfxPropType,                           OFX::eString, 1, eDescDefault, kOfxTypeClip, eDescFinished),
      PropertyDescription(kOfxImageClipPropFieldExtraction,       OFX::eString, 1, eDescDefault, kOfxImageFieldDoubled, eDescFinished),

      // string props with no checkable defaults
      PropertyDescription(kOfxImageEffectPropSupportedComponents, OFX::eString,-1, eDescFinished),
      PropertyDescription(kOfxPropName,                           OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropLabel,                          OFX::eString, 1,  eDescFinished),
      PropertyDescription(kOfxPropShortLabel,                     OFX::eString, 1,  eDescFinished),
      PropertyDescription(kOfxPropLongLabel,                      OFX::eString, 1, eDescFinished),

      // int props with checkable defaults
      PropertyDescription(kOfxImageEffectPropTemporalClipAccess,  OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageClipPropOptional,              OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageClipPropIsMask,                OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsTiles,       OFX::eInt, 1, eDescDefault, 1, eDescFinished),
    };

    /** @brief the property set for a clip descriptor */
    static PropertySetDescription gClipDescriptorPropSet("Clip Descriptor", 
      gClipDescriptorProps, sizeof(gClipDescriptorProps)/sizeof(PropertyDescription),
      NULLPTR);


    /** @brief A list of properties to validate a clip instance */
    static PropertyDescription gClipInstanceProps[ ] =
    {
      // we can only validate this one against a fixed default
      PropertyDescription(kOfxPropType,                           OFX::eString, 1, eDescDefault, kOfxTypeClip, eDescFinished),

      // the rest are set by the plugin during description or by the host
      PropertyDescription(kOfxPropName,                           OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropLabel,                          OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropShortLabel,                     OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropLongLabel,                      OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportedComponents, OFX::eString,-1, eDescFinished),
      PropertyDescription(kOfxImageClipPropFieldExtraction,       OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropPixelDepth,          OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropComponents,          OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropUnmappedPixelDepth,    OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropUnmappedComponents,    OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropPreMultiplication,   OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropFieldOrder,            OFX::eString, 1, eDescFinished),

      // int props
      PropertyDescription(kOfxImageEffectPropTemporalClipAccess, OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropOptional,             OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropIsMask,               OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropSupportsTiles,      OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropConnected,            OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropContinuousSamples,    OFX::eInt, 1, eDescFinished),

      // double props
      PropertyDescription(kOfxImagePropPixelAspectRatio,         OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropFrameRate,          OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropFrameRange,         OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropUnmappedFrameRate,  OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropUnmappedFrameRange, OFX::eDouble, 2, eDescFinished),
    };

    /** @brief the property set for a clip instance */
    static PropertySetDescription gClipInstancePropSet("Clip Instance", gClipInstanceProps, sizeof(gClipInstanceProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief List of properties to validate an image or texture instance */
    static PropertyDescription gImageBaseInstanceProps[ ] =
    {
      // this is the only property with a checkable default
      PropertyDescription(kOfxPropType,                         OFX::eString, 1, eDescDefault, kOfxTypeImage, eDescFinished),

      // all other properties are set by the host
      PropertyDescription(kOfxImageEffectPropPixelDepth,        OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropComponents,        OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropPreMultiplication, OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImagePropField,                   OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImagePropUniqueIdentifier,        OFX::eString, 1, eDescFinished),

      // double props
      PropertyDescription(kOfxImageEffectPropRenderScale,       OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxImagePropPixelAspectRatio,        OFX::eDouble, 1, eDescFinished),

      // pointer props
      PropertyDescription(kOfxImagePropData,                    OFX::ePointer, 1, eDescFinished),

      // int props
      PropertyDescription(kOfxImagePropBounds,                  OFX::eInt, 4, eDescFinished),
      PropertyDescription(kOfxImagePropRegionOfDefinition,      OFX::eInt, 4, eDescFinished),
      PropertyDescription(kOfxImagePropRowBytes,                OFX::eInt, 1, eDescFinished),
    };

    /** @brief the property set for an image instance */
    static PropertySetDescription gImageBaseInstancePropSet("Image or Texture Instance",
      gImageBaseInstanceProps, sizeof(gImageBaseInstanceProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief List of properties to validate an image or texture instance */
    static PropertyDescription gImageInstanceProps[ ] =
    {
      // pointer props
      PropertyDescription(kOfxImagePropData,                    OFX::ePointer, 1, eDescFinished),
    };

    /** @brief the property set for an image instance */
    static PropertySetDescription gImageInstancePropSet("Image Instance",
      gImageInstanceProps, sizeof(gImageInstanceProps)/sizeof(PropertyDescription),
      NULLPTR);

#ifdef OFX_SUPPORTS_OPENGLRENDER
    /** @brief List of properties to validate an image or texture instance */
    static PropertyDescription gTextureInstanceProps[ ] =
    {
      // pointer props
      PropertyDescription(kOfxImageEffectPropOpenGLTextureIndex, OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropOpenGLTextureTarget, OFX::eInt, 1, eDescFinished),
    };

    /** @brief the property set for an image instance */
    static PropertySetDescription gTextureInstancePropSet("Texture Instance",
      gTextureInstanceProps, sizeof(gTextureInstanceProps)/sizeof(PropertyDescription),
      NULLPTR);
#endif

    ////////////////////////////////////////////////////////////////////////////////
    // Action in/out args properties 
    ////////////////////////////////////////////////////////////////////////////////

    /** @brief kOfxImageEffectActionDescribeInContext actions's inargs properties */
    static PropertyDescription gDescribeInContextActionInArgProps[ ] =
    {
      PropertyDescription(kOfxImageEffectPropContext,   OFX::eString, 1, eDescFinished),
    };

    /** @brief the property set for describe in context action  */
    static PropertySetDescription gDescribeInContextActionInArgPropSet(kOfxImageEffectActionDescribeInContext " in argument", 
      gDescribeInContextActionInArgProps, sizeof(gDescribeInContextActionInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionRender action's inargs properties */
    static PropertyDescription gRenderActionInArgProps[ ] =
    {
      PropertyDescription(kOfxPropTime,                     OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderScale,   OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderWindow,  OFX::eInt,    4, eDescFinished),
      PropertyDescription(kOfxImageEffectPropFieldToRender, OFX::eString, 1, eDescFinished),
      // The following appeared in OFX 1.2, and are thus not mandatory
      //PropertyDescription(kOfxImageEffectPropSequentialRenderStatus,  OFX::eInt,    1, eDescFinished),
      //PropertyDescription(kOfxImageEffectPropInteractiveRenderStatus, OFX::eInt,    1, eDescFinished),
      // The following appeared in OFX 1.4,  and is thus not mandatory
      //PropertyDescription(kOfxImageEffectPropRenderQualityDraft, OFX::eInt,    1, eDescFinished),
    };

    /** @brief kOfxImageEffectActionRender property set */
    static PropertySetDescription gRenderActionInArgPropSet(kOfxImageEffectActionRender " in argument", 
      gRenderActionInArgProps, sizeof(gRenderActionInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionBeginSequenceRender action's inargs properties */
    static PropertyDescription gBeginSequenceRenderActionInArgProps[ ] =
    {
      PropertyDescription(kOfxImageEffectPropFrameRange,  OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropFrameStep,   OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderScale, OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxPropIsInteractive,          OFX::eInt, 1, eDescFinished),
      // The following appeared in OFX 1.2, and are thus not mandatory
      //PropertyDescription(kOfxImageEffectPropSequentialRenderStatus,  OFX::eInt,    1, eDescFinished),
      //PropertyDescription(kOfxImageEffectPropInteractiveRenderStatus, OFX::eInt,    1, eDescFinished),
    };

    /** @brief kOfxImageEffectActionBeginSequenceRender property set */
    static PropertySetDescription gBeginSequenceRenderActionInArgPropSet(kOfxImageEffectActionBeginSequenceRender " in argument", 
      gBeginSequenceRenderActionInArgProps, sizeof(gBeginSequenceRenderActionInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionEndSequenceRender action's inargs properties */
    static PropertyDescription gEndSequenceRenderActionInArgProps[ ] =
    {
      PropertyDescription(kOfxImageEffectPropFrameRange,  OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropFrameStep,   OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderScale, OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxPropIsInteractive,          OFX::eInt, 1, eDescFinished),
      // The following appeared in OFX 1.2, and are thus not mandatory
      //PropertyDescription(kOfxImageEffectPropSequentialRenderStatus,  OFX::eInt,    1, eDescFinished),
      //PropertyDescription(kOfxImageEffectPropInteractiveRenderStatus, OFX::eInt,    1, eDescFinished),
    };

    /** @brief kOfxImageEffectActionEndSequenceRender property set */
    static PropertySetDescription gEndSequenceRenderActionInArgPropSet(kOfxImageEffectActionEndSequenceRender " in argument", 
      gEndSequenceRenderActionInArgProps, sizeof(gEndSequenceRenderActionInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionIsIdentity action's inargs properties */
    static PropertyDescription gIsIdentityActionInArgProps[ ] =
    {
      PropertyDescription(kOfxPropTime,                     OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderScale,   OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderWindow,  OFX::eInt,    4, eDescFinished),
      PropertyDescription(kOfxImageEffectPropFieldToRender, OFX::eString, 1, eDescFinished),
    };

    /** @brief kOfxImageEffectActionIsIdentity property set */
    static PropertySetDescription gIsIdentityActionInArgPropSet(kOfxImageEffectActionIsIdentity " in argument", 
      gIsIdentityActionInArgProps, sizeof(gIsIdentityActionInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionIsIdentity action's outargs properties */
    static PropertyDescription gIsIdentityActionOutArgProps[ ] =
    {
      PropertyDescription(kOfxPropTime, OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxPropName, OFX::eString, 1, eDescFinished),
    };

    /** @brief kOfxImageEffectActionIsIdentity property set */
    static PropertySetDescription gIsIdentityActionOutArgPropSet(kOfxImageEffectActionIsIdentity " out argument", 
      gIsIdentityActionOutArgProps, sizeof(gIsIdentityActionOutArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionGetRegionOfDefinition action's inargs properties */
    static PropertyDescription gGetRegionOfDefinitionInArgProps[ ] =
    {
      PropertyDescription(kOfxPropTime,                     OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderScale,   OFX::eDouble, 2, eDescFinished),
    };

    /** @brief kOfxImageEffectActionGetRegionOfDefinition property set */
    static PropertySetDescription gGetRegionOfDefinitionInArgPropSet(kOfxImageEffectActionGetRegionOfDefinition " in argument", 
      gGetRegionOfDefinitionInArgProps, sizeof(gGetRegionOfDefinitionInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionGetRegionOfDefinition action's outargs properties */
    static PropertyDescription gGetRegionOfDefinitionOutArgProps[ ] =
    {
      PropertyDescription(kOfxImageEffectPropRegionOfDefinition,   OFX::eDouble, 4, eDescFinished),
    };

    /** @brief kOfxImageEffectActionGetRegionOfDefinition  property set */
    static PropertySetDescription gGetRegionOfDefinitionOutArgPropSet(kOfxImageEffectActionGetRegionOfDefinition " out argument",
      gGetRegionOfDefinitionOutArgProps, sizeof(gGetRegionOfDefinitionOutArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionGetRegionsOfInterest action's inargs properties */
    static PropertyDescription gGetRegionOfInterestInArgProps[ ] =
    {
      PropertyDescription(kOfxPropTime,                          OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderScale,        OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRegionOfInterest,   OFX::eDouble, 4, eDescFinished),
    };

    /** @brief kOfxImageEffectActionGetRegionsOfInterest property set */
    static PropertySetDescription gGetRegionOfInterestInArgPropSet(kOfxImageEffectActionGetRegionsOfInterest "in argument",
      gGetRegionOfInterestInArgProps, sizeof(gGetRegionOfInterestInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionGetTimeDomain action's outargs properties */
    static PropertyDescription gGetTimeDomainOutArgProps[ ] =
    {
      PropertyDescription(kOfxImageEffectPropFrameRange,   OFX::eDouble, 2, eDescFinished),
    };

    /** @brief kOfxImageEffectActionGetTimeDomain property set */
    static PropertySetDescription gGetTimeDomainOutArgPropSet(kOfxImageEffectActionGetTimeDomain " out argument", 
      gGetTimeDomainOutArgProps, sizeof(gGetTimeDomainOutArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionGetFramesNeeded action's inargs properties */
    static PropertyDescription gGetFramesNeededInArgProps[ ] =
    {
      PropertyDescription(kOfxPropTime,                     OFX::eDouble, 1, eDescFinished),
    };

    /** @brief kOfxImageEffectActionGetFramesNeeded  property set */
    static PropertySetDescription gGetFramesNeededInArgPropSet(kOfxImageEffectActionGetFramesNeeded " in argument", 
      gGetFramesNeededInArgProps, sizeof(gGetFramesNeededInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxImageEffectActionGetClipPreferences action's outargs properties */
    static PropertyDescription gGetClipPreferencesOutArgProps[ ] =
    {
      PropertyDescription(kOfxImageEffectPropFrameRate,         OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropFieldOrder,          OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxImageClipPropContinuousSamples,   OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageEffectFrameVarying,          OFX::eInt, 1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxImageEffectPropPreMultiplication, OFX::eString, 1, eDescFinished),
    };

    /** @brief kOfxImageEffectActionGetClipPreferences property set */
    static PropertySetDescription gGetClipPreferencesOutArgPropSet(kOfxImageEffectActionGetClipPreferences " out argument", 
      gGetClipPreferencesOutArgProps, sizeof(gGetClipPreferencesOutArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxActionInstanceChanged action's inargs properties */
    static PropertyDescription gInstanceChangedInArgProps[ ] =
    {
      PropertyDescription(kOfxPropType,                   OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropName,                   OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropChangeReason,           OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropTime,                   OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxImageEffectPropRenderScale, OFX::eDouble, 2, eDescFinished),
    };

    /** @brief kOfxActionInstanceChanged property set */
    static PropertySetDescription gInstanceChangedInArgPropSet(kOfxActionInstanceChanged " in argument",
      gInstanceChangedInArgProps, sizeof(gInstanceChangedInArgProps)/sizeof(PropertyDescription),
      NULLPTR);

    /** @brief kOfxActionBeginInstanceChanged and kOfxActionEndInstanceChanged actions' inargs properties */
    static PropertyDescription gBeginEndInstanceChangedInArgProps[ ] =
    {
      PropertyDescription(kOfxPropChangeReason,           OFX::eString, 1, eDescFinished),
    };

    /** @brief kOfxActionBeginInstanceChanged property set */
    static PropertySetDescription gBeginInstanceChangedInArgPropSet(kOfxActionBeginInstanceChanged " in argument",
      gBeginEndInstanceChangedInArgProps, sizeof(gBeginEndInstanceChangedInArgProps)/sizeof(PropertyDescription),
      NULLPTR);
    /** @brief kOfxActionEndInstanceChanged property set */
    static PropertySetDescription gEndInstanceChangedInArgPropSet(kOfxActionEndInstanceChanged " in argument",
      gBeginEndInstanceChangedInArgProps, sizeof(gBeginEndInstanceChangedInArgProps)/sizeof(PropertyDescription),
      NULLPTR);


    ////////////////////////////////////////////////////////////////////////////////
    // parameter properties 
    ////////////////////////////////////////////////////////////////////////////////

    /** @brief Basic parameter descriptor properties */
    static PropertyDescription gBasicParamProps[ ] =
    {
      PropertyDescription(kOfxPropType,                   OFX::eString, 1, eDescDefault, kOfxTypeParameter, eDescFinished),
      PropertyDescription(kOfxPropName,                   OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropLabel,                  OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropShortLabel,             OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxPropLongLabel,              OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxParamPropType,              OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxParamPropSecret,            OFX::eInt,    1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxParamPropHint,              OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxParamPropScriptName,        OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxParamPropParent,            OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxParamPropEnabled,           OFX::eInt,    1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDataPtr,           OFX::ePointer,1, eDescDefault, (void *)(0), eDescFinished),
    };


    /** @brief Props for params that can have an interact override their UI */
    static PropertyDescription gInteractOverideParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropInteractV1,           OFX::ePointer,1, eDescDefault, (void *)(0), eDescFinished),
      PropertyDescription(kOfxParamPropInteractSize,         OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxParamPropInteractSizeAspect,   OFX::eDouble, 1, eDescDefault, 1.0, eDescFinished),
      PropertyDescription(kOfxParamPropInteractMinimumSize,  OFX::eDouble, 2, eDescDefault, 10, 10, eDescFinished),
      PropertyDescription(kOfxParamPropInteractPreferedSize, OFX::eInt,    2, eDescDefault, 10, 10, eDescFinished),
    };

    /** @brief Props for params that can hold values. */
    static PropertyDescription gValueHolderParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropIsAnimating,               OFX::eInt,    1, eDescFinished),
      PropertyDescription(kOfxParamPropIsAutoKeying,              OFX::eInt,    1, eDescFinished),
      PropertyDescription(kOfxParamPropPersistant,                OFX::eInt,    1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropEvaluateOnChange,          OFX::eInt,    1, eDescDefault, 1, eDescFinished),
#    ifdef kOfxParamPropPluginMayWrite
      PropertyDescription(kOfxParamPropPluginMayWrite,            OFX::eInt,    1, eDescDefault, 0, eDescFinished), // removed in OFX 1.4
#    endif
      PropertyDescription(kOfxParamPropCacheInvalidation,         OFX::eString, 1, eDescDefault, kOfxParamInvalidateValueChange, eDescFinished),
      PropertyDescription(kOfxParamPropCanUndo,                   OFX::eInt,    1, eDescDefault, 1, eDescFinished),
    };

    /** @brief values for a string param */
    static PropertyDescription gStringParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eString, 1, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt,    1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxParamPropStringMode,           OFX::eString, 1, eDescDefault, kOfxParamStringIsSingleLine, eDescFinished),
      PropertyDescription(kOfxParamPropStringFilePathExists, OFX::eInt,    1, eDescDefault, 1, eDescFinished),
    };

    /** @brief values for a string param */
    static PropertyDescription gCustomParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,                OFX::eString,  1, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,               OFX::eInt,     1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxParamPropCustomInterpCallbackV1, OFX::ePointer, 1, eDescDefault, NULLPTR, eDescFinished),
    };

    /** @brief properties for an RGB colour param */
    static PropertyDescription gRGBColourParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eDouble, 3, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt,    1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eDouble, 3, eDescDefault, 0., 0., 0., eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eDouble, 3, eDescDefault, 1., 1., 1., eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eDouble, 3, eDescDefault, 0., 0., 0., eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eDouble, 3, eDescDefault, 1., 1., 1., eDescFinished),
      PropertyDescription(kOfxParamPropDimensionLabel,       OFX::eString, 3, eDescDefault, "r", "g", "b", eDescFinished),
    };

    /** @brief properties for an RGBA colour param */
    static PropertyDescription gRGBAColourParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eDouble, 4, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt,    1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eDouble, 4, eDescDefault, 0., 0., 0., 0., eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eDouble, 4, eDescDefault, 1., 1., 1., 1., eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eDouble, 4, eDescDefault, 0., 0., 0., 0., eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eDouble, 4, eDescDefault, 1., 1., 1., 1., eDescFinished),
      PropertyDescription(kOfxParamPropDimensionLabel,       OFX::eString, 4, eDescDefault, "r", "g", "b", "a", eDescFinished),
    };

    /** @brief properties for a boolean param */
    static PropertyDescription gBooleanParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt, 1, eDescDefault, 0, eDescFinished),
    };


    /** @brief properties for a choice param */
    static PropertyDescription gChoiceParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eInt,     1, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt,     1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxParamPropChoiceOption,         OFX::eString, -1, eDescFinished),
    };

    /** @brief properties for a string choice param */
    static PropertyDescription gStrChoiceParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eString,  1, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt,     1, eDescDefault, 0, eDescFinished),
      PropertyDescription(kOfxParamPropChoiceEnum,           OFX::eString, -1, eDescFinished),
      PropertyDescription(kOfxParamPropChoiceOption,         OFX::eString, -1, eDescFinished),
    };

    /** @brief properties for a 1D integer param */
    static PropertyDescription gInt1DParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eInt, 1, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt, 1, eDescDefault, 1, eDescFinished),
    };

    /** @brief properties for a 2D integer param */
    static PropertyDescription gInt2DParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eInt, 2, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eInt, 2, eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eInt, 2, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eInt, 2, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eInt, 2, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt, 1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDimensionLabel,       OFX::eString, 2, eDescDefault, "x", "y", eDescFinished),
    };

    /** @brief properties for a 3D integer param */
    static PropertyDescription gInt3DParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eInt, 3, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eInt, 3, eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eInt, 3, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eInt, 3, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eInt, 3, eDescFinished),
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt, 1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDimensionLabel,       OFX::eString, 3, eDescDefault, "x", "y", "z", eDescFinished),
    };

    /** @brief Properties common to all double params */
    static PropertyDescription gDoubleParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropAnimates,             OFX::eInt,    1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropIncrement,            OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDigits,               OFX::eInt,    1, eDescFinished),
      PropertyDescription(kOfxParamPropDoubleType,           OFX::eString, 1, eDescDefault, kOfxParamDoubleTypePlain, eDescFinished),
    };


    /** @brief properties for a 1D double param */
    static PropertyDescription gDouble1DParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eDouble, 1, eDescFinished),
      PropertyDescription(kOfxParamPropShowTimeMarker,       OFX::eInt,    1, eDescDefault, 0, eDescFinished),
    };

    /** @brief properties for a 2D double  param */
    static PropertyDescription gDouble2DParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eDouble, 2, eDescFinished),
      PropertyDescription(kOfxParamPropDimensionLabel,       OFX::eString, 2, eDescDefault, "x", "y", eDescFinished),
    };

    /** @brief properties for a 3D double param */
    static PropertyDescription gDouble3DParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropDefault,              OFX::eDouble, 3, eDescFinished),
      PropertyDescription(kOfxParamPropMin,                  OFX::eDouble, 3, eDescFinished),
      PropertyDescription(kOfxParamPropMax,                  OFX::eDouble, 3, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMin,           OFX::eDouble, 3, eDescFinished),
      PropertyDescription(kOfxParamPropDisplayMax,           OFX::eDouble, 3, eDescFinished),
      PropertyDescription(kOfxParamPropDimensionLabel,       OFX::eString, 3, eDescDefault, "x", "y", "z", eDescFinished),
    };

    /** @brief properties for a group param */
    static PropertyDescription gGroupParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropGroupOpen,           OFX::eInt, 1, eDescFinished),
    };

    /** @brief properties for a page param */
    static PropertyDescription gPageParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropPageChild,            OFX::eString, -1, eDescFinished),
    };

    /** @brief properties for a parametric param */
    static PropertyDescription gParametricParamProps[ ] =
    {
      PropertyDescription(kOfxParamPropAnimates,                     OFX::eInt,     1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropCanUndo,                      OFX::eInt,     1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropParametricDimension,          OFX::eInt,     1, eDescDefault, 1, eDescFinished),
      PropertyDescription(kOfxParamPropParametricUIColour,           OFX::eDouble, -1, eDescFinished),
      PropertyDescription(kOfxParamPropParametricInteractBackground, OFX::ePointer, 1, eDescDefault, (void*)(0), eDescFinished),
      PropertyDescription(kOfxParamPropParametricRange,              OFX::eDouble,  2, eDescDefault, 0.0, 1.0, eDescFinished),
    };

    /** @brief Property set for 1D ints */
    static PropertySetDescription gInt1DParamPropSet("1D Integer parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gInt1DParamProps),
      NULLPTR);


    /** @brief Property set for 2D ints */
    static PropertySetDescription gInt2DParamPropSet("2D Integer parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gInt2DParamProps),
      NULLPTR);

    /** @brief Property set for 3D ints */
    static PropertySetDescription gInt3DParamPropSet("3D Integer parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gInt3DParamProps),
      NULLPTR);

    /** @brief Property set for 1D doubles */
    static PropertySetDescription gDouble1DParamPropSet("1D Double parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gDoubleParamProps),
      mPropDescriptionArg(gDouble1DParamProps),
      NULLPTR);


    /** @brief Property set for 2D doubles */
    static PropertySetDescription gDouble2DParamPropSet("2D Double parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gDoubleParamProps),
      mPropDescriptionArg(gDouble2DParamProps),
      NULLPTR);

    /** @brief Property set for 3D doubles */
    static PropertySetDescription gDouble3DParamPropSet("3D Double parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gDoubleParamProps),
      mPropDescriptionArg(gDouble3DParamProps),
      NULLPTR);

    /** @brief Property set for RGB colour params */
    static PropertySetDescription gRGBParamPropSet("RGB Colour parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gRGBColourParamProps),
      NULLPTR);

    /** @brief Property set for RGB colour params */
    static PropertySetDescription gRGBAParamPropSet("RGB Colour parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gRGBAColourParamProps),
      NULLPTR);

    /** @brief Property set for string params */
    static PropertySetDescription gStringParamPropSet("String parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gStringParamProps),
      NULLPTR);

    /** @brief Property set for string params */
    static PropertySetDescription gCustomParamPropSet("Custom parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gCustomParamProps),
      NULLPTR);

    /** @brief Property set for boolean params */
    static PropertySetDescription gBooleanParamPropSet("Boolean parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gBooleanParamProps),
      NULLPTR);

    /** @brief Property set for choice params */
    static PropertySetDescription gChoiceParamPropSet("Choice parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gChoiceParamProps),
      NULLPTR);

    /** @brief Property set for string choice params */
    static PropertySetDescription gStrChoiceParamPropSet("String Choice parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gStrChoiceParamProps),
      NULLPTR);

    /** @brief Property set for push button params */
    static PropertySetDescription gPushButtonParamPropSet("PushButton parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      NULLPTR);

    /** @brief Property set for group params */
    static PropertySetDescription gGroupParamPropSet("Group Parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gGroupParamProps),
      NULLPTR);

    /** @brief Property set for page params */
    static PropertySetDescription gPageParamPropSet("Page Parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gPageParamProps),
      NULLPTR);

    static PropertySetDescription gParametricParamPropSet("Parametric Parameter",
      mPropDescriptionArg(gBasicParamProps),
      mPropDescriptionArg(gInteractOverideParamProps),
      mPropDescriptionArg(gValueHolderParamProps),
      mPropDescriptionArg(gParametricParamProps),
      NULLPTR);

#endif
    /** @brief Validates the host structure and property handle */
    void
      validateHostProperties(OfxHost *host)
    {
#ifdef kOfxsDisableValidation
    (void)host;
#else
      // make a description set
      PropertySet props(host->host);
      gHostPropSet.validate(props);
#endif
    }

    /** @brief Validates the effect descriptor properties */
    void
      validatePluginDescriptorProperties(PropertySet props)
    {
#ifdef kOfxsDisableValidation
    (void)props;
#else
      gPluginDescriptorPropSet.validate(props);
#endif
    }

    /** @brief Validates the effect instance properties */
    void
      validatePluginInstanceProperties(PropertySet props)
    {
#ifdef kOfxsDisableValidation
    (void)props;
#else
      gPluginInstancePropSet.validate(props);
#endif
    }

    /** @brief validates a clip descriptor */
    void
      validateClipDescriptorProperties(PropertySet props)
    {
#ifdef kOfxsDisableValidation
    (void)props;
#else
      gClipDescriptorPropSet.validate(props);
#endif
    }

    /** @brief validates a clip instance */
    void
      validateClipInstanceProperties(PropertySet props)
    {
#ifdef kOfxsDisableValidation
    (void)props;
#else
      gClipInstancePropSet.validate(props);
#endif
    }

    /** @brief validates an image or texture instance */
    void
      validateImageBaseProperties(PropertySet props)
    {
#ifdef kOfxsDisableValidation
    (void)props;
#else
      gImageBaseInstancePropSet.validate(props);
#endif
    }

    /** @brief validates an image instance */
    void
      validateImageProperties(PropertySet props)
    {
#ifdef kOfxsDisableValidation
    (void)props;
#else
      gImageInstancePropSet.validate(props);
#endif
    }

#ifdef OFX_SUPPORTS_OPENGLRENDER
    /** @brief validates an OpenGL texture instance */
    void
      validateTextureProperties(PropertySet props)
    {
#ifdef kOfxsDisableValidation
    (void)props;
#else
      gTextureInstancePropSet.validate(props);
#endif
    }
#endif

    /** @brief Validates action in/out arguments */
    void
      validateActionArgumentsProperties(const std::string &action, PropertySet inArgs, PropertySet outArgs)
    {
#ifdef kOfxsDisableValidation
    (void)action;
    (void)inArgs;
    (void)outArgs;
#else
      if(action == kOfxActionInstanceChanged) {
        gInstanceChangedInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxActionBeginInstanceChanged) {
        gBeginInstanceChangedInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxActionEndInstanceChanged) {
        gEndInstanceChangedInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxImageEffectActionGetRegionOfDefinition) {
        gGetRegionOfDefinitionInArgPropSet.validate(inArgs);
        gGetRegionOfDefinitionOutArgPropSet.validate(outArgs);
      }
      else if(action == kOfxImageEffectActionGetRegionsOfInterest) {
        gGetRegionOfInterestInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxImageEffectActionGetTimeDomain) {
        gGetTimeDomainOutArgPropSet.validate(outArgs);
      }
      else if(action == kOfxImageEffectActionGetFramesNeeded) {
        gGetFramesNeededInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxImageEffectActionGetClipPreferences) {
        gGetClipPreferencesOutArgPropSet.validate(outArgs);
      }
      else if(action == kOfxImageEffectActionIsIdentity) {
        gIsIdentityActionInArgPropSet.validate(inArgs);
        gIsIdentityActionOutArgPropSet.validate(outArgs);
      }
      else if(action == kOfxImageEffectActionRender) {
        gRenderActionInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxImageEffectActionBeginSequenceRender) {
        gBeginSequenceRenderActionInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxImageEffectActionEndSequenceRender) {
        gEndSequenceRenderActionInArgPropSet.validate(inArgs);
      }
      else if(action == kOfxImageEffectActionDescribeInContext) {
        gDescribeInContextActionInArgPropSet.validate(inArgs);
      }     
#endif 
    }

    /** @brief Validates parameter properties */
    void
      validateParameterProperties(ParamTypeEnum paramType, 
      OFX::PropertySet paramProps,
      bool checkDefaults)
    {
#ifdef kOfxsDisableValidation
    (void)paramType;
    (void)paramProps;
    (void)checkDefaults;
#else
      // should use a map here
      switch(paramType) 
      {
      case eStringParam :
        gStringParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eIntParam :	
        gInt1DParamPropSet.validate(paramProps,  checkDefaults);
        break;
      case eInt2DParam :
        gInt2DParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eInt3DParam :
        gInt3DParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eDoubleParam :
        gDouble1DParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eDouble2DParam :
        gDouble2DParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eDouble3DParam :
        gDouble3DParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eRGBParam :
        gRGBParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eRGBAParam :
        gRGBAParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eBooleanParam :
        gBooleanParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eChoiceParam :
        gChoiceParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eStrChoiceParam :
        gStrChoiceParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eCustomParam :
        gCustomParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eGroupParam :
        gGroupParamPropSet.validate(paramProps, checkDefaults);
        break;
      case ePageParam :
        gPageParamPropSet.validate(paramProps, checkDefaults);
        break;
      case ePushButtonParam :
        gPushButtonParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eParametricParam:
        gParametricParamPropSet.validate(paramProps, checkDefaults);
        break;
      case eDummyParam:
      //default:
            break;
      }
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////
    //

    /** @brief Initialises validation stuff that needs to be done once we know how the host behaves, called during the onload action */
    void
      initialise(void)
    {
#ifndef kOfxsDisableValidation
      static bool beenInitialised = false;
      if(!beenInitialised && getImageEffectHostDescription()) {
        beenInitialised = true;

        // create new property descriptions depending on certain host states
        PropertyDescription *desc;

        // do custom params animate ?
        desc = new PropertyDescription(kOfxParamPropAnimates, OFX::eInt, 1,
          eDescDefault, int(getImageEffectHostDescription()->supportsCustomAnimation),
          eDescFinished);
        gCustomParamPropSet.addProperty(desc, true);

        // do strings animate ?
        desc = new PropertyDescription(kOfxParamPropAnimates, OFX::eInt, 1,
          eDescDefault, int(getImageEffectHostDescription()->supportsStringAnimation),
          eDescFinished);
        gStringParamPropSet.addProperty(desc, true);

        // do choice params animate      
        desc = new PropertyDescription(kOfxParamPropAnimates, OFX::eInt, 1,
          eDescDefault, int(getImageEffectHostDescription()->supportsChoiceAnimation),
          eDescFinished);
        gChoiceParamPropSet.addProperty(desc, true);

        // do string choice params animate
        desc = new PropertyDescription(kOfxParamPropAnimates, OFX::eInt, 1,
          eDescDefault, int(getImageEffectHostDescription()->supportsStrChoiceAnimation),
          eDescFinished);
        gStrChoiceParamPropSet.addProperty(desc, true);

        // do boolean params animate
        desc = new PropertyDescription(kOfxParamPropAnimates, OFX::eInt, 1,
          eDescDefault, int(getImageEffectHostDescription()->supportsBooleanAnimation),
          eDescFinished);
        gBooleanParamPropSet.addProperty(desc, true);
      }
#endif
    }
  };
};
