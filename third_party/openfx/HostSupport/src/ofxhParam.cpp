

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"
#ifdef OFX_SUPPORTS_PARAMETRIC
#include "ofxParametricParam.h"
#endif

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhParam.h"
#include "ofxhImageEffect.h"
#include "ofxOld.h" // old plugins may rely on deprecated properties being present


#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>

namespace OFX {

  namespace Host {

    namespace Param {

      //
      // Base
      //
      Base::Base(const std::string &name, const std::string& type) : 
        _paramName(name),
        _paramType(type)
      {
        assert(_paramType.c_str());
      }

      Base::Base(const std::string &name, const std::string &type, const Property::Set &properties) :
        _paramName(name),
        _paramType(type), 
        _properties(properties)
      {
        assert(_paramType.c_str());
      }


      Base::~Base() {}

      /// grab a handle on the parameter for passing to the C API
      OfxParamHandle Base::getHandle() const {
        return (OfxParamHandle)this;
      }

      /// grab a handle on the properties of this parameter for the C api
      OfxPropertySetHandle Base::getPropHandle() const {
        return _properties.getHandle();
      }

      Property::Set &Base::getProperties() {
        return _properties;
      }

      const Property::Set &Base::getProperties() const {
        return _properties;
      }

      const std::string &Base::getType() const {
        return _paramType;
      } 

      const std::string &Base::getName() const {
        return _paramName;
      }

      const std::string &Base::getParentName() const {
        return _properties.getStringProperty(kOfxParamPropParent);
      }

      const std::string &Base::getScriptName() const {
        return _properties.getStringProperty(kOfxParamPropScriptName);
      }

      const std::string &Base::getLabel() const {
        return _properties.getStringProperty(kOfxPropLabel);
      }

      const std::string &Base::getLongLabel() const {
        return _properties.getStringProperty(kOfxPropLongLabel);
      }

      const std::string &Base::getShortLabel() const {
        return _properties.getStringProperty(kOfxPropShortLabel);
      }

      const std::string &Base::getDoubleType() const {
        return _properties.getStringProperty(kOfxParamPropDoubleType, 0);
      }

      const std::string &Base::getDefaultCoordinateSystem() const {
        return _properties.getStringProperty(kOfxParamPropDefaultCoordinateSystem, 0);
      }

      const std::string &Base::getHint() const {
        return _properties.getStringProperty(kOfxParamPropHint, 0);
      }

      bool Base::getEnabled() const {
        return _properties.getIntProperty(kOfxParamPropEnabled, 0) != 0;
      }
      
      bool Base::getSecret() const {
        return _properties.getIntProperty(kOfxParamPropSecret, 0) != 0;
      }

      bool Base::getIsPersistant() const {
        return _properties.getIntProperty(kOfxParamPropPersistant, 0) != 0;
      }

      bool Base::getEvaluateOnChange() const {
        return _properties.getIntProperty(kOfxParamPropEvaluateOnChange, 0) != 0;
      }
      
      bool Base::getCanUndo() const {
        if (_properties.fetchProperty(kOfxParamPropCanUndo))  {
          return _properties.getIntProperty(kOfxParamPropCanUndo) != 0;
        }
        return false;
      }
      
      bool Base::getCanAnimate() const {
        if (_properties.fetchProperty(kOfxParamPropAnimates))  {
          return _properties.getIntProperty(kOfxParamPropAnimates) != 0;
        }
        return false;
      }

      //
      // Descriptor
      //

      struct TypeMap {
        const char *paramType;
        Property::TypeEnum propType;
        int propDimension;
      };

      static
      bool isDoubleParam(const std::string &paramType)
      {
        return paramType == kOfxParamTypeDouble ||
               paramType == kOfxParamTypeDouble2D ||
               paramType == kOfxParamTypeDouble3D
#ifdef OFX_SUPPORTS_PARAMETRIC
            || paramType == kOfxParamTypeParametric
#endif
          ;
      }

      bool isColourParam(const std::string &paramType)
      {
        return 
          paramType == kOfxParamTypeRGBA ||
          paramType == kOfxParamTypeRGB;
      }

      bool isIntParam(const std::string &paramType)
      {
        return paramType == kOfxParamTypeInteger ||
               paramType == kOfxParamTypeInteger2D ||
               paramType == kOfxParamTypeInteger3D;
      }

      static const TypeMap typeMap[] = {
        { kOfxParamTypeInteger,   Property::eInt,    1 },
        { kOfxParamTypeDouble,    Property::eDouble, 1 },
        { kOfxParamTypeBoolean,   Property::eInt,    1 },
        { kOfxParamTypeChoice,    Property::eInt,    1 },
        { kOfxParamTypeRGBA,      Property::eDouble, 4 },
        { kOfxParamTypeRGB,       Property::eDouble, 3 },
        { kOfxParamTypeDouble2D,  Property::eDouble, 2 },
        { kOfxParamTypeInteger2D, Property::eInt,    2 },
        { kOfxParamTypeDouble3D,  Property::eDouble, 3 },
        { kOfxParamTypeInteger3D, Property::eInt,    3 },
        { kOfxParamTypeString,    Property::eString, 1 },
        { kOfxParamTypeCustom,    Property::eString, 1 },
        { kOfxParamTypeGroup,     Property::eNone,   0 },
        { kOfxParamTypePage,      Property::eNone,   0 },
        { kOfxParamTypePushButton,Property::eNone,   0 },
#ifdef OFX_SUPPORTS_PARAMETRIC
        { kOfxParamTypeParametric,Property::eDouble, 0 },
#endif
        { 0,                      Property::eNone,   0  }
      };
      
      /// is this a standard type
      bool isStandardType(const std::string &type)
      {
        const TypeMap *tm = typeMap;
        while (tm->paramType) {
          if (tm->paramType == type) 
            return true;
          tm++;
        }
        return false;
      }
  
      static
      bool findType(const std::string paramType, Property::TypeEnum &propType, int &propDim)
      {
        const TypeMap *tm = typeMap;
        while (tm->paramType) {
          if (tm->paramType == paramType) {
            propType = tm->propType;
            propDim = tm->propDimension;
            return true;
          }
          tm++;
        }
        return false;
      }

