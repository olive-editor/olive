

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhUtilities.h"

#include <iostream>
#include <string.h>

namespace OFX {
  namespace Host {
    namespace Property {

      /// type holder, for integers
      int IntValue::kEmpty = 0;
      double DoubleValue::kEmpty = 0;
      void *PointerValue::kEmpty = 0;
      std::string StringValue::kEmpty;
      const char *gTypeNames[] = {"int", "double", "string", "pointer" };

      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> int GetHook::getProperty<IntValue>(const std::string &name, int index) const
      {
        return getIntProperty(name, index);
      }

      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> double GetHook::getProperty<DoubleValue>(const std::string &name, int index) const
      {
        return getDoubleProperty(name, index);
      }
      
      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> void *GetHook::getProperty<PointerValue>(const std::string &name, int index) const
      {
        return getPointerProperty(name, index);
      }

      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> const std::string &GetHook::getProperty<StringValue>(const std::string &name, int index) const
      {
        return getStringProperty(name, index);
      }
            
      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> void GetHook::getPropertyN<IntValue>(const std::string &name, int *values, int count) const
      {
        getIntPropertyN(name, values, count);
      }

      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> void GetHook::getPropertyN<DoubleValue>(const std::string &name, double *values, int count) const
      {
        getDoublePropertyN(name, values, count);
      }
      
      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> void GetHook::getPropertyN<PointerValue>(const std::string &name, void **values, int count) const
      {
        getPointerPropertyN(name, values, count);
      }
        
      /// this does some magic so that it calls get string/int/double/pointer appropriately
      template<> void GetHook::getPropertyN<StringValue>(const std::string &name, const char **values, int count) const
      {
        getStringPropertyN(name, values, count);
      }


      /// override this to get a single value at the given index.
      const std::string &GetHook::getStringProperty(const std::string &/*name*/, int /*index*/) const
      {        
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getStringProperty!!!! " << std::endl;
#       endif
        return StringValue::kEmpty;
      }
       
       /// override this function to optimize multiple-string properties fetching
      void GetHook::getStringPropertyN(const std::string &name, const char** values, int count) const
      {
        for (int i = 0; i < count; ++i) {
          values[i] = getStringProperty(name, i).c_str();
        }
      }
      
      /// override this to fetch a single value at the given index.
      int GetHook::getIntProperty(const std::string &/*name*/, int /*index*/) const
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getIntProperty!!!! " << std::endl;
#       endif
        return 0;
      }
      
      /// override this to fetch a single value at the given index.
      double GetHook::getDoubleProperty(const std::string &/*name*/, int /*index*/) const
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getDoubleProperty!!!! " << std::endl;
#       endif
        return 0;
      }
      
