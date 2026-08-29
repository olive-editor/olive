


#ifndef OFXH_PARAM_H
#define OFXH_PARAM_H

#include <string>
#include <map>
#include <list>
#include <cstdarg>

//ofx
#include "ofxParam.h"

//ofxh
#include "ofxhPropertySuite.h"


namespace OFX {

  namespace Host {

    namespace Param {

      /// fetch the param suite
      const void *GetSuite(int version);

      bool isColourParam(const std::string &paramType);

      bool isIntParam(const std::string &paramType);

      /// is this a standard type
      bool isStandardType(const std::string &type);
      
      /// base class for all params
      class Base {

      private:
        Base();
      protected:
        std::string   _paramName;
        std::string   _paramType;
        Property::Set _properties;        
      public:
        Base(const std::string &name, const std::string &type);
        Base(const std::string &name, const std::string &type, const Property::Set &properties);
        virtual ~Base();

        /// grab a handle on the parameter for passing to the C API
        OfxParamHandle getHandle() const;

        virtual bool verifyMagic() { return true; }
        
        /// grab a handle on the properties of this parameter for the C api
        OfxPropertySetHandle getPropHandle() const;

        const Property::Set &getProperties() const;

        Property::Set &getProperties();

        const std::string &getType() const;

        const std::string &getName() const;

        const std::string &getParentName() const;

        const std::string &getLabel() const;

        const std::string &getShortLabel() const;

        const std::string &getLongLabel() const;

        const std::string &getScriptName() const;

        const std::string &getDoubleType() const;

        const std::string &getDefaultCoordinateSystem() const;

        const std::string &getCacheInvalidation() const;

        const std::string &getHint() const;

        bool getEnabled() const;

        bool getCanUndo() const;

        bool getSecret() const;

        bool getIsPersistant() const;

        bool getEvaluateOnChange() const;

        bool getCanAnimate() const;
      };

      /// the Descriptor of a plugin parameter
      class Descriptor : public Base {
        Descriptor();
       
      public:
        /// make a parameter, with the given type and name
        Descriptor(const std::string &type, const std::string &name);
        
        /// add standard param props, will call the below
        void addStandardParamProps(const std::string &type);

        /// add standard properties to a params that can take an interact
        void addInteractParamProps(const std::string &type);

        /// add standard properties to a value holding param
        void addValueParamProps(const std::string &type, Property::TypeEnum valueType, int dim);

        /// add standard properties to a value holding param
        void addNumericParamProps(const std::string &type, Property::TypeEnum valueType, int dim);
      };
      
      /// base class to the param set instance and param set descriptor
      class BaseSet {
      public:
        virtual ~BaseSet();

        /// obtain a handle on this set for passing to the C api
        OfxParamSetHandle getParamSetHandle() const;

        /// get the property handle that lives with the set
        /// The plugin descriptor/instance that derives from
        /// this will provide this.
        virtual Property::Set &getParamSetProps() = 0;
      };

      /// a set of parameters
      class SetDescriptor : public BaseSet {
        std::map<std::string, Descriptor*> _paramMap;
        std::list<Descriptor *> _paramList;
        
        /// CC doesn't exist
        SetDescriptor(const SetDescriptor &);

      public:
        /// default ctor
        SetDescriptor();

        /// dtor
        virtual ~SetDescriptor();

        /// get the map of params
        const std::map<std::string, Descriptor*> &getParams() const;

        /// get the list of params
        const std::list<Descriptor *> &getParamList() const;

        /// define a param
        virtual Descriptor *paramDefine(const char *paramType,
                                        const char *name);

        /// add a param in
        virtual void addParam(const std::string &name, Descriptor *p);
      };

      // forward declare
      class SetInstance;

      /// the description of a plugin parameter
      class Instance : public Base, protected Property::NotifyHook {
        Instance();  
      protected:
        SetInstance*  _paramSetInstance;
        Instance*     _parentInstance;
      public:
        virtual ~Instance();

        /// make a parameter, with the given type and name
        explicit Instance(Descriptor& descriptor, Param::SetInstance* instance = 0);

        //        OfxStatus instanceChangedAction(const std::string &why,
        //                                        OfxTime     time,
        //                                        double      renderScaleX,
        //                                        double      renderScaleY);

        // get the param instance
        OFX::Host::Param::SetInstance* getParamSetInstance() { return _paramSetInstance; }

        // set/get parent instance
        void setParentInstance(Instance* instance);
        Instance* getParentInstance();

        // copy one parameter to another, with a range (NULL means to copy all animation)
        virtual OfxStatus copyFrom(const Instance &instance, OfxTime offset, const OfxRangeD* range);

        // callback which should set enabled state as appropriate
        virtual void setEnabled();

        // callback which should set secret state as appropriate
        virtual void setSecret();

        /// callback which should update label
        virtual void setLabel();

        /// callback which should set range
        virtual void setRange();