      /// make a parameter, with the given type and name
      Descriptor::Descriptor(const std::string &type, 
                             const std::string &name) : Base(name, type)
      {
        const char *ctype = type.c_str();
        const char *cname = name.c_str();

        Property::PropSpec universalProps[] = {
          { kOfxPropType,    Property::eString,    1,    true,    kOfxTypeParameter },
          { kOfxParamPropSecret,  Property::eInt,    1,    false,    "0"},
          { kOfxParamPropHint,    Property::eString,    1,    false,    ""},
          { kOfxParamPropScriptName, Property::eString, 1, false, cname },
          { kOfxParamPropParent,  Property::eString,    1,    false,    "" },
          { kOfxParamPropEnabled, Property::eInt,    1,    false,    "1" },
          { kOfxParamPropDataPtr, Property::ePointer,    1,    false,    0 },
          { kOfxParamPropType,  Property::eString, 1, true,  ctype },
          { kOfxPropName,       Property::eString, 1, false, cname },
          { kOfxPropLabel,      Property::eString, 1, false, cname },
          { kOfxPropShortLabel, Property::eString, 1, false, cname },
          { kOfxPropLongLabel,  Property::eString, 1, false, cname },
          { kOfxPropIcon,       Property::eString, 2, false, "" },
          Property::propSpecEnd
        };
        
        _properties.addProperties(universalProps);
      }

      /// make a parameter, with the given type and name
      void Descriptor::addStandardParamProps(const std::string &type) 
      {
        Property::TypeEnum propType = Property::eString;
        int propDim = 1;
        findType(type, propType, propDim);


        static const Property::PropSpec allString[] = {
          { kOfxParamPropStringMode,  Property::eString,    1,    false,    kOfxParamStringIsSingleLine },
          { kOfxParamPropStringFilePathExists, Property::eInt,    1,    false,    "1" },
          Property::propSpecEnd
        };

        static const Property::PropSpec allChoice[] = {
          { kOfxParamPropChoiceOption,    Property::eString,    0,    false,    "" },
          Property::propSpecEnd
        };

        static const Property::PropSpec allCustom[] = {
          { kOfxParamPropCustomInterpCallbackV1,    Property::ePointer,    1,    false,    0 },
          Property::propSpecEnd
        };

        static const Property::PropSpec allPage[] = {
          { kOfxParamPropPageChild,    Property::eString,    0,    false,    "" },
          Property::propSpecEnd
        };

        static const Property::PropSpec allGroup[] = {
          { kOfxParamPropGroupOpen, Property::eInt, 1, false, "1" },
          Property::propSpecEnd
        };

#       ifdef OFX_SUPPORTS_PARAMETRIC
        static const Property::PropSpec allParametric[] = {
          { kOfxParamPropParametricDimension,         Property::eInt,     1,  false, "1" },
          { kOfxParamPropParametricUIColour,          Property::eDouble,  0,  false, ""  },
          { kOfxParamPropParametricInteractBackground,Property::ePointer, 1,  false, 0   },
          { kOfxParamPropParametricRange,             Property::eDouble,  2,  false, "0" },
          Property::propSpecEnd
        };
#       endif
               
        if (propType != Property::eNone) {
          addValueParamProps(type, propType, propDim);
        }

        if (type == kOfxParamTypeString) {
          _properties.addProperties(allString);
        }
  
        if (isDoubleParam(type) || isIntParam(type) || isColourParam(type)) {
          addNumericParamProps(type, propType, propDim);
        }

        if (type != kOfxParamTypeGroup && type != kOfxParamTypePage) {
          addInteractParamProps(type);
        }

        if (type == kOfxParamTypeChoice) {
          _properties.addProperties(allChoice);
        }

        if (type == kOfxParamTypeCustom) {
          _properties.addProperties(allCustom);
        }

        if (type == kOfxParamTypePage) {
          _properties.addProperties(allPage);
        }

        if (type == kOfxParamTypeGroup) {
          _properties.addProperties(allGroup);
        }

#       ifdef OFX_SUPPORTS_PARAMETRIC
        if (type == kOfxParamTypeParametric) {
          _properties.addProperties(allParametric);
          _properties.setDoubleProperty(kOfxParamPropParametricRange, 0., 0);
          _properties.setDoubleProperty(kOfxParamPropParametricRange, 1., 1);
        }
#       endif
      }

      /// add standard properties to a params that can take an interact
      void Descriptor::addInteractParamProps(const std::string &/*type*/)
      {
        static const Property::PropSpec allButGroupPageProps[] = {
          { kOfxParamPropInteractV1,          Property::ePointer, 1, false, 0 },
          { kOfxParamPropInteractSize,        Property::eDouble,  2, false, "0" },
          { kOfxParamPropInteractSizeAspect,  Property::eDouble,  1, false, "1" },
          { kOfxParamPropInteractMinimumSize, Property::eDouble,  2, false, "10" },
          { kOfxParamPropInteractPreferedSize,Property::eInt,     2, false, "10" },
          Property::propSpecEnd
        };

        
        _properties.addProperties(allButGroupPageProps);
      }

      /// add standard properties to a value holding param
      void Descriptor::addValueParamProps(const std::string &type, Property::TypeEnum valueType, int dim)
      {
        static const Property::PropSpec invariantProps[] = {
          { kOfxParamPropIsAnimating, Property::eInt, 1,       false, "0" },
          { kOfxParamPropIsAutoKeying,Property::eInt, 1,       false, "0" },
          { kOfxParamPropPersistant,  Property::eInt, 1,       false, "1" },
          { kOfxParamPropEvaluateOnChange, Property::eInt, 1,  false, "1" },
#       ifdef kOfxParamPropPluginMayWrite
          { kOfxParamPropPluginMayWrite,    Property::eInt,    1,    false,    "0" }, // removed in OFX 1.4
#       endif
          { kOfxParamPropCanUndo,    Property::eInt,    1,    false,    "1" },
          { kOfxParamPropCacheInvalidation,    Property::eString,    1,    false,    kOfxParamInvalidateValueChange },
          Property::propSpecEnd
        };

        /// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParametersAnimation
        /// The following may animate, depending on the host.
        /// Properties exist on the host to check this. If the host does support animation on them, then they do _not_ animate by default.
        /// They are...
        /// - kOfxParamTypeCustom
        /// - kOfxParamTypeString
        /// - kOfxParamTypeBoolean
        /// - kOfxParamTypeChoice
        /// If host doesn't support animation on them, then setting kOfxParamPropIsAnimating to 0 or 1 doesn't matter
        /// so just set the kOfxParamPropIsAnimating property to 0 for all those "extra animating" params.
        bool animates = type != kOfxParamTypeCustom && type != kOfxParamTypeString && type != kOfxParamTypeBoolean && type != kOfxParamTypeChoice;
          
        Property::PropSpec variantProps[] = {
          { kOfxParamPropAnimates,    Property::eInt, 1,       false, animates ? "1" : "0" },
          { kOfxParamPropDefault,     valueType,               dim, false, valueType == Property::eString ? "" : "0" },
          Property::propSpecEnd
        };

        _properties.addProperties(invariantProps);
        _properties.addProperties(variantProps);
      }
      
