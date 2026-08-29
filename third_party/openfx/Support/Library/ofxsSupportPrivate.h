

#ifndef _ofxsSupportPrivate_H_
#define _ofxsSupportPrivate_H_

#include "ofxsInteract.h"
#include "ofxsImageEffect.h"
#include "ofxsLog.h"
#include "ofxsMultiThread.h"

/** @brief Namespace private to the ofx support library.
*/
namespace OFX {

  namespace Private {
    /** @brief Pointer to the host */
    extern OfxHost *gHost;

    /** @brief Pointer to the effect suite */
    extern OfxImageEffectSuiteV1 *gEffectSuite;

    /** @brief Pointer to the property suite */
    extern OfxPropertySuiteV1    *gPropSuite;

    /** @brief Pointer to the  interact suite */
    extern OfxInteractSuiteV1    *gInteractSuite;

    /** @brief Pointer to the parameter suite */
    extern OfxParameterSuiteV1   *gParamSuite;

    /** @brief Pointer to the general memory suite */
    extern OfxMemorySuiteV1      *gMemorySuite;

    /** @brief Pointer to the threading suite */
    extern OfxMultiThreadSuiteV1 *gThreadSuite;

    /** @brief Pointer to the message suite */
    extern OfxMessageSuiteV1     *gMessageSuite;

    /** @brief Pointer to the optional message suite V2 */
    extern OfxMessageSuiteV2     *gMessageSuiteV2;

    /** @brief Pointer to the optional progress suite */
    extern OfxProgressSuiteV1     *gProgressSuiteV1;

    /** @brief Pointer to the optional progress suite */
    extern OfxProgressSuiteV2     *gProgressSuiteV2;

    /** @brief Pointer to the optional timeline suite */
    extern OfxTimeLineSuiteV1     *gTimeLineSuite;

    /** @brief Pointer to the parametric parameter suite */
    extern OfxParametricParameterSuiteV1* gParametricParameterSuite;

    /** @brief Support lib function called on an ofx load action */
    void loadAction(void);

    /** @brief Support lib function called on an ofx unload action */
    void unloadAction(void);

    /** @brief The plugin function that gets passed the host structure. 
    */
    void setHost(OfxHost *host);

    /** @brief fetches our pointer out of the props on the handle */
    ImageEffect *retrieveImageEffectPointer(OfxImageEffectHandle handle);

    /** @brief fetch the prop set from the effect handle */
    OFX::PropertySet
      fetchEffectProps(OfxImageEffectHandle handle);

    /** @brief the set of descriptors, one per context used by kOfxActionDescribeInContext,  'eContextNone' is the one used by the kOfxActionDescribe */
    typedef std::map<ContextEnum, ImageEffectDescriptor*> EffectContextMap;
    typedef std::map<std::string, EffectContextMap> EffectDescriptorMap;
    extern EffectDescriptorMap gEffectDescriptors;
  };

  /** @brief The validation code has its own namespace */
  namespace Validation {

    /** @brief This is uses to hold a property value, used by the property checking classes.

    Could have been a union, but std::string can't be in one.
    */
    struct ValueHolder {
      std::string  vString;
      int    vInt;
      double vDouble;
      void  *vPointer;

      ValueHolder(void) : vString(), vInt(0), vDouble(0.), vPointer(0) {}
      ValueHolder(char  *s) : vString(s), vInt(0), vDouble(0.), vPointer(0) {}
      ValueHolder(const std::string &s) : vString(s), vInt(0), vDouble(0.), vPointer(0) {}
      ValueHolder(int    i) : vString(), vInt(i), vDouble(0.), vPointer(0) {}
      ValueHolder(double d) : vString(), vInt(0), vDouble(d), vPointer(0) {}
      ValueHolder(void  *p) : vString(), vInt(0), vDouble(0.), vPointer(p) {}