        /// callback which should set display range
        virtual void setDisplayRange();

        /// callback which should set evaluate on change
        virtual void setEvaluateOnChange();

        // va list calls below turn the var args (oh what a mistake)
        // suite functions into virtual function calls on instances
        // they are not to be overridden by host implementors by
        // by the various typeed param instances so that they can
        // deconstruct the var args lists

        /// get a value, implemented by instances to deconstruct var args
        virtual OfxStatus getV(va_list arg);

        /// get a value, implemented by instances to deconstruct var args
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// set a value, implemented by instances to deconstruct var args
        virtual OfxStatus setV(va_list arg);

        /// key a value, implemented by instances to deconstruct var args
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// derive a value, implemented by instances to deconstruct var args
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// integrate a value, implemented by instances to deconstruct var args
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);

        /// overridden from Property::NotifyHook
        virtual void notify(const std::string &name, bool single, int num);
      };

      class KeyframeParam {
      public:
        virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
        virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
        virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
        virtual OfxStatus deleteKey(OfxTime time) ;
        virtual OfxStatus deleteAllKeys() ;

        virtual ~KeyframeParam() {
        }
      };

      class GroupInstance : public Instance {
      protected:
        std::vector<Param::Instance*> _children;
      public:
        GroupInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        void setChildren(std::vector<Param::Instance*> children);
        const std::vector<Param::Instance*> &getChildren() const;
      };

      class PageInstance : public Instance {
      public:
        PageInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}
        const std::map<int,Param::Instance*> &getChildren() const;
      protected :
        mutable std::map<int,Param::Instance*> _children; // if set in a notify hook, this need not be mutable
      };

      class IntegerInstance : public Instance, public KeyframeParam {
      public:
        IntegerInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these 
        virtual OfxStatus get(int&) = 0;
        virtual OfxStatus get(OfxTime time, int&) = 0;
        virtual OfxStatus set(int) = 0;
        virtual OfxStatus set(OfxTime time, int) = 0;