      /// add standard properties to a value holding param
      void Descriptor::addNumericParamProps(const std::string &type, Property::TypeEnum valueType, int dim)
      {
        static std::string dbl_minstr, dbl_maxstr, int_minstr, int_maxstr;
        static bool doneOne = false;

        if(!doneOne) {
          std::ostringstream dbl_min, dbl_max, int_min, int_max;
          doneOne = true;
          dbl_min << -DBL_MAX; // not a bug, @see kOfxParamPropDisplayMin
          dbl_max << DBL_MAX;
          int_min << INT_MIN;
          int_max << INT_MAX;
          
          dbl_minstr = dbl_min.str();
          dbl_maxstr = dbl_max.str();
          int_minstr = int_min.str();
          int_maxstr = int_max.str();
        }
          
        Property::PropSpec allNumeric[] = {
          { kOfxParamPropDisplayMin, valueType, dim, false, isColourParam(type) ? "0" : (valueType == Property::eDouble ? dbl_minstr : int_minstr).c_str() },
          { kOfxParamPropDisplayMax, valueType, dim, false, isColourParam(type) ? "1" : (valueType == Property::eDouble ? dbl_maxstr : int_maxstr).c_str() },
          { kOfxParamPropMin, valueType, dim, false, (valueType == Property::eDouble ? dbl_minstr : int_minstr).c_str() },
          { kOfxParamPropMax, valueType, dim, false, (valueType == Property::eDouble ? dbl_maxstr : int_maxstr).c_str() },
          Property::propSpecEnd
        };

        _properties.addProperties(allNumeric);

        /// if any double or a colour
        if (valueType == Property::eDouble) {
          static const Property::PropSpec allDouble[] = {
            { kOfxParamPropIncrement,  Property::eDouble,    1,    false,    "1" },
            { kOfxParamPropDigits,     Property::eInt,       1,    false,    "2" },
            Property::propSpecEnd
          };
          _properties.addProperties(allDouble);
        }

        /// if a double param type
        if(isDoubleParam(type)) {
          static const Property::PropSpec allDouble[] = {
            { kOfxParamPropDoubleType, Property::eString,    1,    false,    kOfxParamDoubleTypePlain },
            { kOfxParamPropDefaultCoordinateSystem, Property::eString,    1,    false,    kOfxParamCoordinatesCanonical },
            Property::propSpecEnd
          };
          _properties.addProperties(allDouble);
          
          if(dim == 1) {
            static const Property::PropSpec allDouble1D[] = {
              { kOfxParamPropShowTimeMarker, Property::eInt,   1,    false,    "0" },    
              Property::propSpecEnd
            };
            
            _properties.addProperties(allDouble1D);
          }
        }

        /// if a multi dimensional param
        if ((isDoubleParam(type) || isIntParam(type)) && (dim == 2 || dim == 3
#ifdef OFX_SUPPORTS_PARAMETRIC
                                    || dim == 0
#endif
                                    )) {
          static const Property::PropSpec all2D3D[] = {
            { kOfxParamPropDimensionLabel,  Property::eString, dim, false, "" },
            Property::propSpecEnd
          };  

          _properties.addProperties(all2D3D);
          _properties.setStringProperty(kOfxParamPropDimensionLabel, "x", 0);
          _properties.setStringProperty(kOfxParamPropDimensionLabel, "y", 1);
          if (dim == 3) {
            _properties.setStringProperty(kOfxParamPropDimensionLabel, "z", 2);
          }
        }

        /// if a multi dimensional param
        if (isColourParam(type)) {
          static const Property::PropSpec allColor[] = {
            { kOfxParamPropDimensionLabel,  Property::eString, dim, false, "" },
            Property::propSpecEnd
          };  

          _properties.addProperties(allColor);
          _properties.setStringProperty(kOfxParamPropDimensionLabel, "r", 0);
          _properties.setStringProperty(kOfxParamPropDimensionLabel, "g", 1);
          _properties.setStringProperty(kOfxParamPropDimensionLabel, "b", 2);
          if (dim == 4) {
            _properties.setStringProperty(kOfxParamPropDimensionLabel, "a", 3);
          }
        }
      }

      BaseSet::~BaseSet() {}
      
      /// obtain a handle on this set for passing to the C api
      SetDescriptor::SetDescriptor()
      {
      }

       /// obtain a handle on this set for passing to the C api
      OfxParamSetHandle BaseSet::getParamSetHandle() const
      {
        return (OfxParamSetHandle)this;
      }

      SetDescriptor::~SetDescriptor()
      {
        // iterate the params and delete them
        std::list<Descriptor *>::iterator i;
        for(i = _paramList.begin(); i != _paramList.end(); ++i) {
          if(*i)
            delete (*i);
        }
      }

      const std::map<std::string, Descriptor*> &SetDescriptor::getParams() const
      {
        return _paramMap;
      }

      const std::list<Descriptor*> &SetDescriptor::getParamList() const
      {
        return _paramList;
      }

      void SetDescriptor::addParam(const std::string &name, Descriptor *p) {
        _paramList.push_back(p);
        _paramMap[name] = p;
      }

      /// define a param on this effect
      Descriptor *SetDescriptor::paramDefine(const char *paramType,
                                             const char *name)
      {
        if(!isStandardType(paramType)) 
          return NULL; /// << EEK! This is bad.

        Descriptor *desc = new Descriptor(paramType, name); 
        desc->addStandardParamProps(paramType);
        addParam(name, desc);
        return desc;
      }

      ////////////////////////////////////////////////////////////////////////////////
      //
      // Instance
      //

      /// the description of a plugin parameter
      Instance::~Instance() {}

      /// make a parameter, with the given type and name
      Instance::Instance(Descriptor& descriptor, Param::SetInstance* paramSet) 
        : Base(descriptor.getName(), descriptor.getType(), descriptor.getProperties())
        , _paramSetInstance(paramSet)
        , _parentInstance(0)
      {
        _properties.addNotifyHook(kOfxParamPropEnabled, this);
        _properties.addNotifyHook(kOfxParamPropSecret, this);
        _properties.addNotifyHook(kOfxPropLabel, this);
        _properties.addNotifyHook(kOfxParamPropMin, this);
        _properties.addNotifyHook(kOfxParamPropMax, this);
        _properties.addNotifyHook(kOfxParamPropDisplayMin, this);
        _properties.addNotifyHook(kOfxParamPropDisplayMax, this);
        _properties.addNotifyHook(kOfxParamPropEvaluateOnChange, this);
      }