      ValueHolder &operator = (char *v)  {vString = v; return *this;}
      ValueHolder &operator = (std::string v)  {vString = v; return *this;}
      ValueHolder &operator = (void *v)  {vPointer = v; return *this;}
      ValueHolder &operator = (int v)    {vInt = v; return *this;}
      ValueHolder &operator = (double v) {vDouble = v; return *this;}

      operator const char * () {return vString.c_str();}
      operator std::string &() {return vString;}
      operator int    &() {return vInt;}
      operator double &() {return vDouble;}
      operator void * &() {return vPointer;}
    };

    /** @brief Enum used in the varargs list of the PropertyDescription constructor  */
    enum DescriptionTag {
      eDescDefault,  /** @brief following values are the default to check against */
      eDescFinished  /** @brief we have finished the description */
    };

    /** @brief  class to describe properties, check their default and set their values */
    class PropertyDescription
    {
    public :
      /** @brief name of the property */
      const std::string _name;

      /** @brief Was it validated */
      bool _exists;

      /** @brief dimension of the property */
      int      _dimension;

      /** @brief What type of property is it */
      OFX::PropertyTypeEnum _ilk;

      /** @brief The default value that this property should have. Empty implies no default (eg: a host name has no default). */
      std::vector<ValueHolder> _defaultValue; 

    public :
      /** @brief var args constructor that is use to describe properties */
      PropertyDescription(const char *name, OFX::PropertyTypeEnum ilk, int dimension, ...);

      /** @brief Die! Die! Die! */
      virtual ~PropertyDescription(void) {}

      /** @brief See if the property exists in the containing property set and has the correct dimension */
      void validate(bool checkDefaults, PropertySet &propSet);
    };

    /** @brief Describes a set of properties */
    class PropertySetDescription {
    protected :
      /** @brief name of the property set */
      const std::string           _setName;

      /** @brief the descriptions of each property */
      std::vector<PropertyDescription *> _descriptions;

      /** @brief The descriptions of each property */
      std::vector<PropertyDescription *> _deleteThese;

    public :
      /** @brief constructor. 

      The varargs zero terminated are made from pairs of PropertyDescription * and ints indicating the number of properties pointed to.
      These are to come from static arrays and need not be deleted
      */
      PropertySetDescription(const char *setName, ...);// [PropertyDescription *v, int nSetToThese]

      /** @brief destructor */
      virtual ~PropertySetDescription();

      /** @brief add another property in */
      void addProperty(PropertyDescription *desc, bool deleteOnDestruction = true);

      /** @brief See if all properties exist and have the correct dimensions */
      void validate(PropertySet &propSet, bool checkDefaults = true, bool logOrdinaryMessages = false); 
    };


    /** @brief Validates the host structure and property handle */
    void
      validateHostProperties(OfxHost *host);

    /** @brief Validates the effect descriptor properties */
    void
      validatePluginDescriptorProperties(PropertySet props);

    /** @brief Validates the effect instance properties */
    void
      validatePluginInstanceProperties(PropertySet props);

    /** @brief validates a clip descriptor */
    void
      validateClipDescriptorProperties(PropertySet props);

    /** @brief validates a clip instance */
    void
      validateClipInstanceProperties(PropertySet props);

    /** @brief validates an image or texture instance */
    void
      validateImageBaseProperties(PropertySet props);

    /** @brief validates an image instance */
    void
      validateImageProperties(PropertySet props);

#ifdef OFX_SUPPORTS_OPENGLRENDER
    /** @brief validates an OpenGL texture descriptor */
    void
      validateTextureProperties(PropertySet props);
#endif

    /** @brief Validates action in/out arguments */
    void
      validateActionArgumentsProperties(const std::string &action, PropertySet inArgs, PropertySet outArgs);

    /** @brief Validates parameter properties */
    void
      validateParameterProperties(ParamTypeEnum paramType, 
      OFX::PropertySet paramProps,
      bool checkDefaults);

    /** @brief initialises the validation code, call this in on load */
    void initialise(void);
  };

};

#endif