        // probably derived class does not need to implement, default is an approximation
        virtual OfxStatus derive(OfxTime time, int&) ;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, int&) ;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };

      class ChoiceInstance : public Instance, public KeyframeParam {
      public:
        ChoiceInstance(Descriptor& descriptor, Param::SetInstance* instance = 0);

        // callback which should set option as appropriate
        virtual void setOption(int num);

        // Deriving implementatation needs to overide these 
        virtual OfxStatus get(int&) = 0;
        virtual OfxStatus get(OfxTime time, int&) = 0;
        virtual OfxStatus set(int) = 0;
        virtual OfxStatus set(OfxTime time, int) = 0;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// overridden from Instance
        virtual void notify(const std::string &name, bool single, int num);
      };

      class DoubleInstance : public Instance, public KeyframeParam {
      public:
        DoubleInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these 
        virtual OfxStatus get(double&) = 0;
        virtual OfxStatus get(OfxTime time, double&) = 0;
        virtual OfxStatus set(double) = 0;
        virtual OfxStatus set(OfxTime time, double) = 0;
        virtual OfxStatus derive(OfxTime time, double&) = 0;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double&) = 0;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };

      class BooleanInstance : public Instance, public KeyframeParam {
      public:
        BooleanInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these
        virtual OfxStatus get(bool&) = 0;
        virtual OfxStatus get(OfxTime time, bool&) = 0;
        virtual OfxStatus set(bool) = 0;
        virtual OfxStatus set(OfxTime time, bool) = 0;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);
      };

      class RGBAInstance : public Instance, public KeyframeParam {
      public:
        RGBAInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these
        virtual OfxStatus get(double&,double&,double&,double&) = 0;
        virtual OfxStatus get(OfxTime time, double&,double&,double&,double&) = 0;
        virtual OfxStatus set(double,double,double,double) = 0;
        virtual OfxStatus set(OfxTime time, double,double,double,double) = 0;

        // derived class does not need to implement, default is an approximation
        virtual OfxStatus derive(OfxTime time, double&,double&,double&,double&) ;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double&,double&,double&,double&) ;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };

      class RGBInstance : public Instance, public KeyframeParam {
      public:
        RGBInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these
        virtual OfxStatus get(double&,double&,double&) = 0;
        virtual OfxStatus get(OfxTime time, double&,double&,double&) = 0;
        virtual OfxStatus set(double,double,double) = 0;
        virtual OfxStatus set(OfxTime time, double,double,double) = 0;

        // derived class does not need to implement, default is an approximation
        virtual OfxStatus derive(OfxTime time, double&,double&,double&) ;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double&,double&,double&) ;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };
        
      class Double2DInstance : public Instance, public KeyframeParam {
      public:
        Double2DInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these
        virtual OfxStatus get(double&,double&) = 0;
        virtual OfxStatus get(OfxTime time, double&,double&) = 0;
        virtual OfxStatus set(double,double) = 0;
        virtual OfxStatus set(OfxTime time, double,double) = 0;

        // derived class does not need to implement, default is an approximation
        virtual OfxStatus derive(OfxTime time, double&,double&) ;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double&,double&) ;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };

      class Integer2DInstance : public Instance, public KeyframeParam {
      public:
        Integer2DInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these
        virtual OfxStatus get(int&,int&) = 0;
        virtual OfxStatus get(OfxTime time, int&,int&) = 0;
        virtual OfxStatus set(int,int) = 0;
        virtual OfxStatus set(OfxTime time, int,int) = 0;

        // derived class does not need to implement, default is an approximation
        virtual OfxStatus derive(OfxTime time, int&,int&) ;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, int&,int&) ;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };

      class Double3DInstance : public Instance , public KeyframeParam{
      public:
        Double3DInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        // Deriving implementatation needs to overide these
        virtual OfxStatus get(double&,double&,double&)  = 0;
        virtual OfxStatus get(OfxTime time, double&,double&,double&)  = 0;
        virtual OfxStatus set(double,double,double)  = 0;
        virtual OfxStatus set(OfxTime time, double,double,double)  = 0;

        // derived class does not need to implement, default is an approximation
        virtual OfxStatus derive(OfxTime time, double&,double&,double&) ;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double&,double&,double&) ;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };

      class Integer3DInstance : public Instance, public KeyframeParam {
      public:
        Integer3DInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        virtual OfxStatus get(int&,int&,int&) = 0;
        virtual OfxStatus get(OfxTime time, int&,int&,int&) = 0;
        virtual OfxStatus set(int,int,int) = 0;
        virtual OfxStatus set(OfxTime time, int,int,int) = 0;

        // derived class does not need to implement, default is an approximation
        virtual OfxStatus derive(OfxTime time, int&,int&,int&) ;
        virtual OfxStatus integrate(OfxTime time1, OfxTime time2, int&,int&,int&) ;

        /// implementation of var args function
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus deriveV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus integrateV(OfxTime time1, OfxTime time2, va_list arg);
      };

      class StringInstance : public Instance, public KeyframeParam {
        std::string _returnValue; ///< location to hold temporary return value. Should delegate this to implementation!!!
      public:
        StringInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}

        virtual OfxStatus get(std::string &) = 0;
        virtual OfxStatus get(OfxTime time, std::string &) = 0;
        virtual OfxStatus set(const char*) = 0;
        virtual OfxStatus set(OfxTime time, const char*) = 0;

        /// implementation of var args function
        /// Be careful: the char* is only valid until next API call
        /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
        virtual OfxStatus getV(va_list arg);

        /// implementation of var args function
        /// Be careful: the char* is only valid until next API call
        /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
        virtual OfxStatus getV(OfxTime time, va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(va_list arg);

        /// implementation of var args function
        virtual OfxStatus setV(OfxTime time, va_list arg);
      };

      class CustomInstance : public StringInstance {
      public:
        CustomInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : StringInstance(descriptor,instance) {}
      };

      class PushbuttonInstance : public Instance, public KeyframeParam {
      public:
        PushbuttonInstance(Descriptor& descriptor, Param::SetInstance* instance = 0) : Instance(descriptor,instance) {}
      };

      /// A set of parameters
      ///
      /// As we are the owning object we delete the params inside ourselves. It was tempting
      /// to make params autoref objects and have shared ownership with the client code
      /// but that adds complexity for no strong gain.
      class SetInstance : public BaseSet {
      protected:
        std::map<std::string, Instance*> _params;        ///< params by name
        std::list<Instance *>            _paramList;     ///< params list

      public :
        /// ctor
        ///
        /// The propery set being passed in belongs to the owning 
        /// plugin instance.
        explicit SetInstance();

        /// dtor. 
        virtual ~SetInstance();

        /// get the params
        const std::map<std::string, Instance*> &getParams() const;

        /// get the params
        const std::list<Instance*> &getParamList() const;

        // get the param
        Instance* getParam(const std::string &name) const {
          std::map<std::string,Instance*>::const_iterator it = _params.find(name);
          if(it!=_params.end())
            return it->second;
          else
            return 0;
        }

        /// The inheriting plugin instance needs to set this up to deal with 
        /// plug-ins changing their own values.
        virtual void paramChangedByPlugin(Param::Instance *param) = 0;

        /// add a param
        virtual OfxStatus addParam(const std::string& name, Instance* instance);

        /// make a parameter instance
        ///
        /// Client host code needs to implement this
        virtual Instance* newParam(const std::string& name, Descriptor& Descriptor) = 0;        

        /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditBegin
        ///
        /// Client host code needs to implement this
        virtual OfxStatus editBegin(const std::string& name) = 0;

        /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditEnd
        ///
        /// Client host code needs to implement this
        virtual OfxStatus editEnd() = 0;

      };
    }
  }
}

#endif // OFXH_PARAM_H