      // callback which should set enabled state as appropriate
      void Instance::setEnabled()
      {
      }

      // callback which should set secret state as appropriate
      void Instance::setSecret()
      {
      }
      
      // callback which should update label
      void Instance::setLabel()
      {
      }
      
      /// callback which should set range
      void Instance::setRange()
      {
      }
      
      /// callback which should set display range
      void Instance::setDisplayRange()
      {
      }
      
      /// callback which should set evaluate on change
      void Instance::setEvaluateOnChange()
      {
      }

      /// get a value, implemented by instances to deconstruct var args
      OfxStatus Instance::getV(va_list /*arg*/)
      {
        return kOfxStatErrUnsupported;
      }
      
      /// get a value, implemented by instances to deconstruct var args
      OfxStatus Instance::getV(OfxTime /*time*/, va_list /*arg*/)
      {
        return kOfxStatErrUnsupported;
      }

      /// set a value, implemented by instances to deconstruct var args
      OfxStatus Instance::setV(va_list /*arg*/)
      {
        return kOfxStatErrUnsupported;
      }

      /// key a value, implemented by instances to deconstruct var args
      OfxStatus Instance::setV(OfxTime /*time*/, va_list /*arg*/)
      {
        return kOfxStatErrUnsupported;
      }

      /// derive a value, implemented by instances to deconstruct var args
      OfxStatus Instance::deriveV(OfxTime /*time*/, va_list /*arg*/)
      {
        return kOfxStatErrUnsupported;
      }

      /// integrate a value, implemented by instances to deconstruct var args
      OfxStatus Instance::integrateV(OfxTime /*time1*/, OfxTime /*time2*/, va_list /*arg*/)
      {
        return kOfxStatErrUnsupported;
      }

      /// overridden from Property::NotifyHook
      void Instance::notify(const std::string &name, bool /*single*/, int /*num*/)
      {
        if (name == kOfxPropLabel) {
          setLabel();
        }
        if (name == kOfxParamPropEnabled) {
          setEnabled();
        }
        if (name == kOfxParamPropSecret) {
          setSecret();
        }
        if (name == kOfxParamPropMin || name == kOfxParamPropMax) {
          setRange();
        }
        if (name == kOfxParamPropDisplayMin || name == kOfxParamPropDisplayMax) {
          setDisplayRange();
        }
        if (name == kOfxParamPropEvaluateOnChange) {
          setEvaluateOnChange();
        }
      }

      // copy one parameter to another, with a range (NULL means to copy all animation)
      OfxStatus Instance::copyFrom(const Instance &/*instance*/, OfxTime /*offset*/, const OfxRangeD* /*range*/) {
        return kOfxStatErrMissingHostFeature; 
      }

      void Instance::setParentInstance(Instance* instance){
        _parentInstance = instance;
      }

      Instance* Instance::getParentInstance(){
        return _parentInstance;
      }

      //
      // KeyframeParam
      // 