      /// override this to fetch a single value at the given index.
      void *GetHook::getPointerProperty(const std::string &/*name*/, int /*index*/) const
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getPointerProperty!!!! " << std::endl;
#       endif
        return NULL;
      }
      
      /// override this to fetch a multiple values in a multi-dimension property
      void GetHook::getDoublePropertyN(const std::string &/*name*/, double *values, int count) const
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getDoublePropertyN!!!! " << std::endl;
#       endif
        memset(values, 0, sizeof(double) * count);
      }

      /// override this to fetch a multiple values in a multi-dimension property
      void GetHook::getIntPropertyN(const std::string &/*name*/, int *values, int count) const
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getIntPropertyN!!!! " << std::endl;
#       endif
        memset(values, 0, sizeof(int) * count);
      }

      /// override this to fetch a multiple values in a multi-dimension property
      void GetHook::getPointerPropertyN(const std::string &/*name*/, void **values, int count) const
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getPointerPropertyN!!!! " << std::endl;
#       endif
        memset(values, 0, sizeof(void *) * count);
      }


      /// override this to fetch the dimension size.
      int GetHook::getDimension(const std::string &/*name*/) const
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::getDimension!!!! " << std::endl;
#       endif
        return 0;
      }
      
      /// override this to handle a reset(). 
      void GetHook::reset(const std::string &/*name*/)
      {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: Calling un-overriden GetHook::reset!!!! " << std::endl;
#       endif
      }

      Property::Property(const std::string &name,
                         TypeEnum type,
                         int dimension,
                         bool pluginReadOnly)
        : _name(name)
        , _type(type)
        , _dimension(dimension)
        , _pluginReadOnly(pluginReadOnly) 
        , _getHook(0)          
      {
      }

      Property::Property(const Property &other)
        : _name(other._name)
        , _type(other._type)
        , _dimension(other._dimension)
        , _pluginReadOnly(other._pluginReadOnly) 
        , _getHook(0)          
      {
      }
      
      /// call notify on the contained notify hooks
      void Property::notify(bool single, int indexOrN)
      {
        std::vector<NotifyHook *>::iterator i;
        for(i = _notifyHooks.begin(); i != _notifyHooks.end(); ++i) {
          (*i)->notify(_name,  single, indexOrN);
        }
      }

      inline int castToAPIType(int i) { return i; }
      inline void *castToAPIType(void *v) { return v; }
      inline double castToAPIType(double d) { return d; }
      inline const char *castToAPIType(const std::string &s) { return s.c_str(); }

      template<class T> PropertyTemplate<T>::PropertyTemplate(const std::string &name,					    
        int dimension,													    
        bool /*pluginReadOnly*/,
        APIType defaultValue)												    
        : Property(name, T::typeCode, dimension)									    
      {															    
															    
        if (dimension) {												    
          _value.resize(dimension);											    
          _defaultValue.resize(dimension);										    
        }														    
															    
        if (dimension) {												    
          for (int i=0;i<dimension;i++) {										    
            _defaultValue[i] = defaultValue;										    
            _value[i] = defaultValue;											    
          }														    
        }														    
      }															    
															    
      template<class T> PropertyTemplate<T>::PropertyTemplate(const PropertyTemplate<T> &pt)				    
        : Property(pt)													    
        , _value(pt._value)												    
        , _defaultValue(pt._defaultValue)										    
      {															    
      }

#ifdef WINDOWS
#pragma warning( disable : 4181 )
#endif
      /// get one value
      template<class T> 
      const typename T::ReturnType PropertyTemplate<T>::getValue(int index) const 
      {
        if (_getHook) {
          return _getHook->getProperty<T>(_name, index);
        } 
        else {
          return getValueRaw(index);
        }
      }
#ifdef WINDOWS
#pragma warning( default : 4181 )
#endif
      // get multiple values
      template<class T> 
      void PropertyTemplate<T>::getValueN(typename T::APIType *values, int count) const {
        if (_getHook) {
          _getHook->getPropertyN<T>(_name, values, count);
        } 
        else {
          getValueNRaw(values, count);
        }
      }

#ifdef WINDOWS
#pragma warning( disable : 4181 )
#endif
      /// get one value, without going through the getHook
      template<class T> 
      const typename T::ReturnType PropertyTemplate<T>::getValueRaw(int index) const
      {
        if (index < 0 || ((size_t)index >= _value.size())) {
          throw Exception(kOfxStatErrBadIndex);
        }
        return _value[index];
      }