      OfxStatus KeyframeParam::getNumKeys(unsigned int &/*nKeys*/) const {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus KeyframeParam::getKeyTime(int /*nth*/, OfxTime& /*time*/) const {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus KeyframeParam::getKeyIndex(OfxTime /*time*/, int /*direction*/, int & /*index*/) const {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus KeyframeParam::deleteKey(OfxTime /*time*/) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus KeyframeParam::deleteAllKeys() { 
        return kOfxStatErrMissingHostFeature; 
      }

      void GroupInstance::setChildren(std::vector<Param::Instance*> children)
      {
        _children = children;
        for (std::vector<Param::Instance*>::iterator it=children.begin(); it!=children.end(); ++it) {
          if(*it){
            (*it)->setParentInstance(this);
          }
        }
      }
      
      const std::vector<Param::Instance*> &GroupInstance::getChildren() const
      {
        return _children;
      }

      //
      // Page Instance
      //

      const std::map<int,Param::Instance*> &PageInstance::getChildren() const 
      {
        // HACK!!!! this really should be done with a notify hook so we don't force
        // _children to be mutable
        if(_children.size() == 0 )
        {
          int nChildren = _properties.getDimension(kOfxParamPropPageChild);
          for(int i=0;i<nChildren;i++)
          {
            std::string childName = _properties.getStringProperty(kOfxParamPropPageChild,i);        
            Param::Instance* child = _paramSetInstance->getParam(childName);
            if(child)
              _children[i]=child;
          }
        }
        return _children;
      }

      //
      // ChoiceInstance
      //

      /// make a parameter, with the given type and name
      ChoiceInstance::ChoiceInstance(Descriptor& descriptor, Param::SetInstance* instance)
        : Instance(descriptor,instance)
      {
        _properties.addNotifyHook(kOfxParamPropChoiceOption, this);
      }

      // callback which should set option as appropriate
      void ChoiceInstance::setOption(int /*num*/)
      {
      }

      /// implementation of var args function
      OfxStatus ChoiceInstance::getV(va_list arg)
      {
        int *value = va_arg(arg, int*);
        OfxStatus stat = get(*value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus ChoiceInstance::getV(OfxTime time, va_list arg)
      {
        int *value = va_arg(arg, int*);
        OfxStatus stat = get(time, *value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus ChoiceInstance::setV(va_list arg)
      {
        int value = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(value);
      }

      /// implementation of var args function
      OfxStatus ChoiceInstance::setV(OfxTime time, va_list arg)
      {
        int value = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(time, value);
      }
      
      /// overridden from Instance
      void ChoiceInstance::notify(const std::string &name, bool single, int num)
      {
        Instance::notify(name, single, num);
        if (name == kOfxParamPropChoiceOption) {
          setOption(num);
        }
      }
      //
      // IntegerInstance
      //
      OfxStatus IntegerInstance::derive(OfxTime /*time*/, int&) {
        return kOfxStatErrUnsupported; 
      }

      OfxStatus IntegerInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, int&) {
        return kOfxStatErrUnsupported; 
      }

      /// implementation of var args function
      OfxStatus IntegerInstance::getV(va_list arg)
      {
        int *value = va_arg(arg, int*);
        OfxStatus stat = get(*value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus IntegerInstance::getV(OfxTime time, va_list arg)
      {
        int *value = va_arg(arg, int*);
        OfxStatus stat = get(time, *value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus IntegerInstance::setV(va_list arg)
      {
        int value = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(value);
      }

      /// implementation of var args function
      OfxStatus IntegerInstance::setV(OfxTime time, va_list arg)
      {
        int value = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(time, value);
      }
      
      /// implementation of var args function
      OfxStatus IntegerInstance::deriveV(OfxTime time, va_list arg)
      {
        int *value = va_arg(arg, int*);
        OfxStatus stat = derive(time, *value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus IntegerInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        int *value = va_arg(arg, int*);
        OfxStatus stat = integrate(time1, time2, *value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }

      //
      // DoubleInstance
      //
      /// implementation of var args function
      OfxStatus DoubleInstance::getV(va_list arg)
      {
        double *value = va_arg(arg, double*);
        OfxStatus stat = get(*value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus DoubleInstance::getV(OfxTime time, va_list arg)
      {
        double *value = va_arg(arg, double*);
        OfxStatus stat = get(time, *value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus DoubleInstance::setV(va_list arg)
      {
        double value = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(value);
      }

      /// implementation of var args function
      OfxStatus DoubleInstance::setV(OfxTime time, va_list arg)
      {
        double value = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(time, value);
      }
      
      /// implementation of var args function
      OfxStatus DoubleInstance::deriveV(OfxTime time, va_list arg)
      {
        double *value = va_arg(arg, double*);
        OfxStatus stat = derive(time, *value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus DoubleInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        double *value = va_arg(arg, double*);
        OfxStatus stat = integrate(time1, time2, *value);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }

      //
      // BooleanInstance
      //
      /// implementation of var args function
      OfxStatus BooleanInstance::getV(va_list arg)
      {
        bool v;
        OfxStatus stat = get(v);

        int *value = va_arg(arg, int*);
        *value = v;
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus BooleanInstance::getV(OfxTime time, va_list arg)
      {
        bool v;
        OfxStatus stat = get(time, v);

        int *value = va_arg(arg, int*);
        *value = v;
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus BooleanInstance::setV(va_list arg)
      {
        bool value = va_arg(arg, int) != 0;
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(value);
      }

      /// implementation of var args function
      OfxStatus BooleanInstance::setV(OfxTime time, va_list arg)
      {
        bool value = va_arg(arg, int) != 0;
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(time, value);
      }
      

      // 
      // RGBAInstance
      // 

      OfxStatus RGBAInstance::derive(OfxTime /*time*/, double&, double&, double&, double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus RGBAInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, double&,double&,double&,double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      /// implementation of var args function
      OfxStatus RGBAInstance::getV(va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        double *a = va_arg(arg, double*);
        OfxStatus stat = get(*r, *g, *b, *a);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b << ' ' << *a;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus RGBAInstance::getV(OfxTime time, va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        double *a = va_arg(arg, double*);
        OfxStatus stat = get(time, *r, *g, *b, *a);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b << ' ' << *a;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus RGBAInstance::setV(va_list arg)
      {
        double r = va_arg(arg, double);
        double g = va_arg(arg, double);
        double b = va_arg(arg, double);
        double a = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << r << ',' << g << ',' << b << ',' << a << ')';
#       endif
        return set(r, g, b, a);
      }

      /// implementation of var args function
      OfxStatus RGBAInstance::setV(OfxTime time, va_list arg)
      {
        double r = va_arg(arg, double);
        double g = va_arg(arg, double);
        double b = va_arg(arg, double);
        double a = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << r << ',' << g << ',' << b << ',' << a << ')';
#       endif
        return set(time, r, g, b, a);
      }
      
      /// implementation of var args function
      OfxStatus RGBAInstance::deriveV(OfxTime time, va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        double *a = va_arg(arg, double*);
        OfxStatus stat = derive(time, *r, *g, *b, *a);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b << ' ' << *a;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus RGBAInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        double *a = va_arg(arg, double*);
        OfxStatus stat = integrate(time1, time2, *r, *g, *b, *a);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b << ' ' << *a;
        }
#       endif
        return stat;
      }

      //
      // RGBInstance
      //
      OfxStatus RGBInstance::derive(OfxTime /*time*/, double&,double&,double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus RGBInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, double&,double&,double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      /// implementation of var args function
      OfxStatus RGBInstance::getV(va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        OfxStatus stat = get(*r, *g, *b);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus RGBInstance::getV(OfxTime time, va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        OfxStatus stat = get(time, *r, *g, *b);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus RGBInstance::setV(va_list arg)
      {
        double r = va_arg(arg, double);
        double g = va_arg(arg, double);
        double b = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << r << ',' << g << ',' << b << ')';
#       endif
        return set(r, g, b);
      }

      /// implementation of var args function
      OfxStatus RGBInstance::setV(OfxTime time, va_list arg)
      {
        double r = va_arg(arg, double);
        double g = va_arg(arg, double);
        double b = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << r << ',' << g << ',' << b << ')';
#       endif
        return set(time, r, g, b);
      }
      
      /// implementation of var args function
      OfxStatus RGBInstance::deriveV(OfxTime time, va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        OfxStatus stat = derive(time, *r, *g, *b);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus RGBInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        double *r = va_arg(arg, double*);
        double *g = va_arg(arg, double*);
        double *b = va_arg(arg, double*);
        OfxStatus stat = integrate(time1, time2, *r, *g, *b);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *r << ' ' << *g << ' ' << *b;
        }
#       endif
        return stat;
      }

      //
      // Double2DInstance
      //

      OfxStatus Double2DInstance::derive(OfxTime /*time*/, double&,double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Double2DInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, double&,double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Double2DInstance::getV(va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        OfxStatus stat = get(*value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Double2DInstance::getV(OfxTime time, va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        OfxStatus stat = get(time, *value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Double2DInstance::setV(va_list arg)
      {
        double value1 = va_arg(arg, double);
        double value2 = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ')';
#       endif
        return set(value1, value2);
      }

      /// implementation of var args function
      OfxStatus Double2DInstance::setV(OfxTime time, va_list arg)
      {
        double value1 = va_arg(arg, double);
        double value2 = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ')';
#       endif
        return set(time, value1, value2);
      }
      
      /// implementation of var args function
      OfxStatus Double2DInstance::deriveV(OfxTime time, va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        OfxStatus stat = derive(time, *value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Double2DInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        OfxStatus stat = integrate(time1, time2, *value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }

      //
      // Integer2DInstance
      //

      OfxStatus Integer2DInstance::derive(OfxTime /*time*/, int&,int&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Integer2DInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, int&,int&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Integer2DInstance::getV(va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        OfxStatus stat = get(*value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Integer2DInstance::getV(OfxTime time, va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        OfxStatus stat = get(time, *value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Integer2DInstance::setV(va_list arg)
      {
        int value1 = va_arg(arg, int);
        int value2 = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ')';
#       endif
        return set(value1, value2);
      }

      /// implementation of var args function
      OfxStatus Integer2DInstance::setV(OfxTime time, va_list arg)
      {
        int value1 = va_arg(arg, int);
        int value2 = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ')';
#       endif
        return set(time, value1, value2);
      }
      
      /// implementation of var args function
      OfxStatus Integer2DInstance::deriveV(OfxTime time, va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        OfxStatus stat = derive(time, *value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Integer2DInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        OfxStatus stat = integrate(time1, time2, *value1, *value2);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2;
        }
#       endif
        return stat;
      }

      //
      // Double3DInstance
      //

      OfxStatus Double3DInstance::derive(OfxTime /*time*/, double&,double&,double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Double3DInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, double&,double&,double&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Double3DInstance::getV(va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        double *value3 = va_arg(arg, double*);
        OfxStatus stat = get(*value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Double3DInstance::getV(OfxTime time, va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        double *value3 = va_arg(arg, double*);
        OfxStatus stat = get(time, *value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Double3DInstance::setV(va_list arg)
      {
        double value1 = va_arg(arg, double);
        double value2 = va_arg(arg, double);
        double value3 = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ',' << value3 << ')';
#       endif
        return set(value1, value2, value3);
      }

      /// implementation of var args function
      OfxStatus Double3DInstance::setV(OfxTime time, va_list arg)
      {
        double value1 = va_arg(arg, double);
        double value2 = va_arg(arg, double);
        double value3 = va_arg(arg, double);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ',' << value3 << ')';
#       endif
        return set(time, value1, value2, value3);
      }
      
      /// implementation of var args function
      OfxStatus Double3DInstance::deriveV(OfxTime time, va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        double *value3 = va_arg(arg, double*);
        OfxStatus stat = derive(time, *value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Double3DInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        double *value1 = va_arg(arg, double*);
        double *value2 = va_arg(arg, double*);
        double *value3 = va_arg(arg, double*);
        OfxStatus stat = integrate(time1, time2, *value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }

      //
      // Integer3DInstance
      //
      OfxStatus Integer3DInstance::derive(OfxTime /*time*/, int&,int&,int&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Integer3DInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, int&,int&,int&) {
        return kOfxStatErrMissingHostFeature; 
      }

      OfxStatus Integer3DInstance::getV(va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        int *value3 = va_arg(arg, int*);
        OfxStatus stat = get(*value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Integer3DInstance::getV(OfxTime time, va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        int *value3 = va_arg(arg, int*);
        OfxStatus stat = get(time, *value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Integer3DInstance::setV(va_list arg)
      {
        int value1 = va_arg(arg, int);
        int value2 = va_arg(arg, int);
        int value3 = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ',' << value3 << ')';
#       endif
        return set(value1, value2, value3);
      }

      /// implementation of var args function
      OfxStatus Integer3DInstance::setV(OfxTime time, va_list arg)
      {
        int value1 = va_arg(arg, int);
        int value2 = va_arg(arg, int);
        int value3 = va_arg(arg, int);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << '(' << value1 << ',' << value2 << ',' << value3 << ')';
#       endif
        return set(time, value1, value2, value3);
      }
      
      /// implementation of var args function
      OfxStatus Integer3DInstance::deriveV(OfxTime time, va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        int *value3 = va_arg(arg, int*);
        OfxStatus stat = derive(time, *value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus Integer3DInstance::integrateV(OfxTime time1, OfxTime time2, va_list arg)
      {
        int *value1 = va_arg(arg, int*);
        int *value2 = va_arg(arg, int*);
        int *value3 = va_arg(arg, int*);
        OfxStatus stat = integrate(time1, time2, *value1, *value2, *value3);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value1 << ' ' << *value2 << ' ' << *value3;
        }
#       endif
        return stat;
      }

      ////////////////////////////////////////////////////////////////////////////////
      // string param
      OfxStatus StringInstance::getV(va_list arg)
      {
        const char **value = va_arg(arg, const char **);

        OfxStatus stat = get(_returnValue); // I so don't like this, temp storage should be delegated to the implementation
        *value = _returnValue.c_str();
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus StringInstance::getV(OfxTime time, va_list arg)
      {
        const char **value = va_arg(arg, const char **);

        OfxStatus stat = get(time, _returnValue); // I so don't like this, temp storage should be delegated to the implementation
        *value = _returnValue.c_str();
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *value;
        }
#       endif
        return stat;
      }
      
      /// implementation of var args function
      OfxStatus StringInstance::setV(va_list arg)
      {
        char *value = va_arg(arg, char*);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(value);
      }

      /// implementation of var args function
      OfxStatus StringInstance::setV(OfxTime time, va_list arg)
      {
        char *value = va_arg(arg, char*);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << value;
#       endif
        return set(time, value);
      }
      
      //////////////////////////////////////////////////////////////////////////////////
      // Param::SetInstance
      //

      /// ctor
      SetInstance::SetInstance()
      {}

      /// dtor. 
      SetInstance::~SetInstance()
      {
        // iterate the params and delete them
        std::list<Instance *>::iterator i;
        for(i = _paramList.begin(); i != _paramList.end(); ++i) {
          if(*i) 
            delete (*i);
        }
      }

      const std::map<std::string, Instance*> &SetInstance::getParams() const
      {
        return _params;
      }

      const std::list<Instance*> &SetInstance::getParamList() const
      {
        return _paramList;
      }

      OfxStatus SetInstance::addParam(const std::string& name, Instance* instance)
      {
        if(_params.find(name)==_params.end()){
          _params[name] = instance;
          _paramList.push_back(instance);
        }
        else
          return kOfxStatErrExists;

        return kOfxStatOK;
      }

      ////////////////////////////////////////////////////////////////////////////////
      // Suite functions below

      static OfxStatus paramDefine(OfxParamSetHandle paramSet,
                                   const char *paramType,
                                   const char *name,
                                   OfxPropertySetHandle *propertySet)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramDefine - " << paramSet << ' ' << paramType << ' ' << name << ' ' << propertySet << " ...";
#       endif
        SetDescriptor *paramSetDescriptor = reinterpret_cast<SetDescriptor*>(paramSet);

        if (!paramSetDescriptor) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        Descriptor *desc = paramSetDescriptor->paramDefine(paramType, name);

        if(desc) {
          if (propertySet)
            *propertySet = desc->getPropHandle();
          // desc is still referenced by _paramList and _paramMap
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
          return kOfxStatOK;
        } else {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrUnsupported) << std::endl;
#         endif
          return kOfxStatErrUnsupported;
        }
      }
      
      static OfxStatus paramGetHandle(OfxParamSetHandle paramSet,
                                      const char *name,
                                      OfxParamHandle *param,
                                      OfxPropertySetHandle *propertySet)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetHandle - " << paramSet << ' ' << name << ' ' << param << ' ' << propertySet << " ...";
#       endif
        BaseSet *baseSet = reinterpret_cast<BaseSet*>(paramSet);

        if (!baseSet) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        SetInstance *setInstance = dynamic_cast<SetInstance*>(baseSet);

        if(setInstance){          
          const std::map<std::string,Instance*>& params = setInstance->getParams();
          std::map<std::string,Instance*>::const_iterator it = params.find(name);         

          // if we can't find it return an error...
          if(it==params.end()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' '<< StatStr(kOfxStatErrUnknown) << std::endl;
#           endif
            return kOfxStatErrUnknown;
          }

          // get the param
          if (param) {
            *param = (it->second)->getHandle();
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << *param;
#           endif
          }

          // get the param property set
          if(propertySet) {
            *propertySet = (it->second)->getPropHandle();
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << *propertySet;
#           endif
          }

#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
          return kOfxStatOK;
        }

        SetDescriptor *setDescriptor = dynamic_cast<SetDescriptor*>(baseSet);
        
        if(setDescriptor){            
          const std::map<std::string,Descriptor*>& params = setDescriptor->getParams();
          std::map<std::string,Descriptor*>::const_iterator it = params.find(name);         
          
          // if we can't find it return an error...
          if(it==params.end()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' '<< StatStr(kOfxStatErrUnknown) << std::endl;
#           endif
            return kOfxStatErrUnknown;
          }
          
          // get the param
          if (param) {
            *param = (it->second)->getHandle();  
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << *param;
#           endif
          }

          // get the param property set
          if(propertySet) {
            *propertySet = (it->second)->getPropHandle();
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << *propertySet;
#           endif
          }

#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
          return kOfxStatOK;
        }

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
      }
      
      static OfxStatus paramSetGetPropertySet(OfxParamSetHandle paramSet,
                                              OfxPropertySetHandle *propHandle)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramSetGetPropertySet - " << paramSet << ' ' << propHandle << " ...";
#       endif
        BaseSet *baseSet = reinterpret_cast<BaseSet*>(paramSet);

        if (baseSet) {
          if (propHandle) {
            *propHandle = baseSet->getParamSetProps().getHandle();
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << *propHandle;
#           endif
          }
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
          return kOfxStatOK;
        }
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
        return kOfxStatErrBadHandle;
      } 
      
      static OfxStatus paramGetPropertySet(OfxParamHandle param,
                                           OfxPropertySetHandle *propHandle)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetPropertySet - " << param << ' ' << propHandle << " ...";
#       endif
        Param::Instance *paramInstance = reinterpret_cast<Param::Instance*>(param);
        
        if(paramInstance && paramInstance->verifyMagic()){
          // get the param property set
          if (propHandle) {
            *propHandle = paramInstance->getPropHandle();
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << *propHandle;
#           endif
          }

#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
          return kOfxStatOK;
        } else {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }
      }

      /// get the current param value
      static OfxStatus paramGetValue(OfxParamHandle  paramHandle,
                                     ...)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetValue - " << paramHandle << " ...";
#       endif
        Instance *paramInstance = reinterpret_cast<Instance*>(paramHandle);
        if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        va_list ap;
        va_start(ap,paramHandle);
        OfxStatus stat = kOfxStatErrUnsupported;

        try {
          stat = paramInstance->getV(ap);
        }
        catch(...) {}

        va_end(ap);

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }

      /// get the param value at a time
      static OfxStatus paramGetValueAtTime(OfxParamHandle  paramHandle,
                                           OfxTime time,
                                           ...)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetValueAtTime - " << paramHandle << ' ' << time << " ...";
#       endif
        Instance *paramInstance = reinterpret_cast<Instance*>(paramHandle);
        if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }


        va_list ap;
        va_start(ap, time);
        OfxStatus stat = kOfxStatErrUnsupported;

        try {
          stat = paramInstance->getV(time, ap);
        }
        catch(...) {}

        va_end(ap);

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      /// get the param's derivative at the given time
      static OfxStatus paramGetDerivative(OfxParamHandle  paramHandle,
                                          OfxTime time,
                                          ...)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetDerivative - " << paramHandle << ' ' << time << " ...";
#       endif
        Instance *paramInstance = reinterpret_cast<Instance*>(paramHandle);
        if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        va_list ap;
        va_start(ap, time);
        OfxStatus stat = kOfxStatErrUnsupported;

        try {
          stat = paramInstance->deriveV(time, ap);
        }
        catch(...) {}

        va_end(ap);

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }

      static OfxStatus paramGetIntegral(OfxParamHandle  paramHandle,
                                        OfxTime time1, OfxTime time2,
                                        ...)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetIntegral - " << paramHandle << ' ' << time1 << ' ' << time2 << " ...";
#       endif
        Instance *paramInstance = reinterpret_cast<Instance*>(paramHandle);
        if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        va_list ap;
        va_start(ap, time2);
        OfxStatus stat = kOfxStatErrUnsupported;

        try {
          stat = paramInstance->integrateV(time1, time2, ap);
        }
        catch(...) {}

        va_end(ap);

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }

      /// set the param's value at the 'current' time
      static OfxStatus paramSetValue(OfxParamHandle  paramHandle,
                                     ...) 
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramSetValue - " << paramHandle << ' ';
#       endif
        Instance *paramInstance = reinterpret_cast<Instance*>(paramHandle);
        if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << " ... " << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        va_list ap;
        va_start(ap, paramHandle);
        OfxStatus stat = kOfxStatErrUnsupported;

        try {
          stat = paramInstance->setV(ap);
        }
        catch(...) {}

        va_end(ap);

        if (stat == kOfxStatOK) {
          paramInstance->getParamSetInstance()->paramChangedByPlugin(paramInstance);
        }

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << " ... " << StatStr(stat) << std::endl;
#       endif
        return stat;
      }

      
      /// set the param's value at the indicated time, and set a key
      static OfxStatus paramSetValueAtTime(OfxParamHandle  paramHandle,
                                           OfxTime time,  // time in frames
                                           ...)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramSetValueAtTime - " << paramHandle << ' ' << time << ' ';
#       endif
        Instance *paramInstance = reinterpret_cast<Instance*>(paramHandle);
        if(!paramInstance || !paramInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << " ... " << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        va_list ap;
        va_start(ap, time);
        OfxStatus stat = kOfxStatErrUnsupported;

        try {
          stat = paramInstance->setV(time, ap);
        }
        catch(...) {}

        va_end(ap);

        if (stat == kOfxStatOK) {
          paramInstance->getParamSetInstance()->paramChangedByPlugin(paramInstance);
        }

#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << " ... " << StatStr(stat) << std::endl;
#       endif
        return stat;
      }

      static OfxStatus paramGetNumKeys(OfxParamHandle  paramHandle,
                                       unsigned int  *numberOfKeys)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetNumKeys - " << paramHandle << " ...";
#       endif
        Param::Instance *pInstance = reinterpret_cast<Param::Instance*>(paramHandle);

        if (!pInstance || !pInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        KeyframeParam *paramInstance = dynamic_cast<KeyframeParam*>(pInstance);
        if(!paramInstance) {
#       ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
          return kOfxStatErrBadHandle;
        }
        OfxStatus stat = paramInstance->getNumKeys(*numberOfKeys);
#       ifdef OFX_DEBUG_PARAMETERS
        if (stat == kOfxStatOK) {
          std::cout << ' ' << *numberOfKeys;
        }
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      static OfxStatus paramGetKeyTime(OfxParamHandle  paramHandle,
                                       unsigned int nthKey,
                                       OfxTime *time)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetKeyTime - " << paramHandle << " ...";
#       endif
        Param::Instance *pInstance = reinterpret_cast<Param::Instance*>(paramHandle);

        if (!pInstance || !pInstance->verifyMagic()) {
#       ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
          return kOfxStatErrBadHandle;
        }

        KeyframeParam *paramInstance = dynamic_cast<KeyframeParam*>(pInstance);
        if(!paramInstance) {
#       ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#       endif
          return kOfxStatErrBadHandle;
        }
        OfxStatus stat = paramInstance->getKeyTime(nthKey,*time);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      static OfxStatus paramGetKeyIndex(OfxParamHandle  paramHandle,
                                        OfxTime time,
                                        int     direction,
                                        int    *index) 
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramGetKeyIndex - " << paramHandle << " ...";
#       endif
        Param::Instance *pInstance = reinterpret_cast<Param::Instance*>(paramHandle);

        if (!pInstance || !pInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        KeyframeParam *paramInstance = dynamic_cast<KeyframeParam*>(pInstance);
        if(!paramInstance) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }
        OfxStatus stat = paramInstance->getKeyIndex(time,direction,*index);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      static OfxStatus paramDeleteKey(OfxParamHandle  paramHandle,
                                      OfxTime time)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramDeleteKey - " << paramHandle << " ...";
#       endif
        Param::Instance *pInstance = reinterpret_cast<Param::Instance*>(paramHandle);

        if (!pInstance || !pInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        KeyframeParam *paramInstance = dynamic_cast<KeyframeParam*>(pInstance);
        if(!paramInstance) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }
        OfxStatus stat = paramInstance->deleteKey(time);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      static OfxStatus paramDeleteAllKeys(OfxParamHandle  paramHandle) 
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramDeleteAllKeys - " << paramHandle << " ...";
#       endif
        Param::Instance *pInstance = reinterpret_cast<Param::Instance*>(paramHandle);
        
        if (!pInstance || !pInstance->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        KeyframeParam *paramInstance = dynamic_cast<KeyframeParam*>(pInstance);
        if(!paramInstance) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }
        OfxStatus stat = paramInstance->deleteAllKeys();
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      static OfxStatus paramCopy(OfxParamHandle  paramTo,
                                 OfxParamHandle  paramFrom, 
                                 OfxTime dstOffset, const OfxRangeD *frameRange)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramCopy - " << paramTo << " ...";
#       endif
        Instance *paramInstanceTo = reinterpret_cast<Instance*>(paramTo);
        Instance *paramInstanceFrom = reinterpret_cast<Instance*>(paramFrom);        

        if(!paramInstanceTo || !paramInstanceTo->verifyMagic() ||
           !paramInstanceFrom || !paramInstanceFrom->verifyMagic()) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }

        OfxStatus stat = paramInstanceTo->copyFrom(*paramInstanceFrom,dstOffset,frameRange);
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      static OfxStatus paramEditBegin(OfxParamSetHandle paramSet, const char *name)
      {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramEditBegin - " << paramSet << ' ' << name << " ...";
#       endif
        SetInstance *setInstance = reinterpret_cast<SetInstance*>(paramSet);
        if(!setInstance) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }
        OfxStatus stat = setInstance->editBegin(std::string(name));
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }

      
      static OfxStatus paramEditEnd(OfxParamSetHandle paramSet) {
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << "OFX: paramEditEnd - " << paramSet << " ...";
#       endif
        SetInstance *setInstance = reinterpret_cast<SetInstance*>(paramSet);
        if(!setInstance) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }
        OfxStatus stat = setInstance->editEnd();
#       ifdef OFX_DEBUG_PARAMETERS
        std::cout << ' ' << StatStr(stat) << std::endl;
#       endif
        return stat;
      }
      
      static const OfxParameterSuiteV1 gParamSuiteV1 = {
        paramDefine,
        paramGetHandle,
        paramSetGetPropertySet,
        paramGetPropertySet,
        paramGetValue,
        paramGetValueAtTime,
        paramGetDerivative,
        paramGetIntegral,
        paramSetValue,
        paramSetValueAtTime,
        paramGetNumKeys,
        paramGetKeyTime,
        paramGetKeyIndex,
        paramDeleteKey,
        paramDeleteAllKeys,
        paramCopy,
        paramEditBegin,
        paramEditEnd
      };


      const void *GetSuite(int version) {
        if(version ==1)
          return &gParamSuiteV1;
        return NULL;
      }

    } // Param

  } // Host

} // OFX