#ifdef WINDOWS
#pragma warning( default : 4181 )
#endif      
      // get multiple values, without going through the getHook
      template<class T> 
      void PropertyTemplate<T>::getValueNRaw(APIType *value, int count) const
      {
        size_t size = count;
        if (size > _value.size()) {
          size = _value.size();
        }

        for (size_t i=0;i<size;i++) {
          value[i] = castToAPIType(_value[i]);
        }
      }

      /// set one value
      template<class T> void PropertyTemplate<T>::setValue(const typename T::Type &value, int index) 
      {
        if (index < 0 || ((size_t)index > _value.size() && _dimension)) {
          throw Exception(kOfxStatErrBadIndex);
        }

        if (_value.size() <= (size_t)index) {
          _value.resize(index+1);
        }
        _value[index] = value;

        notify(true, index);
      }

      /// set multiple values
      template<class T> void PropertyTemplate<T>::setValueN(const typename T::APIType *value, int count)
      {
        if (_dimension && ((size_t)count > _value.size())) {
          throw Exception(kOfxStatErrBadIndex);              
        }
        if (_value.size() != (size_t)count) {
          _value.resize(count);
        }
        for (int i=0;i<count;i++) {
          _value[i] = value[i];
        }
        
        notify(false, count);
      }
        
      /// get the dimension of the property
      template <class T> int PropertyTemplate<T>::getDimension() const {
        if (_dimension != 0) {
          return _dimension;
        } 
        else {
          // code to get it from the hook
          if (_getHook) {
            return _getHook->getDimension(_name);
          } 
          else {
            return (int)_value.size();
          }
        }
      }

      template <class T> void PropertyTemplate<T>::reset() 
      {
        if (_getHook) {
          _getHook->reset(_name);
          int dim = getDimension();

          if(!isFixedSize()) {
            _value.resize(dim);
          }
          for(int i = 0; i < dim; ++i) {
            _value[i] = _getHook->getProperty<T>(_name, i);
          }
        } 
        else {
          if(isFixedSize()) {
            _value = _defaultValue;
          } 
          else {
            _value.resize(0);
          }

          // now notify on a reset
          notify(false, _dimension);
        }
      }

      // explicit instanciations (required by ofxhPluginAPICache.cpp)
      template class PropertyTemplate<IntValue>;
      template class PropertyTemplate<DoubleValue>;
      template class PropertyTemplate<PointerValue>;
      template class PropertyTemplate<StringValue>;

      inline int castAwayConst(int i) { return i; }
      inline double castAwayConst(double d) { return d; }
      inline void *castAwayConst(void *v) { return v; }
      inline char *castAwayConst(const char *s) { return const_cast<char*>(s); }

      inline int *castToConst(int *i) { return i; }
      inline double *castToConst(double *d) { return d; }
      inline const char **castToConst(char **s) { return const_cast<const char**>(s); }
      inline void **castToConst(void **v) { return v; }


      void Set::setGetHook(const std::string &s, GetHook *ghook) const
      {
        Property *prop = fetchProperty(s);

        if(prop) {
          prop->setGetHook(ghook);
        }
      }


      /// add a notify hook for a particular property.  users may need to call particular
      /// specialised versions of this.
      void Set::addNotifyHook(const std::string &s, NotifyHook *hook) const
      {
        Property *prop = fetchProperty(s);

        if(prop) {
          prop->addNotifyHook(hook);
        }
      }

      Property *Set::fetchProperty(const std::string&name, bool followChain) const
      {
        PropertyMap::const_iterator i = _props.find(name);
        if (i == _props.end()) {
          if(followChain && _chainedSet) {
            return _chainedSet->fetchProperty(name, true);
          }
          return NULL;
        }
        return i->second;
      }

      template<class T> bool Set::fetchTypedProperty(const std::string&name, T *&prop, bool followChain) const
      {
        Property *myprop = fetchProperty(name, followChain);

        if(!myprop)
          return false;

        prop = dynamic_cast<T *>(myprop);
        if (prop == 0) {
          return false;
        }
        return true;
      }

      String *Set::fetchStringProperty(const std::string &name, bool followChain) const {
        String *p;
        if (fetchTypedProperty(name, p, followChain)) {
          return p;
        }
        return NULL;
      }

      Int *Set::fetchIntProperty(const std::string &name, bool followChain) const {
        Int *p;
        if (fetchTypedProperty(name, p, followChain)) {
          return p;
        }
        return NULL;
      }

      Pointer *Set::fetchPointerProperty(const std::string &name,  bool followChain) const {
        Pointer *p;
        if (fetchTypedProperty(name, p, followChain)) {
          return p;
        }
        return NULL;
      }

      Double *Set::fetchDoubleProperty(const std::string &name, bool followChain) const {
        Double *p;
        if (fetchTypedProperty(name, p, followChain)) {
          return p;
        }
        return NULL;
      }

      /// add one new property
      void Set::createProperty(const PropSpec &spec)
      {
        if (_props.find(spec.name) != _props.end()) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << "OFX: Tried to add a duplicate property to a Property::Set: " << spec.name << std::endl;
#         endif
          return;
        }

        switch (spec.type) {
        case eInt: 
          _props[spec.name] = new Int(spec.name, spec.dimension, spec.readonly, spec.defaultValue?atoi(spec.defaultValue):0);
          break;
        case eDouble: 
          _props[spec.name] = new Double(spec.name, spec.dimension, spec.readonly, spec.defaultValue?atof(spec.defaultValue):0);
          break;
        case eString: 
          _props[spec.name] = new String(spec.name, spec.dimension, spec.readonly, spec.defaultValue?spec.defaultValue:"");
          break;
        case ePointer: 
          _props[spec.name] = new Pointer(spec.name, spec.dimension, spec.readonly, (void*)spec.defaultValue);
          break;
        default: // XXX  error - unrecognised type
          break;
        }
      }

      void Set::addProperties(const PropSpec spec[]) 
      {
        while (spec->name) {
          createProperty(*spec);          
          spec++;
        }
      }

      /// add one new property
      void Set::addProperty(Property *prop)
      {
        PropertyMap::iterator t = _props.find(prop->getName());
        if(t != _props.end())
           delete t->second;
        _props[prop->getName()] = prop;
      }

      /// empty ctor
      Set::Set()
        : _magic(kMagic)
        , _chainedSet(NULL) 
      {
      }

      Set::Set(const PropSpec spec[])
        : _magic(kMagic)
        , _chainedSet(NULL) 
      {
        addProperties(spec);
      }

      Set::Set(const Set &other) 
        : _magic(kMagic)
        , _chainedSet(NULL) 
      {
        bool failed = false;

        for (std::map<std::string, Property *>::const_iterator i = other._props.begin();
             i != other._props.end();
             i++) 
          {
            Property *copyProp = i->second->deepCopy();
            if (!copyProp) {
              failed = true;
              break;
            }
            _props[i->first] = copyProp;
          }
        
        if (failed)
          
          for (std::map<std::string, Property *>::iterator j = _props.begin();
               j != _props.end();
               j++) {
            delete j->second;
          }
        
      }

      Set::~Set()
      {
        std::map<std::string, Property *>::iterator i = _props.begin();
        while (i != _props.end()) {
          delete i->second;
          i++;
        }
      }

      /// set a particular property
      template<class T> void Set::setProperty(const std::string &property, int index, const typename T::Type &value) 
      {
        try {
          PropertyTemplate<T> *prop = 0;
          if(fetchTypedProperty(property, prop)) {
            prop->setValue(value, index);
          }
          else {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << "OFX: setProperty " << property << "[" << index << "] ignored because host property not defined" << std::endl;
#           endif
          }
        }
        catch(...) {}
      }
      
      /// set a particular property
      template<class T> void Set::setPropertyN(const std::string &property, int count, const typename T::APIType *value) 
      {
        try {
          PropertyTemplate<T> *prop = 0;
          if(fetchTypedProperty(property, prop)) {
            prop->setValueN(value, count);
          }
          else {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << "OFX: setPropertyN " << property << " ignored because host property not defined" << std::endl;
#           endif
          }
        }
        catch(...) {}
      }
      
      /// get a particular property
      template<class T> typename T::ReturnType Set::getProperty(const std::string &property, int index)  const
      {
        try {
          PropertyTemplate<T> *prop;
          if(fetchTypedProperty(property, prop, true)) {
            return prop->getValue(index);
          }
        }
        catch(...) {}
        return T::kEmpty;
      }

      /// get a particular property
      template<class T> void Set::getPropertyN(const std::string &property, int count,  typename T::APIType *value)  const
      {
        try {
          PropertyTemplate<T> *prop;
          if(fetchTypedProperty(property, prop, true)) {
            return prop->getValueN(value, count);
          }
        }
        catch(...) {}
      }
      
      /// get a particular property
      template<class T> typename T::ReturnType Set::getPropertyRaw(const std::string &property, int index)  const
      {
        try {
          PropertyTemplate<T> *prop;
          if(fetchTypedProperty(property, prop, true)) {
            return prop->getValueRaw(index);
          }
        }
        catch(...) {}
        return T::kEmpty;
      }
      
      /// get a particular property
      template<class T> void Set::getPropertyRawN(const std::string &property, int count,  typename T::APIType *value)  const
      {
        try {
          PropertyTemplate<T> *prop;
          if(fetchTypedProperty(property, prop, true)) {
            return prop->getValueNRaw(value, count);
          }
        }
        catch(...) {}
      }
      
      /// get a particular int property
      int Set::getIntPropertyRaw(const std::string &property, int index) const
      {
        return getPropertyRaw<OFX::Host::Property::IntValue>(property, index);
      }
        
      /// get a particular double property
      double Set::getDoublePropertyRaw(const std::string &property, int index)  const
      {
        return getPropertyRaw<OFX::Host::Property::DoubleValue>(property, index);
      }

      /// get a particular double property
      void *Set::getPointerPropertyRaw(const std::string &property, int index)  const
      {
        return getPropertyRaw<OFX::Host::Property::PointerValue>(property, index);
      }
        
      /// get a particular double property
      const std::string &Set::getStringPropertyRaw(const std::string &property, int index)  const
      {
        String *prop;
        if(fetchTypedProperty(property, prop, true)) {
          return prop->getValueRaw(index);
        }
        return StringValue::kEmpty;
      }

      /// get a particular int property
      int Set::getIntProperty(const std::string &property, int index)  const
      {
        return getProperty<OFX::Host::Property::IntValue>(property, index);
      }
        
      /// get the value of a particular double property
      void Set::getIntPropertyN(const std::string &property,  int *v, int N) const
      {
        return getPropertyN<OFX::Host::Property::IntValue>(property, N, v);
      }

      /// get a particular double property
      double Set::getDoubleProperty(const std::string &property, int index)  const
      {
        return getProperty<OFX::Host::Property::DoubleValue>(property, index);
      }

      /// get the value of a particular double property
      void Set::getDoublePropertyN(const std::string &property,  double *v, int N) const
      {
        return getPropertyN<OFX::Host::Property::DoubleValue>(property, N, v);
      }

      /// get a particular double property
      void *Set::getPointerProperty(const std::string &property, int index)  const
      {
        return getProperty<OFX::Host::Property::PointerValue>(property, index);
      }
        
      /// get a particular double property
      const std::string &Set::getStringProperty(const std::string &property, int index)  const
      {
        return getProperty<OFX::Host::Property::StringValue>(property, index);
      }
      
      /// set a particular string property
      void Set::setStringProperty(const std::string &property, const std::string &value, int index)
      {
        setProperty<OFX::Host::Property::StringValue>(property, index, value);
      }
      
      /// get a particular int property
      void Set::setIntProperty(const std::string &property, int v, int index)
      {
        setProperty<OFX::Host::Property::IntValue>(property, index, v);
      }
      
      /// get a particular double property
      void Set::setIntPropertyN(const std::string &property, const int *v, int N)
      {
        setPropertyN<OFX::Host::Property::IntValue>(property, N, v);
      }

      /// get a particular double property
      void Set::setDoubleProperty(const std::string &property, double v, int index)
      {
        setProperty<OFX::Host::Property::DoubleValue>(property, index, v);
      }
      
      /// get a particular double property
      void Set::setDoublePropertyN(const std::string &property, const double *v, int N)
      {
        setPropertyN<OFX::Host::Property::DoubleValue>(property, N, v);
      }

      /// get a particular double property
      void Set::setPointerProperty(const std::string &property,  void *v, int index)
      {
        setProperty<OFX::Host::Property::PointerValue>(property, index, v);
      }
        
      /// get the dimension of a particular property
      int Set::getDimension(const std::string &property) const
      {
        Property *prop = 0;
        if(fetchTypedProperty(property, prop, true)) {
          return  prop->getDimension();
        }
        return 0;
      }

      /// is the given string one of the values of a multi-dimensional string prop
      /// this returns a non negative index if it is found, otherwise, -1
      int Set::findStringPropValueIndex(const std::string &propName,
                                        const std::string &propValue) const
      {
        String *prop = fetchStringProperty(propName, true);
        
        if(prop) {
          const std::vector<std::string> &values = prop->getValues();
          std::vector<std::string>::const_iterator i = find(values.begin(), values.end(), propValue);
          if(i != values.end()) {
            return int(i - values.begin());
          }
        }
        return -1;
      }
      
      /// static functions for the suite
      template<class T> static OfxStatus propSet(OfxPropertySetHandle properties,
                                                 const char *property,
                                                 int index,
                                                 typename T::APIType value) {          
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: propSet - " << properties << ' ' << property << "[" << index << "] = " << value << " ...";
#       endif
        try {            
          Set *thisSet = reinterpret_cast<Set*>(properties);
          if(!thisSet || !thisSet->verifyMagic()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
          }
          PropertyTemplate<T> *prop = 0;
          if(!thisSet->fetchTypedProperty(property, prop, false)) {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << "OFX: propSet " << property << "[" << index << "] ignored because effect property not defined" << std::endl;
            std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#           endif
            return kOfxStatErrUnknown;
          }
          prop->setValue(value, index);
        } catch (const Exception& e) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(e.getStatus()) << std::endl;
#         endif
          return e.getStatus();
        } catch (...) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#         endif
          return kOfxStatErrUnknown;
        }
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#       endif
        return kOfxStatOK;
      }
      
      /// static functions for the suite
      template<class T> static OfxStatus propSetN(OfxPropertySetHandle properties,
                                                const char *property,
                                                int count,
                                                const typename T::APIType *values) {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: propSetN - " << properties << ' ' << property << "[0.." << count-1 << "] = ";
        for (int i = 0; i < count; ++i) {
            if (i != 0) {
                std::cout << ',';
            }
            std::cout << values[i];
        }
#       endif
        try {
          Set *thisSet = reinterpret_cast<Set*>(properties);
          if(!thisSet || !thisSet->verifyMagic()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
          }
          PropertyTemplate<T> *prop = 0;
          if(!thisSet->fetchTypedProperty(property, prop, false)) {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << "OFX: propSetN " << property << " ignored because effect property not defined" << std::endl;
            std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#           endif
            return kOfxStatErrUnknown;
          }
          prop->setValueN(values, count);
        } catch (const Exception& e) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(e.getStatus()) << std::endl;
#         endif
          return e.getStatus();
        } catch (...) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#         endif
          return kOfxStatErrUnknown;
        }
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#       endif
        return kOfxStatOK;
      }
      
      /// static functions for the suite
      template<class T> static OfxStatus propGet(OfxPropertySetHandle properties,
                                               const char *property,
                                               int index,
                                               typename T::APITypeConstless *value) {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: propGet - " << properties << ' ' << property << "[" << index << "] = ...";
#       endif
        try {
          Set *thisSet = reinterpret_cast<Set*>(properties);
          if(!thisSet || !thisSet->verifyMagic()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
          }
          PropertyTemplate<T> *prop = 0;
          if(!thisSet->fetchTypedProperty(property, prop, true)) {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#           endif
            return kOfxStatErrUnknown;
          }
          *value = castAwayConst(castToAPIType(prop->getValue(index)));

#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << *value << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
        } catch (const Exception& e) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(e.getStatus()) << std::endl;
#         endif
          return e.getStatus();
        } catch (...) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#         endif
          return kOfxStatErrUnknown;
        }
        return kOfxStatOK;
      }
      
      /// static functions for the suite
      template<class T> static OfxStatus propGetN(OfxPropertySetHandle properties,
                                            const char *property,
                                            int count,
                                            typename T::APITypeConstless *values) {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: propGetN - " << properties << ' ' << property << "[0.." << count-1 << "] = ...";
#       endif
        try {
          Set *thisSet = reinterpret_cast<Set*>(properties);
          if(!thisSet || !thisSet->verifyMagic()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
          }
          PropertyTemplate<T> *prop = 0;
          if(!thisSet->fetchTypedProperty(property, prop, true)) {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#           endif
            return kOfxStatErrUnknown;
          }
          prop->getValueN(castToConst(values), count);
#         ifdef OFX_DEBUG_PROPERTIES
          for (int i = 0; i < count; ++i) {
            if (i != 0) {
                std::cout << ',';
            }
            std::cout << values[i];
          }
          std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
        } catch (const Exception& e) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(e.getStatus()) << std::endl;
#         endif
          return e.getStatus();
        } catch (...) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#         endif
          return kOfxStatErrUnknown;
        }
        return kOfxStatOK;
      }
      
      /// static functions for the suite
      static OfxStatus propReset(OfxPropertySetHandle properties, const char *property) {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: propReset - " << properties << ' ' << property << " ...";
#       endif
        try {            
          Set *thisSet = reinterpret_cast<Set*>(properties);
          if(!thisSet || !thisSet->verifyMagic()) {
#           ifdef OFX_DEBUG_PARAMETERS
            std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#           endif
            return kOfxStatErrBadHandle;
          }
          Property *prop = thisSet->fetchProperty(property, false);
          if(!prop) {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#           endif
            return kOfxStatErrUnknown;
          }
          prop->reset();
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
        } catch (const Exception& e) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(e.getStatus()) << std::endl;
#         endif
          return e.getStatus();
        } catch (...) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#         endif
          return kOfxStatErrUnknown;
        }
        return kOfxStatOK;
      }
      
      /// static functions for the suite
      static OfxStatus propGetDimension(OfxPropertySetHandle properties, const char *property, int *count) {
#       ifdef OFX_DEBUG_PROPERTIES
        std::cout << "OFX: propGetDimension - " << properties << ' ' << property << " ...";
#       endif
        if (!properties) {
#         ifdef OFX_DEBUG_PARAMETERS
          std::cout << ' ' << StatStr(kOfxStatErrBadHandle) << std::endl;
#         endif
          return kOfxStatErrBadHandle;
        }
        try {            
          Set *thisSet = reinterpret_cast<Set*>(properties);
          Property *prop = thisSet->fetchProperty(property, true);
          if(!prop) {
#           ifdef OFX_DEBUG_PROPERTIES
            std::cout << "unknown property\n";
#           endif
            return kOfxStatErrUnknown;
          }
          *count = prop->getDimension();
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << *count << ' ' << StatStr(kOfxStatOK) << std::endl;
#         endif
          return kOfxStatOK;
        } catch (const Exception& e) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(e.getStatus()) << std::endl;
#         endif
          return e.getStatus();
        } catch (...) {
#         ifdef OFX_DEBUG_PROPERTIES
          std::cout << ' ' << StatStr(kOfxStatErrUnknown) << std::endl;
#         endif
          return kOfxStatErrUnknown;
        }
      }

      /// the actual suite that is passed across the API to manage properties
      struct OfxPropertySuiteV1 gSuite = {
        propSet<PointerValue>,
        propSet<StringValue>,
        propSet<DoubleValue>,
        propSet<IntValue>,
        propSetN<PointerValue>,
        propSetN<StringValue>,
        propSetN<DoubleValue>,
        propSetN<IntValue>,
        propGet<PointerValue>,
        propGet<StringValue>,
        propGet<DoubleValue>,
        propGet<IntValue>,
        propGetN<PointerValue>,
        propGetN<StringValue>,
        propGetN<DoubleValue>,
        propGetN<IntValue>,
        propReset,
        propGetDimension
      };     
      
      /// return the OFX function suite that manages properties
      const void *GetSuite(int version)
      {
        if(version == 1)
          return (void *)(&gSuite);
        return NULL;
      }

    }
  }
}
