// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

#include <math.h>

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhImageEffect.h"
#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include "ofxhHost.h"
#include "ofxhImageEffectAPI.h"
#include "ofxhUtilities.h"
#ifdef OFX_SUPPORTS_PARAMETRIC
#include "ofxhParametricParam.h"
#endif
#ifdef OFX_SUPPORTS_OPENGLRENDER
#include "ofxGPURender.h"
#endif
#include "ofxOld.h" // old plugins may rely on deprecated properties being present

#include <string.h>
#include <stdarg.h>
#include <iostream>

namespace OFX {

  namespace Host {

    namespace ImageEffect {
      
      /// properties common on an effect and a descriptor
      static const Property::PropSpec effectDescriptorStuff[] = {
        /* name                                 type                   dim. r/o default value */
        { kOfxPropType,                         Property::eString,     1, true,  kOfxTypeImageEffect },
        { kOfxPropLabel,                        Property::eString,     1, false, "" },
        { kOfxPropShortLabel,                   Property::eString,     1, false, "" },
        { kOfxPropLongLabel,                    Property::eString,     1, false, "" },
        { kOfxPropVersion,                      Property::eInt,        0, false, "0" },
        { kOfxPropVersionLabel,                 Property::eString,     1, false, "" },
        { kOfxPropPluginDescription,            Property::eString,     1, false, "" },
        { kOfxImageEffectPropSupportedContexts, Property::eString,     0, false, "" },
        { kOfxImageEffectPluginPropGrouping,    Property::eString,     1, false, "" },
        { kOfxImageEffectPluginPropSingleInstance, Property::eInt,     1, false, "0" },
        { kOfxImageEffectPluginRenderThreadSafety, Property::eString,  1, false, kOfxImageEffectRenderInstanceSafe },
        { kOfxImageEffectPluginPropHostFrameThreading, Property::eInt, 1, false, "1" },
        { kOfxImageEffectPluginPropOverlayInteractV1, Property::ePointer, 1, false, NULL },
        { kOfxImageEffectPropSupportsMultiResolution, Property::eInt,  1, false, "1" } ,
        { kOfxImageEffectPropSupportsTiles,     Property::eInt,        1, false, "1" }, 
        { kOfxImageEffectPropTemporalClipAccess, Property::eInt,       1, false, "0" },
        { kOfxImageEffectPropSupportedPixelDepths, Property::eString,  0, false, "" }, 
        { kOfxImageEffectPluginPropFieldRenderTwiceAlways, Property::eInt, 1, false, "1" } ,
        { kOfxImageEffectPropSupportsMultipleClipDepths, Property::eInt, 1, false, "0" },
        { kOfxImageEffectPropSupportsMultipleClipPARs,   Property::eInt, 1, false, "0" },
        { kOfxImageEffectPropClipPreferencesSlaveParam, Property::eString, 0, false, "" },
        { kOfxImageEffectInstancePropSequentialRender, Property::eInt, 1, false, "0" },
        { kOfxPluginPropFilePath, Property::eString, 1, true, ""},
#ifdef OFX_SUPPORTS_OPENGLRENDER
        { kOfxImageEffectPropOpenGLRenderSupported, Property::eString, 1, false, "false"}, // OFX 1.3
	{ kOfxImageEffectPropCudaRenderSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropCudaStreamSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropMetalRenderSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropOpenCLRenderSupported, Property::eString, 1, false, "false" },
#endif

        Property::propSpecEnd
      };

      //
      // Base
      //

      Base::Base(const Property::Set &set) 
        : _properties(set) 
      {}

      Base::Base(const Property::PropSpec * propSpec)
        : _properties(propSpec) 
      {}

      Base::~Base() {}

      /// obtain a handle on this for passing to the C api
      OfxImageEffectHandle Base::getHandle() const {
        return (OfxImageEffectHandle)this;
      }
      
      /// get the properties set
      Property::Set &Base::getProps() {
        return _properties;
      }

      /// get the properties set, const version
      const Property::Set &Base::getProps() const {
        return _properties;
      }

      /// name of the clip
      const std::string &Base::getShortLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropShortLabel);
        if(s == "") {
          const std::string &s2 = _properties.getStringProperty(kOfxPropLabel);
          if(s2 == "") {
            return _properties.getStringProperty(kOfxPropName);
          }
        }
        return s;
      }
        
      /// name of the clip
      const std::string &Base::getLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropLabel);
        if(s == "") {
          return _properties.getStringProperty(kOfxPropName);
        }
        return s;
      }
        
      /// name of the clip
      const std::string &Base::getLongLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropLongLabel);
        if(s == "") {
          const std::string &s2 = _properties.getStringProperty(kOfxPropLabel);
          if(s2 == "") {
            return _properties.getStringProperty(kOfxPropName);
          }
        }
        return s;
      }

      /// is the given context supported
      bool Base::isContextSupported(const std::string &s) const
      {
        return _properties.findStringPropValueIndex(kOfxImageEffectPropSupportedContexts, s) != -1;
      }

      /// what is the name of the group the plug-in belongs to
      const std::string &Base::getPluginGrouping() const
      {
        return _properties.getStringProperty(kOfxImageEffectPluginPropGrouping);
      }

      /// is the effect single instance
      bool Base::isSingleInstance() const
      {
        return _properties.getIntProperty(kOfxImageEffectPluginPropSingleInstance) != 0;
      }

      /// what is the thread safety on this effect
      const std::string &Base::getRenderThreadSafety() const
      {
        return _properties.getStringProperty(kOfxImageEffectPluginRenderThreadSafety);
      }

      /// should the host attempt to managed multi-threaded rendering if it can
      /// via tiling or some such
      bool Base::getHostFrameThreading() const
      {
        return _properties.getIntProperty(kOfxImageEffectPluginPropHostFrameThreading) != 0;
      }

      /// get the overlay interact main entry if it exists
      OfxPluginEntryPoint *Base::getOverlayInteractMainEntry() const
      {
        return (OfxPluginEntryPoint *)(_properties.getPointerProperty(kOfxImageEffectPluginPropOverlayInteractV1));
      }

      /// does the effect support images of differing sizes
      bool Base::supportsMultiResolution() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropSupportsMultiResolution) != 0;
      }

      /// does the effect support tiled rendering
      bool Base::supportsTiles() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropSupportsTiles) != 0;
      }

      /// does this effect need random temporal access
      bool Base::temporalAccess() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropTemporalClipAccess) != 0;
      }

      /// is the given RGBA/A pixel depth supported by the effect
      bool Base::isPixelDepthSupported(const std::string &s) const
      {
        return _properties.findStringPropValueIndex(kOfxImageEffectPropSupportedPixelDepths, s) != -1;
      }

      /// when field rendering, does the effect need to be called
      /// twice to render a frame in all Base::circumstances (with different fields)
      bool Base::fieldRenderTwiceAlways() const
      {
        return _properties.getIntProperty(kOfxImageEffectPluginPropFieldRenderTwiceAlways) != 0;
      }
        
      /// does the effect support multiple clip depths
      bool Base::supportsMultipleClipDepths() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropSupportsMultipleClipDepths) != 0;
      }
        
      /// does the effect support multiple clip pixel aspect ratios
      bool Base::supportsMultipleClipPARs() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropSupportsMultipleClipPARs) != 0;
      }
        
      /// does changing the named param re-tigger a clip preferences action
      bool Base::isClipPreferencesSlaveParam(const std::string &s) const
      {
        return _properties.findStringPropValueIndex(kOfxImageEffectPropClipPreferencesSlaveParam, s) != -1;
      }


      ////////////////////////////////////////////////////////////////////////////////
      // descriptor

      Descriptor::Descriptor(Plugin *plug) 
        : Base(effectDescriptorStuff)
        , _plugin(plug)
      {
        _properties.setStringProperty(kOfxPluginPropFilePath, plug->getBinary()->getBundlePath());
        gImageEffectHost->initDescriptor(this);
      }

      Descriptor::Descriptor(const Descriptor &other, Plugin *plug) 
        : Base(other._properties)
        , _plugin(plug)
      {
        _properties.setStringProperty(kOfxPluginPropFilePath, plug->getBinary()->getBundlePath());
        gImageEffectHost->initDescriptor(this);
      }

      Descriptor::Descriptor(const std::string &bundlePath, Plugin *plug) 
        : Base(effectDescriptorStuff) 
        , _plugin(plug)
      {
        _properties.setStringProperty(kOfxPluginPropFilePath, bundlePath);
        gImageEffectHost->initDescriptor(this);
      }

      Descriptor::~Descriptor()	
      {	
        for(std::map<std::string, ClipDescriptor*>::iterator it = _clips.begin(); it != _clips.end(); ++it)	
          delete it->second;
        _clips.clear();
      }		


      /// create a new clip and add this to the clip map
      ClipDescriptor *Descriptor::defineClip(const std::string &name) {
        ClipDescriptor *c = new ClipDescriptor(name);
        _clips[name] = c;
        _clipsByOrder.push_back(c);
        return c;
      }

      /// implemented for Param::SetDescriptor
      Property::Set &Descriptor::getParamSetProps()
      {
        return _properties;
      }

      /// get the interact description, this will also call describe on the interact
      Interact::Descriptor &Descriptor::getOverlayDescriptor(int bitDepthPerComponent, bool hasAlpha)
      {
        if(_overlayDescriptor.getState() == Interact::eUninitialised) {
          // OK, we need to describe it, set the entry point and describe away
          _overlayDescriptor.setEntryPoint(getOverlayInteractMainEntry());
          _overlayDescriptor.describe(bitDepthPerComponent, hasAlpha);
        }

        return _overlayDescriptor;
      }

      /// get the clips
      const std::map<std::string, ClipDescriptor*> &Descriptor::getClips() const {
        return _clips;
      }

      void Descriptor::addClip(const std::string &name, ClipDescriptor *clip) {
        _clips[name] = clip;
        _clipsByOrder.push_back(clip);
      }

      //
      // Instance
      //

      static const Property::PropSpec effectInstanceStuff[] = {
        /* name                                 type                   dim.   r/o    default value */
        { kOfxPropType,                         Property::eString,     1, true,  kOfxTypeImageEffectInstance },
        { kOfxImageEffectPropContext,           Property::eString,     1, true, "" },
        { kOfxPropInstanceData,                 Property::ePointer,    1, false, NULL },
        { kOfxImageEffectPropPluginHandle,      Property::ePointer,    1, false, NULL },
        { kOfxImageEffectPropProjectSize,       Property::eDouble,     2, true,  "0" },
        { kOfxImageEffectPropProjectOffset,     Property::eDouble,     2, true,  "0" },
        { kOfxImageEffectPropProjectExtent,     Property::eDouble,     2, true,  "0" },
        { kOfxImageEffectPropProjectPixelAspectRatio, Property::eDouble, 1, true,  "0" },
        { kOfxImageEffectInstancePropEffectDuration, Property::eDouble, 1, true,  "0" },
        { kOfxImageEffectInstancePropSequentialRender, Property::eInt, 1, false, "0" },
        { kOfxImageEffectPropFrameRate ,        Property::eDouble,     1, true,  "0" },
        { kOfxPropIsInteractive,                Property::eInt,        1, true, "0" },
#     ifdef kOfxImageEffectPropInAnalysis
        { kOfxImageEffectPropInAnalysis,        Property::eInt,        1, false, "0" }, // removed in OFX 1.4
#     endif
        { kOfxImageEffectPropSupportsTiles,     Property::eInt,        1, false, "1" }, // OFX 1.4
#ifdef OFX_SUPPORTS_OPENGLRENDER
        { kOfxImageEffectPropOpenGLRenderSupported, Property::eString, 1, false, "false"}, // OFX 1.4
	{ kOfxImageEffectPropCudaRenderSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropCudaStreamSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropMetalRenderSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropOpenCLRenderSupported, Property::eString, 1, false, "false" },
#endif
        Property::propSpecEnd
      };

      Instance::Instance(ImageEffectPlugin* plugin,
                         Descriptor         &other,
                         const std::string  &context,
                         bool               interactive)
        : Base(effectInstanceStuff)
        , _plugin(plugin)
        , _context(context)
        , _descriptor(&other)
        , _interactive(interactive)
        , _created(false)
        , _clipPrefsDirty(true)
        , _continuousSamples(false)
        , _frameVarying(false)
        , _outputFrameRate(24)
      {
        int i = 0;
        _properties.setChainedSet(&other.getProps());

        _properties.setPointerProperty(kOfxImageEffectPropPluginHandle, _plugin->getPluginHandle()->getOfxPlugin());

        _properties.setStringProperty(kOfxImageEffectPropContext,context);
        _properties.setIntProperty(kOfxPropIsInteractive,interactive);

        // copy is sequential over
        bool sequential = other.getProps().getIntProperty(kOfxImageEffectInstancePropSequentialRender) != 0;
        _properties.setIntProperty(kOfxImageEffectInstancePropSequentialRender,sequential);

        while(effectInstanceStuff[i].name) {
          
          // don't set hooks for context or isinteractive

          if(strcmp(effectInstanceStuff[i].name,kOfxImageEffectPropContext) &&
             strcmp(effectInstanceStuff[i].name,kOfxPropIsInteractive) &&
             strcmp(effectInstanceStuff[i].name,kOfxImageEffectInstancePropSequentialRender) )
            {
              const Property::PropSpec& spec = effectInstanceStuff[i];

              switch (spec.type) {
              case Property::eDouble:
                _properties.setGetHook(spec.name, this);
                break;
              default:
                break;
              }
            }

          i++;
        }
      }

      /// implemented for Param::SetDescriptor
      Property::Set &Instance::getParamSetProps()
      {
        return _properties;
      }

      /// called after construction to populate clips and params
      OfxStatus Instance::populate() 
      {        
        const std::vector<ClipDescriptor*>& clips = _descriptor->getClipsByOrder();

        int counter = 0;
        for(std::vector<ClipDescriptor*>::const_iterator it=clips.begin();
            it!=clips.end();
            ++it, ++counter) {
            const std::string &name =  (*it)->getName();
            // foreach clip descriptor make a clip instance
            ClipInstance* instance = newClipInstance(this, *it, counter);   
            if(!instance) return kOfxStatFailed;

            _clips[name] = instance;
          }        

        const std::list<Param::Descriptor*>& map = _descriptor->getParamList();

        std::map<std::string,std::vector<Param::Instance*> > parameters;
        std::map<std::string, Param::Instance*> groups;

        for(std::list<Param::Descriptor*>::const_iterator it=map.begin();
            it!=map.end();
            ++it) {
            Param::Descriptor* descriptor = (*it);
            // get the param descriptor
            if(!descriptor) return kOfxStatErrValue;

            // name of the parameter
            std::string name = descriptor->getName();

            // get a param instance from a param descriptor
            Param::Instance* instance = newParam(name,*descriptor);
            if(!instance) return kOfxStatFailed;
          
            // add the value into the param set instance
            OfxStatus st = addParam(name,instance);
            if(st != kOfxStatOK) return st;

            std::string parent = instance->getParentName();
          
            if(parent!="")
              parameters[parent].push_back(instance);

            if(instance->getType()==kOfxParamTypeGroup){
              groups[instance->getName()]=instance;
            }
          }

        // for each group parameter made
        for(std::map<std::string, Param::Instance*>::iterator it=groups.begin();
            it!=groups.end();
            ++it) {
            // cast to a group instance
            Param::GroupInstance* group = dynamic_cast<Param::GroupInstance*>(it->second);

            // if cast ok
            if(group){
              // find the parameters whose parent was this group
              std::map<std::string,std::vector<Param::Instance*> >::iterator it2 = parameters.find(group->getName());
              if(it2!=parameters.end()){
                // associate the group with its children, and the children with its parent group
                group->setChildren(it2->second);
              }
            }
          }

        return kOfxStatOK;
      }

      // do nothing
      int Instance::getDimension(const std::string &name) const {
        printf("failing in %s with name=%s\n", __PRETTY_FUNCTION__, name.c_str());
        throw Property::Exception(kOfxStatErrMissingHostFeature);
      }

      int Instance::upperGetDimension(const std::string &name) {
        return _properties.getDimension(name);
      }

      void Instance::notify(const std::string &/*name*/, bool /*singleValue*/, int /*indexOrN*/)
      { 
        printf("failing in %s\n", __PRETTY_FUNCTION__);
      }

      // don't know what to do
      void Instance::reset(const std::string &/*name*/) {
        printf("failing in %s\n", __PRETTY_FUNCTION__);
        throw Property::Exception(kOfxStatErrMissingHostFeature);
      }

      // get the virutals for viewport size, pixel scale, background colour
      double Instance::getDoubleProperty(const std::string &name, int index) const
      {
        if(name==kOfxImageEffectPropProjectSize){
          if(index>=2) throw Property::Exception(kOfxStatErrBadIndex);
          double values[2];
          getProjectSize(values[0],values[1]);
          return values[index];
        }
        else if(name==kOfxImageEffectPropProjectOffset){
          if(index>=2) throw Property::Exception(kOfxStatErrBadIndex);
          double values[2];
          getProjectOffset(values[0],values[1]);
          return values[index];
        }
        else if(name==kOfxImageEffectPropProjectExtent){
          if(index>=2) throw Property::Exception(kOfxStatErrBadIndex);
          double values[2];
          getProjectExtent(values[0],values[1]);
          return values[index];
        }
        else if(name==kOfxImageEffectPropProjectPixelAspectRatio){
          if(index>=1) throw Property::Exception(kOfxStatErrBadIndex);
          return getProjectPixelAspectRatio();
        }
        else if(name==kOfxImageEffectInstancePropEffectDuration){
          if(index>=1) throw Property::Exception(kOfxStatErrBadIndex);
          return getEffectDuration();
        }
        else if(name==kOfxImageEffectPropFrameRate){
          if(index>=1) throw Property::Exception(kOfxStatErrBadIndex);
          return getFrameRate();
        }
        else
          throw Property::Exception(kOfxStatErrUnknown);        
      }

      void Instance::getDoublePropertyN(const std::string &name, double* first, int n) const
      {
        if(name==kOfxImageEffectPropProjectSize){
          if(n>2) throw Property::Exception(kOfxStatErrBadIndex);
          getProjectSize(first[0],first[1]);
        }
        else if(name==kOfxImageEffectPropProjectOffset){
          if(n>2) throw Property::Exception(kOfxStatErrBadIndex);
          getProjectOffset(first[0],first[1]);
        }
        else if(name==kOfxImageEffectPropProjectExtent){
          if(n>2) throw Property::Exception(kOfxStatErrBadIndex);
          getProjectExtent(first[0],first[1]);
        }
        else if(name==kOfxImageEffectPropProjectPixelAspectRatio){
          if(n>1) throw Property::Exception(kOfxStatErrBadIndex);
          *first = getProjectPixelAspectRatio();
        }
        else if(name==kOfxImageEffectInstancePropEffectDuration){
          if(n>1) throw Property::Exception(kOfxStatErrBadIndex);
          *first = getEffectDuration();
        }
        else if(name==kOfxImageEffectPropFrameRate){
          if(n>1) throw Property::Exception(kOfxStatErrBadIndex);
          *first = getFrameRate();
        }
        else
          throw Property::Exception(kOfxStatErrUnknown);
      }

      Instance::~Instance(){
        // destroy the instance, only if succesfully created
        if (_created) {
#       ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionDestroyInstance<<"()"<<std::endl;
#       endif
          OfxStatus st = mainEntry(kOfxActionDestroyInstance,this->getHandle(),0,0);
#       ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionDestroyInstance<<"()->"<<StatStr(st)<<std::endl;
#       endif
          (void)st;
        }
        
        /// clobber my clips
        std::map<std::string, ClipInstance*>::iterator i;
        for(i = _clips.begin(); i != _clips.end(); ++i) {
          if(i->second)
            delete i->second;
          i->second = NULL;
        }
      }

      /// this is used to populate with any extra action in argumnents that may be needed
      void Instance::setCustomInArgs(const std::string &/*action*/, Property::Set &/*inArgs*/)
      {
      }
      
      /// this is used to populate with any extra action out argumnents that may be needed
      void Instance::setCustomOutArgs(const std::string &/*action*/, Property::Set &/*outArgs*/)
      {
      }

      /// this is used to populate with any extra action out argumnents that may be needed
      void Instance::examineOutArgs(const std::string &/*action*/, OfxStatus, const Property::Set &/*outArgs*/)
      {
      }

      /// check for connection
      bool Instance::checkClipConnectionStatus() const
      {
        std::map<std::string, ClipInstance*>::const_iterator i;      
        for(i = _clips.begin(); i != _clips.end(); ++i) {
          if(!i->second->isOptional() && !i->second->getConnected()) {
            return false;
          }
        }
        return true;
      }

      // override this to make processing abort, return 1 to abort processing
      int Instance::abort() { 
        return 0; 
      }

      // override this to use your own memory instance - must inherrit from memory::instance
      Memory::Instance* Instance::newMemoryInstance(size_t /*nBytes*/) {
        return 0; 
      }

      // return an memory::instance calls makeMemoryInstance that can be overriden
      Memory::Instance* Instance::imageMemoryAlloc(size_t nBytes){
        Memory::Instance* instance = newMemoryInstance(nBytes);
        if(instance)
          return instance;
        else{
          Memory::Instance* instance = new Memory::Instance;
          instance->alloc(nBytes);
          return instance;
        }
      }

      // call the effect entry point
      OfxStatus Instance::mainEntry(const char *action, 
                                    const void *handle, 
                                    Property::Set *inArgs,
                                    Property::Set *outArgs)
      {
        if(_plugin){
          PluginHandle* pHandle = _plugin->getPluginHandle();
          if(pHandle){
            OfxPlugin* ofxPlugin = pHandle->getOfxPlugin();
            if(ofxPlugin){
              
              OfxPropertySetHandle inHandle = 0;
              if(inArgs) {
                setCustomInArgs(action, *inArgs);
                inHandle = inArgs->getHandle();
              }
              
              OfxPropertySetHandle outHandle = 0;
              if(outArgs) {
                setCustomOutArgs(action, *outArgs);
                outHandle = outArgs->getHandle();
              }
                
              OfxStatus stat;
              try {
                 stat = ofxPlugin->mainEntry(action, handle, inHandle, outHandle);
              } CatchAllSetStatus(stat, gImageEffectHost, ofxPlugin, action);

              if(outArgs) 
                examineOutArgs(action, stat, *outArgs);

              return stat;
            }
            return kOfxStatFailed;
          }
          return kOfxStatFailed;
        }
        return kOfxStatFailed;
      }

      // get the nth clip, in order of declaration
      ClipInstance* Instance::getNthClip(int index)
      {
        const std::string name = _descriptor->getClipsByOrder()[index]->getName();
        return _clips[name];
      }

      ClipInstance* Instance::getClip(const std::string& name) const {
        std::map<std::string,ClipInstance*>::const_iterator it = _clips.find(name);
        if(it!=_clips.end()){
          return it->second;
        }
        return 0;
      }

      // create a clip instance
      OfxStatus Instance::createInstanceAction() 
      {
        /// we need to init the clips before we call create instance incase
        /// they try and fetch something in create instance, which they are allowed
        setDefaultClipPreferences();

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionCreateInstance<<"()"<<std::endl;
#       endif
        // now tell the plug-in to create instance
        OfxStatus st = mainEntry(kOfxActionCreateInstance,this->getHandle(),0,0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionCreateInstance<<"()->"<<StatStr(st)<<std::endl;
#       endif

        if (st == kOfxStatOK) {
          _created = true;
        }

        return st;
      }

      // begin/change/end instance changed
      OfxStatus Instance::beginInstanceChangedAction(const std::string & why)
      {
        Property::PropSpec stuff[] = {
          { kOfxPropChangeReason, Property::eString, 1, true, why.c_str() },
          Property::propSpecEnd
        };

        Property::Set inArgs(stuff);

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionBeginInstanceChanged<<"("<<why<<")"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxActionBeginInstanceChanged,this->getHandle(), &inArgs, 0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionBeginInstanceChanged<<"("<<why<<")->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      OfxStatus Instance::paramInstanceChangedAction(const std::string & paramName,
                                                     const std::string & why,
                                                     OfxTime     time,
                                                     OfxPointD   renderScale)
      {        
        Param::Instance* param = getParam(paramName);

        if(isClipPreferencesSlaveParam(paramName))
          _clipPrefsDirty = true;

        if (!param) {
          return kOfxStatFailed;
        }

        Property::PropSpec stuff[] = {
          { kOfxPropType, Property::eString, 1, true, kOfxTypeParameter },
          { kOfxPropName, Property::eString, 1, true, paramName.c_str() },
          { kOfxPropChangeReason, Property::eString, 1, true, why.c_str() },
          { kOfxPropTime, Property::eDouble, 1, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          Property::propSpecEnd
        };

        Property::Set inArgs(stuff);

        // add the second dimension of the render scale
        inArgs.setDoubleProperty(kOfxPropTime,time);

        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionInstanceChanged<<"("<<kOfxTypeParameter<<","<<paramName<<","<<why<<","<<time<<",("<<renderScale.x<<","<<renderScale.y<<"))"<<std::endl;
#       endif

        OfxStatus st = mainEntry(kOfxActionInstanceChanged,this->getHandle(), &inArgs, 0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionInstanceChanged<<"("<<kOfxTypeParameter<<","<<paramName<<","<<why<<","<<time<<",("<<renderScale.x<<","<<renderScale.y<<"))->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      OfxStatus Instance::clipInstanceChangedAction(const std::string & clipName,
                                                    const std::string & why,
                                                    OfxTime     time,
                                                    OfxPointD   renderScale)
      {
        _clipPrefsDirty = true;
        std::map<std::string,ClipInstance*>::iterator it=_clips.find(clipName);
        if(it!=_clips.end())
          return (it->second)->instanceChangedAction(why,time,renderScale);
        else
          return kOfxStatFailed;
      }

      OfxStatus Instance::endInstanceChangedAction(const std::string & why)
      {
        Property::PropSpec whyStuff[] = {
          { kOfxPropChangeReason, Property::eString, 1, true, why.c_str() },
          Property::propSpecEnd
        };

        Property::Set inArgs(whyStuff);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionEndInstanceChanged<<"("<<why<<")"<<std::endl;
#       endif

        OfxStatus st = mainEntry(kOfxActionEndInstanceChanged,this->getHandle(), &inArgs, 0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionEndInstanceChanged<<"("<<why<<")->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      // purge your caches
      OfxStatus Instance::purgeCachesAction(){
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionPurgeCaches<<"()"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxActionPurgeCaches ,this->getHandle(),0,0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionPurgeCaches<<"()->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      // sync your private data
      OfxStatus Instance::syncPrivateDataAction(){
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionSyncPrivateData<<"()"<<std::endl;
#       endif
        // Only call kOfxActionSyncPrivateData if kOfxPropParamSetNeedsSyncing is not set,
        // or if it is set to 1.
        // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxPropParamSetNeedsSyncing
        Property::Int* s = _properties.fetchIntProperty(kOfxPropParamSetNeedsSyncing);
        bool needsSyncing = s ? s->getValue() : true;
        OfxStatus st = kOfxStatReplyDefault;
        if (needsSyncing) {
          st = mainEntry(kOfxActionSyncPrivateData,this->getHandle(),0,0);
          if (s) {
            s->setValue(0);
          }
        }
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionSyncPrivateData<<"()->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      // begin/end edit instance
      OfxStatus Instance::beginInstanceEditAction(){
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionBeginInstanceEdit<<"()"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxActionBeginInstanceEdit,this->getHandle(),0,0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionBeginInstanceEdit<<"()->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      OfxStatus Instance::endInstanceEditAction(){
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionEndInstanceEdit<<"()"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxActionEndInstanceEdit,this->getHandle(),0,0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionEndInstanceEdit<<"()->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

#   ifdef OFX_SUPPORTS_OPENGLRENDER
      // attach/detach OpenGL context
      OfxStatus Instance::contextAttachedAction(){
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionOpenGLContextAttached<<"()"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxActionOpenGLContextAttached,this->getHandle(),0,0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionOpenGLContextAttached<<"()->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      OfxStatus Instance::contextDetachedAction(){
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionOpenGLContextDetached<<"()"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxActionOpenGLContextDetached,this->getHandle(),0,0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxActionOpenGLContextDetached<<"()->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }
#   endif

      OfxStatus Instance::beginRenderAction(OfxTime  startFrame,
                                            OfxTime  endFrame,
                                            OfxTime  step,
                                            bool     interactive,
                                            OfxPointD   renderScale,
                                            bool     sequentialRender,
                                            bool     interactiveRender
                                            )
      {
        Property::PropSpec stuff[] = {
          { kOfxImageEffectPropFrameRange, Property::eDouble, 2, true, "0" },
          { kOfxImageEffectPropFrameStep, Property::eDouble, 1, true, "0" }, 
          { kOfxPropIsInteractive, Property::eInt, 1, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          { kOfxImageEffectPropSequentialRenderStatus, Property::eInt, 1, true, "0" },
          { kOfxImageEffectPropInteractiveRenderStatus, Property::eInt, 1, true, "0" },
          Property::propSpecEnd
        };

        Property::Set inArgs(stuff);

        // set up second dimension for frame range and render scale
        inArgs.setDoubleProperty(kOfxImageEffectPropFrameRange,startFrame, 0);
        inArgs.setDoubleProperty(kOfxImageEffectPropFrameRange,endFrame, 1);

        inArgs.setDoubleProperty(kOfxImageEffectPropFrameStep,step);

        inArgs.setIntProperty(kOfxPropIsInteractive,interactive);

        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);

        inArgs.setIntProperty(kOfxImageEffectPropSequentialRenderStatus,sequentialRender);
        inArgs.setIntProperty(kOfxImageEffectPropInteractiveRenderStatus,interactiveRender);

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionBeginSequenceRender<<"(("<<startFrame<<","<<endFrame<<"),"<<step<<","<<interactive<<",("<<renderScale.x<<","<<renderScale.y<<"),"<<sequentialRender<<","<<interactiveRender
          <<")"<<std::endl;
#       endif

        OfxStatus st = mainEntry(kOfxImageEffectActionBeginSequenceRender, this->getHandle(), &inArgs, 0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionBeginSequenceRender<<"(("<<startFrame<<","<<endFrame<<"),"<<step<<","<<interactive<<",("<<renderScale.x<<","<<renderScale.y<<"),"<<sequentialRender<<","<<interactiveRender
          <<")->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      OfxStatus Instance::renderAction(OfxTime      time,
                                       const std::string &  field,
                                       const OfxRectI    &renderRoI,
                                       OfxPointD   renderScale,
                                       bool     sequentialRender,
                                       bool     interactiveRender,
                                       bool     draftRender
                                       )
      {
        static const Property::PropSpec inStuff[] = {
          { kOfxPropTime, Property::eDouble, 1, true, "0" },
          { kOfxImageEffectPropFieldToRender, Property::eString, 1, true, "" }, 
          { kOfxImageEffectPropRenderWindow, Property::eInt, 4, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          { kOfxImageEffectPropSequentialRenderStatus, Property::eInt, 1, true, "0" },
          { kOfxImageEffectPropInteractiveRenderStatus, Property::eInt, 1, true, "0" },
          { kOfxImageEffectPropRenderQualityDraft, Property::eInt, 1, true, "0" },
          Property::propSpecEnd
        };

        Property::Set inArgs(inStuff);
        
        inArgs.setStringProperty(kOfxImageEffectPropFieldToRender,field);
        inArgs.setDoubleProperty(kOfxPropTime,time);
        inArgs.setIntPropertyN(kOfxImageEffectPropRenderWindow, &renderRoI.x1, 4);
        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);
        inArgs.setIntProperty(kOfxImageEffectPropSequentialRenderStatus,sequentialRender);
        inArgs.setIntProperty(kOfxImageEffectPropInteractiveRenderStatus,interactiveRender);
        inArgs.setIntProperty(kOfxImageEffectPropRenderQualityDraft,draftRender);

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionRender<<"("<<time<<","<<field<<",("<<renderRoI.x1<<","<<renderRoI.y1<<","<<renderRoI.x2<<","<<renderRoI.y2<<"),("<<renderScale.x<<","<<renderScale.y<<"),"<<sequentialRender<<","<<interactiveRender
          <<")"<<std::endl;
#       endif

        OfxStatus st = mainEntry(kOfxImageEffectActionRender,this->getHandle(), &inArgs, 0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionRender<<"("<<time<<","<<field<<",("<<renderRoI.x1<<","<<renderRoI.y1<<","<<renderRoI.x2<<","<<renderRoI.y2<<"),("<<renderScale.x<<","<<renderScale.y<<"),"<<sequentialRender<<","<<interactiveRender
          <<")->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      OfxStatus Instance::endRenderAction(OfxTime  startFrame,
                                          OfxTime  endFrame,
                                          OfxTime  step,
                                          bool     interactive,
                                          OfxPointD   renderScale,
                                          bool     sequentialRender,
                                          bool     interactiveRender
                                          )
      {
        static const Property::PropSpec inStuff[] = {
          { kOfxImageEffectPropFrameRange, Property::eDouble, 2, true, "0" },
          { kOfxImageEffectPropFrameStep, Property::eDouble, 1, true, "0" }, 
          { kOfxPropIsInteractive, Property::eInt, 1, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          { kOfxImageEffectPropSequentialRenderStatus, Property::eInt, 1, true, "0" },
          { kOfxImageEffectPropInteractiveRenderStatus, Property::eInt, 1, true, "0" },
          Property::propSpecEnd
        };

        Property::Set inArgs(inStuff);

        inArgs.setDoubleProperty(kOfxImageEffectPropFrameStep,step);

        inArgs.setDoubleProperty(kOfxImageEffectPropFrameRange,startFrame, 0);
        inArgs.setDoubleProperty(kOfxImageEffectPropFrameRange,endFrame, 1);
        inArgs.setIntProperty(kOfxPropIsInteractive,interactive);
        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);
        inArgs.setIntProperty(kOfxImageEffectPropSequentialRenderStatus,sequentialRender);
        inArgs.setIntProperty(kOfxImageEffectPropInteractiveRenderStatus,interactiveRender);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionEndSequenceRender<<"(("<<startFrame<<","<<endFrame<<"),"<<step<<","<<interactive<<",("<<renderScale.x<<","<<renderScale.y<<"),"<<sequentialRender<<","<<interactiveRender
          <<")"<<std::endl;
#       endif

        OfxStatus st = mainEntry(kOfxImageEffectActionEndSequenceRender,this->getHandle(), &inArgs, 0);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionEndSequenceRender<<"(("<<startFrame<<","<<endFrame<<"),"<<step<<","<<interactive<<",("<<renderScale.x<<","<<renderScale.y<<"),"<<sequentialRender<<","<<interactiveRender
          <<")->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      /// calculate the default rod for this effect instance
      OfxRectD Instance::calcDefaultRegionOfDefinition(OfxTime  time,
                                                       OfxPointD   /*renderScale*/) const
      {
        OfxRectD rod;

        // figure out the default contexts
        if(
           _context == kOfxImageEffectContextGenerator) {
          // generator is the extent
          rod.x1 = rod.y1 = 0;
          getProjectExtent(rod.x2, rod.y2);
        }                                     
        else if(_context == kOfxImageEffectContextFilter ||
                _context == kOfxImageEffectContextPaint) {
          // filter and paint default to the input clip
          ClipInstance *clip = getClip(kOfxImageEffectSimpleSourceClipName);
          if(clip) {
            rod = clip->getRegionOfDefinition(time);
          } else {
            throw Property::Exception(kOfxStatFailed);
          }
        }
        else if(_context == kOfxImageEffectContextTransition) {
          // transition is the union of the two clips
          ClipInstance *clipFrom = getClip(kOfxImageEffectTransitionSourceFromClipName);
          ClipInstance *clipTo = getClip(kOfxImageEffectTransitionSourceToClipName);
          if(clipFrom && clipTo) {
            rod = clipFrom->getRegionOfDefinition(time);
            rod = Union(rod, clipTo->getRegionOfDefinition(time));
          } else {
            throw Property::Exception(kOfxStatFailed);
          }
        }
        else if(_context == kOfxImageEffectContextGeneral
                ) {
          // general context is the union of all the non optional clips
          bool gotOne = false;
          for(std::map<std::string, ClipInstance*>::const_iterator it=_clips.begin();
              it!=_clips.end();
              ++it) {
            ClipInstance *clip = it->second;
            if(!clip->isOutput() && (!clip->isOptional() || (clip->getConnected() && clip->getName() == kOfxImageEffectSimpleSourceClipName))) {
              if(!gotOne)
                rod = clip->getRegionOfDefinition(time);
              else
                rod = Union(rod, clip->getRegionOfDefinition(time));
              gotOne = true;
            }
          }
            
          if(!gotOne) {
            /// no non optionals? then be the extent
            rod.x1 = rod.y1 = 0;
            getProjectExtent(rod.x2, rod.y2);
          }
            
        }
        else if(_context == kOfxImageEffectContextRetimer) {
          // retimer
          ClipInstance *clip = getClip(kOfxImageEffectSimpleSourceClipName);
          if(clip) {
            Param::DoubleInstance *param = dynamic_cast<Param::DoubleInstance *>(getParam(kOfxImageEffectRetimerParamName));
            if(param) {
              rod = clip->getRegionOfDefinition(floor(time));
              rod = Union(rod, clip->getRegionOfDefinition(floor(time) + 1));
            } else {
                throw Property::Exception(kOfxStatFailed);
            }
          } else {
            throw Property::Exception(kOfxStatFailed);
          }
        }
        else {
          // unknown context
          throw Property::Exception(kOfxStatErrMissingHostFeature);
        }

        return rod;
      }

      ////////////////////////////////////////////////////////////////////////////////
      // RoD call
      OfxStatus Instance::getRegionOfDefinitionAction(OfxTime  time,
                                                      OfxPointD   renderScale,
                                                      OfxRectD &rod)
      {
        static const Property::PropSpec inStuff[] = {
          { kOfxPropTime, Property::eDouble, 1, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          Property::propSpecEnd
        };

        static const Property::PropSpec outStuff[] = {
          { kOfxImageEffectPropRegionOfDefinition , Property::eDouble, 4, false, "0" },
          Property::propSpecEnd
        };

        Property::Set inArgs(inStuff);
        Property::Set outArgs(outStuff);
        
        inArgs.setDoubleProperty(kOfxPropTime,time);
        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetRegionOfDefinition<<"("<<time<<",("<<renderScale.x<<","<<renderScale.y<<"))"<<std::endl;
#       endif
        OfxStatus stat = mainEntry(kOfxImageEffectActionGetRegionOfDefinition,
                                   this->getHandle(),
                                   &inArgs,
                                   &outArgs);
        if(stat == kOfxStatOK) {
          outArgs.getDoublePropertyN(kOfxImageEffectPropRegionOfDefinition, &rod.x1, 4);
        }
        else if(stat == kOfxStatReplyDefault) {
          rod = calcDefaultRegionOfDefinition(time, renderScale);
        }        


#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetRegionOfDefinition<<"("<<time<<",("<<renderScale.x<<","<<renderScale.y<<"))->"<<StatStr(stat);
          if(stat == kOfxStatOK) {
              std::cout << ": ("<<rod.x1<<","<<rod.y1<<","<<rod.x2<<","<<rod.y2<<")";
          }
          std::cout << std::endl;
#       endif
          
        return stat;
      }

      /// get the region of interest for each input and return it in the given std::map
      OfxStatus Instance::getRegionOfInterestAction(OfxTime  time,
                                                    OfxPointD   renderScale,
                                                    const OfxRectD &roi,
                                                    std::map<ClipInstance *, OfxRectD>& rois) 
      {
        OfxStatus stat = kOfxStatReplyDefault;

        // reset the map
        rois.clear();

        if(!supportsTiles()) {
          /// No tiling support on the effect at all. So set the roi of each input clip to be the RoD of that clip.
          for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
              it!=_clips.end();
              ++it) {
            if(!it->second->isOutput() ||
               getContext() == kOfxImageEffectContextGenerator) {
              if (it->second->isOutput() || it->second->getConnected()) {// needed to be able to fetch the RoD
					/// @todo tuttle: how to support size on generators... check if this is correct in all cases.
                OfxRectD roi = it->second->getRegionOfDefinition(time);
                rois[it->second] = roi;
              }
            }
          }
          stat = kOfxStatOK;
        }
        else {
          /// set up the in args 
          static const Property::PropSpec inStuff[] = {
            { kOfxPropTime, Property::eDouble, 1, true, "0" },
            { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
            { kOfxImageEffectPropRegionOfInterest , Property::eDouble, 4, true, 0 },
            Property::propSpecEnd
          };
          Property::Set inArgs(inStuff);

          inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);
          inArgs.setDoubleProperty(kOfxPropTime,time);
          inArgs.setDoublePropertyN(kOfxImageEffectPropRegionOfInterest, &roi.x1, 4);

          Property::Set outArgs;
          for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
              it!=_clips.end();
              ++it) {
            if(!it->second->isOutput() ||
               getContext() == kOfxImageEffectContextGenerator) {
              Property::PropSpec s;
              std::string name = "OfxImageClipPropRoI_"+it->first;
            
              s.name = name.c_str();
              s.type = Property::eDouble;
              s.dimension = 4;
              s.readonly = false;
              s.defaultValue = "";
              outArgs.createProperty(s);
            
              /// initialise to the default
              outArgs.setDoublePropertyN(s.name, &roi.x1, 4);
            }
          }

#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetRegionsOfInterest<<"("<<time<<",("<<renderScale.x<<","<<renderScale.y<<"),("<<roi.x1<<","<<roi.y1<<","<<roi.x2<<","<<roi.y2<<"))"<<std::endl;
#         endif
          /// call the action
          stat = mainEntry(kOfxImageEffectActionGetRegionsOfInterest,
                           this->getHandle(),
                           &inArgs,
                           &outArgs);

#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetRegionsOfInterest<<"("<<time<<",("<<renderScale.x<<","<<renderScale.y<<"),("<<roi.x1<<","<<roi.y1<<","<<roi.x2<<","<<roi.y2<<"))->"<<StatStr(stat);
            if (stat == kOfxStatOK) {
                std::cout << ": ";
                for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
                    it!=_clips.end();
                    ++it) {
                    std::string name = "OfxImageClipPropRoI_"+it->first;
                    OfxRectD thisRoi;
                    thisRoi.x1 = outArgs.getDoubleProperty(name,0);
                    thisRoi.y1 = outArgs.getDoubleProperty(name,1);
                    thisRoi.x2 = outArgs.getDoubleProperty(name,2);
                    thisRoi.y2 = outArgs.getDoubleProperty(name,3);
                    std::cout << it->first << "->("<<thisRoi.x1<<","<<thisRoi.y1<<","<<thisRoi.x2<<","<<thisRoi.y2<<") ";
                }
            }
            std::cout << std::endl;
#           endif
          /// set the thing up
          for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
              it!=_clips.end();
              ++it) {
              if(!it->second->isOutput() ||
               getContext() == kOfxImageEffectContextGenerator) {
                if (it->second->isOutput() || it->second->getConnected()) { // needed to be able to fetch the RoD
                  
                  if(it->second->supportsTiles()) {
                    std::string name = "OfxImageClipPropRoI_"+it->first;
                    OfxRectD thisRoi;
                    thisRoi.x1 = outArgs.getDoubleProperty(name,0);
                    thisRoi.y1 = outArgs.getDoubleProperty(name,1);
                    thisRoi.x2 = outArgs.getDoubleProperty(name,2);
                    thisRoi.y2 = outArgs.getDoubleProperty(name,3);
                  
                    // and DON'T clamp it to the clip's rod
                    // We cannot clip it against the RoD because the RoI may be used for frames
                    // at different a time or view than the current time and view passed to this action
                    // which would result in a wrong clipping. Unfortunately only the implementation of
                    // the host can do the correct clipping.
                    //thisRoi = Clamp(thisRoi, rod);
                    rois[it->second] = thisRoi;
                  }
                  else {
                    /// not supporting tiles on this input, so set it to the rod
                    OfxRectD rod = it->second->getRegionOfDefinition(time
                                                                       );
                    rois[it->second] = rod;
                  }
                }
              }
            }
        }
  
        return stat;
      }
        

      ////////////////////////////////////////////////////////////////////////////////
      /// see how many frames are needed from each clip to render the indicated frame
      OfxStatus Instance::getFrameNeededAction(OfxTime time, 
                                               RangeMap &rangeMap)
      {
        OfxStatus stat = kOfxStatReplyDefault;
        Property::Set outArgs;
      
        if(temporalAccess()) {
          static const Property::PropSpec inStuff[] = {
            { kOfxPropTime, Property::eDouble, 1, true, "0" },          
            Property::propSpecEnd
          };
          Property::Set inArgs(inStuff);       
          inArgs.setDoubleProperty(kOfxPropTime,time);
        
        
          for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
              it!=_clips.end();
              ++it) {
            if(!it->second->isOutput()) {
              Property::PropSpec s;
              std::string name = "OfxImageClipPropFrameRange_"+it->first;
              
              s.name = name.c_str();
              s.type = Property::eDouble;
              s.dimension = 0;
              s.readonly = false;
              s.defaultValue = "";
              outArgs.createProperty(s);
              /// intialise it to the current frame
              outArgs.setDoubleProperty(name, time, 0);
              outArgs.setDoubleProperty(name, time, 1);
            }
          }

#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetFramesNeeded<<"("<<time<<")"<<std::endl;
#         endif
          stat = mainEntry(kOfxImageEffectActionGetFramesNeeded,
                           this->getHandle(),
                           &inArgs,
                           &outArgs);
#         ifdef OFX_DEBUG_ACTIONS
            std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetFramesNeeded<<"("<<time<<")->"<<StatStr(stat);
            if (stat == kOfxStatOK) {
                std::cout << ": ";
                for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
                    it!=_clips.end();
                    ++it) {
                    ClipInstance *clip = it->second;

                    if(!clip->isOutput()) {
                        std::string name = "OfxImageClipPropFrameRange_"+it->first;
                        std::cout << it->first << "->[";

                        int nRanges = outArgs.getDimension(name);
                        for(int r=0;r<nRanges;){
                            double min = outArgs.getDoubleProperty(name,r);
                            double max = outArgs.getDoubleProperty(name,r+1);
                            r += 2;
                            std::cout <<"("<<min<<","<<max<<")";
                            if (r < nRanges-1) {
                                std::cout << ",";
                            }
                        }
                        std::cout << "]";
                    }
                }
            }
            std::cout << std::endl;
#         endif
        }
        
        OfxRangeD defaultRange;
        defaultRange.min = 
          defaultRange.max = time;

        for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
            it!=_clips.end();
            ++it) {
          ClipInstance *clip = it->second;
          
          if(!clip->isOutput()) {
            if(stat != kOfxStatOK) {
              rangeMap[clip].push_back(defaultRange);
            }
            else {
              std::string name = "OfxImageClipPropFrameRange_"+it->first;
          
              int nRanges = outArgs.getDimension(name);
              if(nRanges%2 != 0)
                return kOfxStatFailed; // bad! needs to be divisible by 2

              if(nRanges == 0) {
                rangeMap[clip].push_back(defaultRange);
              }
              else {
                for(int r=0;r<nRanges;){
                  double min = outArgs.getDoubleProperty(name,r);
                  double max = outArgs.getDoubleProperty(name,r+1);
                  r += 2;
                
                  OfxRangeD range;
                  range.min = min;
                  range.max = max;
                  rangeMap[clip].push_back(range);
                }
              }
            }
          }
        }

        return stat;
      }

      OfxStatus Instance::isIdentityAction(OfxTime     &time,
                                           const std::string &  field,
                                           const OfxRectI &renderRoI,
                                           OfxPointD   renderScale,
                                           std::string &clip)
      {
        static const Property::PropSpec inStuff[] = {
          { kOfxPropTime, Property::eDouble, 1, true, "0" },
          { kOfxImageEffectPropFieldToRender, Property::eString, 1, true, "" }, 
          { kOfxImageEffectPropRenderWindow, Property::eInt, 4, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          Property::propSpecEnd
        };

        static const Property::PropSpec outStuff[] = {
          { kOfxPropTime, Property::eDouble, 1, false, "0.0" },
          { kOfxPropName, Property::eString, 1, false, "" },
          Property::propSpecEnd
        };

        Property::Set inArgs(inStuff);        

        inArgs.setStringProperty(kOfxImageEffectPropFieldToRender,field);
        inArgs.setDoubleProperty(kOfxPropTime,time);
        inArgs.setIntPropertyN(kOfxImageEffectPropRenderWindow, &renderRoI.x1, 4);
        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);

        Property::Set outArgs(outStuff);

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionIsIdentity<<"("<<time<<","<<field<<",("<<renderRoI.x1<<","<<renderRoI.y1<<","<<renderRoI.x2<<","<<renderRoI.y2<<"),("<<renderScale.x<<","<<renderScale.y<<"))"<<std::endl;
#       endif
        outArgs.setDoubleProperty(kOfxPropTime,time);

        OfxStatus st = mainEntry(kOfxImageEffectActionIsIdentity,
                                 this->getHandle(),
                                 &inArgs,
                                 &outArgs);        

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionIsIdentity<<"("<<time<<","<<field<<",("<<renderRoI.x1<<","<<renderRoI.y1<<","<<renderRoI.x2<<","<<renderRoI.y2<<"),("<<renderScale.x<<","<<renderScale.y<<"))->"<<StatStr(st);
          if(st==kOfxStatOK){
              std::cout << ": "<<outArgs.getDoubleProperty(kOfxPropTime)<<","<<outArgs.getStringProperty(kOfxPropName);
          }
          std::cout<<std::endl;
#       endif

        if(st==kOfxStatOK){
          time = outArgs.getDoubleProperty(kOfxPropTime);
          clip = outArgs.getStringProperty(kOfxPropName);        
        }
        
        return st;
      }

      /// Get whether the component is a supported 'chromatic' component (RGBA or alpha) in
      /// the base API.
      /// Override this if you have extended your chromatic colour types (eg RGB) and want
      /// the clip preferences logic to still work
      bool Instance::isChromaticComponent(const std::string &str) const
      {
        if(str == kOfxImageComponentRGBA)
          return true;
        if(str == kOfxImageComponentRGB)
          return true;
        if(str == kOfxImageComponentAlpha)
          return true;
        return false;
      }
      
      /// function to check for multiple bit depth support
      /// The answer will depend on host, plugin and context
      bool Instance::canCurrentlyHandleMultipleClipDepths() const
      {
        /// does the host support 'em
        bool hostSupports = gImageEffectHost->getProperties().getIntProperty(kOfxImageEffectPropSupportsMultipleClipDepths) != 0;;

        /// does the plug-in support 'em
        bool pluginSupports = supportsMultipleClipDepths();

        /// no support, so no
        if(!hostSupports || !pluginSupports)
          return false;

        /// if filter context, no support, tempted to change this though
        if(_context == kOfxImageEffectContextFilter)
          return false;

        /// if filter context, no support
        if(_context == kOfxImageEffectContextGenerator ||
           _context == kOfxImageEffectContextTransition ||
           _context == kOfxImageEffectContextPaint ||
           _context == kOfxImageEffectContextRetimer)
          return false;
        
        /// OK we're cool
        return true;
      }
      
      /// Setup the default clip preferences on the clips
      void Instance::setDefaultClipPreferences()
      {
        /// is there multiple bit depth support? Depends on host, plugin and context
        bool multiBitDepth = canCurrentlyHandleMultipleClipDepths();

        /// OK find the deepest chromatic component on our input clips and the one with the
        /// most components
        bool hasSetCompsAndDepth = false;
        std::string deepestBitDepth = kOfxBitDepthNone;
        std::string mostComponents  = kOfxImageComponentNone;       
        double frameRate = getFrameRate(); //< default to the project frame rate
        std::string premult = kOfxImageOpaque;
        for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
            it!=_clips.end();
            ++it) {
          ClipInstance *clip = it->second;

          if(!clip->isOutput()) {
            bool connected = clip->getConnected();
             
            if (connected) {
              frameRate = Maximum(frameRate, clip->getFrameRate());
            }

            std::string rawComp  = clip->getUnmappedComponents();
            rawComp = clip->findSupportedComp(rawComp); // turn that into a comp the plugin expects on that clip

            const std::string &rawDepth = clip->getUnmappedBitDepth();
            const std::string &rawPreMult = clip->getPremult();            
              
            if(isChromaticComponent(rawComp)) {
              if(connected) {
                if(rawPreMult == kOfxImagePreMultiplied)
                  premult = kOfxImagePreMultiplied;
                else if(rawPreMult == kOfxImageUnPreMultiplied && premult != kOfxImagePreMultiplied)
                  premult = kOfxImageUnPreMultiplied;
              }
                
              if(connected) {
                //Update deepest bitdepth and most components only if the infos are relevant, i.e: only if the clip is connected
                hasSetCompsAndDepth = true;
                deepestBitDepth = FindDeepestBitDepth(deepestBitDepth, rawDepth);
                mostComponents  = findMostChromaticComponents(mostComponents, rawComp);
              }
            }
          }
        }
        // default to a reasonable value if there is no input
        if (!hasSetCompsAndDepth) {
          mostComponents = kOfxImageComponentRGBA;
          deepestBitDepth = kOfxBitDepthFloat;
        }

        /// set some stuff up
        _outputFrameRate           = frameRate;
        _outputFielding            = getDefaultOutputFielding();
        _outputPreMultiplication   = premult;
        _continuousSamples         = false;
        _frameVarying              = false;

        /// now find the best depth that the plugin supports
        deepestBitDepth = bestSupportedDepth(deepestBitDepth);

        /// now add the clip gubbins to the out args
        for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
            it!=_clips.end();
            ++it) {
          ClipInstance *clip = it->second;

          std::string comp, depth;

          std::string rawComp  = clip->getUnmappedComponents();
          rawComp = clip->findSupportedComp(rawComp); // turn that into a comp the plugin expects on that clip
          const std::string &rawDepth = clip->getUnmappedBitDepth();

          if(isChromaticComponent(rawComp)) {
                
            if(clip->isOutput() || clip->isOptional()) {
              // "Optional input clips can always have their component types remapped"
              // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id482755
              depth = deepestBitDepth;
              comp = clip->findSupportedComp(mostComponents);
                  
              clip->setPixelDepth(depth);
              clip->setComponents(comp);
            }
            else {                 
              comp  = rawComp;
              depth = multiBitDepth ? bestSupportedDepth(rawDepth) : deepestBitDepth;
                  
              clip->setPixelDepth(depth);
              clip->setComponents(comp);
            }
          }
          else {
            /// hmm custom component type, don't touch it and pass it through
            clip->setPixelDepth(rawDepth);
            clip->setComponents(rawComp);
          }
        }
      }

      /// Initialise the clip preferences arguments, override this to do
      /// stuff with wierd components etc...
      void Instance::setupClipPreferencesArgs(Property::Set &outArgs)
      {
        /// reset all the clip prefs stuff to their defaults
        setDefaultClipPreferences();

        static const Property::PropSpec clipPrefsStuffs []=
          {
            { kOfxImageEffectPropFrameRate,          Property::eDouble,  1, false,  "1" },
            { kOfxImageEffectPropPreMultiplication,  Property::eString,  1, false,  "" },
            { kOfxImageClipPropFieldOrder,           Property::eString,  1, false,  "" },
            { kOfxImageClipPropContinuousSamples,    Property::eInt,     1, false,  "0" },
            { kOfxImageEffectFrameVarying,           Property::eInt,     1, false,  "0" },
            Property::propSpecEnd
          };
        
        outArgs.addProperties(clipPrefsStuffs);
        
        /// set the default for those

        /// is there multiple bit depth support? Depends on host, plugin and context
        bool multiBitDepth = canCurrentlyHandleMultipleClipDepths();

        outArgs.setStringProperty(kOfxImageClipPropFieldOrder, _outputFielding);
        outArgs.setStringProperty(kOfxImageEffectPropPreMultiplication, _outputPreMultiplication);

        /// now add the clip gubbins to the out args
        double projectPAR = getProjectPixelAspectRatio();
        bool multipleClipsPAR = supportsMultipleClipPARs();
        /// get the PAR of inputs, if it has different PARs and the effect does not support multiple clips PAR, throw an exception
        double inputPar;
        bool inputParSet = false;
        for (std::map<std::string, ClipInstance*>::iterator it2 = _clips.begin(); it2 != _clips.end(); ++it2) {
          if (!it2->second->isOutput() && it2->second->getConnected()) {
              if (!inputParSet) {
                  inputPar = it2->second->getAspectRatio();
                  inputParSet = true;
              } else if (!multipleClipsPAR && inputPar != it2->second->getAspectRatio()) {
                  // We have several inputs with different aspect ratio, which should be forbidden by the host.
                  throw Property::Exception(kOfxStatErrValue);
              }
            }
        }
  
          
        for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
            it!=_clips.end();
            ++it) {
          ClipInstance *clip = it->second;

          std::string componentParamName = "OfxImageClipPropComponents_"+it->first;
          std::string depthParamName     = "OfxImageClipPropDepth_"+it->first;
          std::string parParamName       = "OfxImageClipPropPAR_"+it->first;

          Property::PropSpec specComp = {componentParamName.c_str(),  Property::eString, 0, false,          ""}; // note the support for multi-planar clips
          outArgs.createProperty(specComp);
          outArgs.setStringProperty(componentParamName.c_str(), clip->getComponents().c_str()); // as it is variable dimension, there is no default value, so we have to set it explicitly

          Property::PropSpec specDep = {depthParamName.c_str(),       Property::eString, 1, !multiBitDepth, clip->getPixelDepth().c_str()};
          outArgs.createProperty(specDep);

          Property::PropSpec specPAR = {parParamName.c_str(),         Property::eDouble, 1, false,          "1"};
          outArgs.createProperty(specPAR);
          if (!clip->isOutput()) {
            // If the clip is input, use the same par for all inputs unless the plug-in supports multiple clips PAR
            double par;
            if (!multipleClipsPAR && inputParSet) {
                par = inputPar;
            } else if (multipleClipsPAR) {
                par = clip->getAspectRatio();
            } else {
                par = projectPAR;
            }
            outArgs.setDoubleProperty(parParamName, par);
          } else {
            // If the clip is output we should propagate the pixel aspect ratio of the inputs
            outArgs.setDoubleProperty(parParamName, inputParSet ? inputPar : projectPAR);
          }
        }
          
        //Set the output frame rate according to what input clips have. Several inputs with different frame rates should be
        //forbidden by the host.
        bool outputFrameRateSet = false;
        double outputFrameRate = _outputFrameRate;
        for (std::map<std::string, ClipInstance*>::iterator it2 = _clips.begin(); it2 != _clips.end(); ++it2) {
            if (!it2->second->isOutput() && it2->second->getConnected()) {
                if (!outputFrameRateSet) {
                    outputFrameRate = it2->second->getFrameRate();
                    outputFrameRateSet = true;
                } else if (outputFrameRate != it2->second->getFrameRate()) {
                    // We have several inputs with different frame rates
                    throw Property::Exception(kOfxStatErrValue);
                }
            }
        }
          
        outArgs.setDoubleProperty(kOfxImageEffectPropFrameRate, outputFrameRate);

      }

      /// the idea here is the clip prefs live as active props on the effect
      /// and are set up by clip preferences. The action manages the clip
      /// preferences bits. We also monitor clip and param changes and
      /// flag when clip prefs is dirty.      
      /// call the clip preferences action
      bool Instance::getClipPreferences()
      {      
        /// create the out args with the stuff that does not depend on individual clips
        Property::Set outArgs;

        setupClipPreferencesArgs(outArgs);


#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetClipPreferences<<"()"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxImageEffectActionGetClipPreferences,
                                 this->getHandle(),
                                 0,
                                 &outArgs);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetClipPreferences<<"()->"<<StatStr(st);
#       endif

        if(st!=kOfxStatOK && st!=kOfxStatReplyDefault) {
#       ifdef OFX_DEBUG_ACTIONS
            std::cout << std::endl;
#       endif
          /// ouch
          return false;
        }

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << ": ";
#       endif
        /// OK, go pump the components/depths back into the clips themselves
        for(std::map<std::string, ClipInstance*>::iterator it=_clips.begin();
            it!=_clips.end();
            ++it) {
            ClipInstance *clip = it->second;

            std::string componentParamName = "OfxImageClipPropComponents_"+it->first;
            std::string depthParamName = "OfxImageClipPropDepth_"+it->first;
            std::string parParamName = "OfxImageClipPropPAR_"+it->first;

#       ifdef OFX_DEBUG_ACTIONS
            std::cout << it->first<<"->"<<outArgs.getStringProperty(depthParamName)<<","<<outArgs.getStringProperty(componentParamName)<<","<<outArgs.getDoubleProperty(parParamName)<<" ";
#       endif

            clip->setPixelDepth(outArgs.getStringProperty(depthParamName));
            clip->setComponents(outArgs.getStringProperty(componentParamName));
            //clip->setPixelAspect(outArgs.getDoubleProperty(parParamName));
          }

        _outputFrameRate           = outArgs.getDoubleProperty(kOfxImageEffectPropFrameRate);
        _outputFielding            = outArgs.getStringProperty(kOfxImageClipPropFieldOrder);
        _outputPreMultiplication   = outArgs.getStringProperty(kOfxImageEffectPropPreMultiplication);
        _continuousSamples = outArgs.getIntProperty(kOfxImageClipPropContinuousSamples) != 0;
        _frameVarying      = outArgs.getIntProperty(kOfxImageEffectFrameVarying) != 0;
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << _outputFrameRate<<","<<_outputFielding<<","<<_outputPreMultiplication<<","<<_continuousSamples<<","<<_frameVarying<<std::endl;
#       endif

        _clipPrefsDirty  = false;

        return true;
      }

      /// find the most chromatic components out of the two. Override this if you define
      /// more chromatic components
      const std::string &Instance::findMostChromaticComponents(const std::string &a, const std::string &b) const
      {
        if(a == kOfxImageComponentNone)
          return b;
        if(a == kOfxImageComponentRGBA)
          return a;
        if(b == kOfxImageComponentRGBA)
          return b;
        if(a == kOfxImageComponentRGB)
          return a;
        if(b == kOfxImageComponentRGB)
          return b;
        return a;
      }

      /// given the bit depth, find the best match for it.
      const std::string &Instance::bestSupportedDepth(const std::string &depth) const
      {
        static const std::string none(kOfxBitDepthNone);
        static const std::string bytes(kOfxBitDepthByte);
        static const std::string shorts(kOfxBitDepthShort);
        static const std::string halfs(kOfxBitDepthHalf);
        static const std::string floats(kOfxBitDepthFloat);

        if(depth == none)
          return none;

        if(isPixelDepthSupported(depth))
          return depth;
        
        if(depth == floats) {
          if(isPixelDepthSupported(shorts))
            return shorts;
          if(isPixelDepthSupported(bytes))
            return bytes;
        }
          
        if(depth == halfs) {
          if(isPixelDepthSupported(floats))
            return floats;
          if(isPixelDepthSupported(shorts))
            return shorts;
          if(isPixelDepthSupported(bytes))
            return bytes;
        }
          
        if(depth == shorts) {
          if(isPixelDepthSupported(floats))
            return floats;
          if(isPixelDepthSupported(bytes))
            return bytes;
        }

        if(depth == bytes) {
          if(isPixelDepthSupported(shorts))
            return shorts;
          if(isPixelDepthSupported(floats))
            return floats;
        }
        
        /// WTF? Something wrong here
        return none;          
      }


      /// get the interact description, this will also call describe on the interact
      Interact::Descriptor &Instance::getOverlayDescriptor(int bitDepthPerComponent, bool hasAlpha)
      {
        return _descriptor->getOverlayDescriptor(bitDepthPerComponent, hasAlpha);
      }

      OfxStatus Instance::getTimeDomainAction(OfxRangeD& range)
      {
        static const Property::PropSpec outStuff[] = {
          { kOfxImageEffectPropFrameRange , Property::eDouble, 2, false, "0.0" },
          Property::propSpecEnd
        };

        Property::Set outArgs(outStuff);  

#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetTimeDomain<<"()"<<std::endl;
#       endif
        OfxStatus st = mainEntry(kOfxImageEffectActionGetTimeDomain,
                                 this->getHandle(),
                                 0,
                                 &outArgs);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetTimeDomain<<"()->"<<StatStr(st);
          if (st == kOfxStatOK) {
              std::cout << ": ("<<outArgs.getDoubleProperty(kOfxImageEffectPropFrameRange,0)<<","<<outArgs.getDoubleProperty(kOfxImageEffectPropFrameRange,1)<<")";
          }
          std::cout << std::endl;
#       endif
        if(st!=kOfxStatOK) return st;

        range.min = outArgs.getDoubleProperty(kOfxImageEffectPropFrameRange,0);
        range.max = outArgs.getDoubleProperty(kOfxImageEffectPropFrameRange,1);

        return kOfxStatOK;
      }

      
      /// implemented for Param::SetInstance
      void Instance::paramChangedByPlugin(Param::Instance *param)
      {
        if (!_created) {
          // setValue() was probably called from kOfxActionCreateInstance 
          // this is legal according to http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#SettingParams
          // but kOfxActionInstanceChanged should not be called according to the preconditions of http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionInstanceChanged
          return;
        }
        double frame  = getFrameRecursive();
        OfxPointD renderScale; getRenderScaleRecursive(renderScale.x, renderScale.y);

        beginInstanceChangedAction(kOfxChangePluginEdited);
        paramInstanceChangedAction(param->getName(), kOfxChangePluginEdited, frame, renderScale);
        endInstanceChangedAction(kOfxChangePluginEdited);
      }

      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      /// The image effect suite functions
      static OfxStatus getPropertySet(OfxImageEffectHandle h1, 
                                      OfxPropertySetHandle *h2)
      {        
        try {
        if (!h2) {
          return kOfxStatErrBadHandle;
        }

        Base *effectBase = reinterpret_cast<Base*>(h1);

        if (!effectBase || !effectBase->verifyMagic()) {
          *h2 = NULL;

          return kOfxStatErrBadHandle;
        }

        *h2 = effectBase->getProps().getHandle();

        return kOfxStatOK;
        } catch (...) {
          *h2 = NULL;

          return kOfxStatErrBadHandle;
        }
      }

      static OfxStatus getParamSet(OfxImageEffectHandle h1, 
                                   OfxParamSetHandle *h2)
      {
        try {
        if (!h2) {
          return kOfxStatErrBadHandle;
        }

        ImageEffect::Base *effectBase = reinterpret_cast<ImageEffect::Base*>(h1);

        if (!effectBase || !effectBase->verifyMagic()) {
          *h2 = NULL;

          return kOfxStatErrBadHandle;
        }

        ImageEffect::Descriptor *effectDescriptor = dynamic_cast<ImageEffect::Descriptor*>(effectBase);

        if(effectDescriptor) {
          *h2 = effectDescriptor->getParamSetHandle();

          return kOfxStatOK;
        }

        ImageEffect::Instance *effectInstance = dynamic_cast<ImageEffect::Instance*>(effectBase);

        if(effectInstance) {
          *h2 = effectInstance->getParamSetHandle();

          return kOfxStatOK;
        }

        *h2 = NULL;

        return kOfxStatErrBadHandle;
        } catch (...) {
          *h2 = NULL;

          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus clipDefine(OfxImageEffectHandle h1, 
                                  const char *name, 
                                  OfxPropertySetHandle *h2)
      {
        try {
        if (!h2) {
          return kOfxStatErrBadHandle;
        }

        ImageEffect::Base *effectBase = reinterpret_cast<ImageEffect::Base*>(h1);
        
        if (!effectBase || !effectBase->verifyMagic()) {
          *h2 = NULL;

          return kOfxStatErrBadHandle;
        }

        ImageEffect::Descriptor *effectDescriptor = dynamic_cast<ImageEffect::Descriptor*>(effectBase);
        
        if(effectDescriptor){ 
          ClipDescriptor *clip = effectDescriptor->defineClip(name);
          *h2 = clip->getPropHandle();

          return kOfxStatOK;
        }

        *h2 = NULL;

        return kOfxStatErrBadHandle;
        } catch (...) {
          *h2 = NULL;

          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus clipGetPropertySet(OfxImageClipHandle clip,
                                          OfxPropertySetHandle *propHandle){        
        try {
        if (!propHandle) {
          return kOfxStatErrBadHandle;
        }

        ClipInstance *clipInstance = reinterpret_cast<ClipInstance*>(clip);

        if (!clipInstance || !clipInstance->verifyMagic()) {
          *propHandle = NULL;

          return kOfxStatErrBadHandle;
        }

        *propHandle = clipInstance->getPropHandle();
        return kOfxStatOK;
        } catch (...) {
          *propHandle = NULL;

          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus clipGetImage(OfxImageClipHandle h1, 
                                    OfxTime time, 
                                    const OfxRectD *h2,
                                    OfxPropertySetHandle *h3)
      {
        ClipInstance *clipInstance = nullptr;
        try {
        if (!h3) {
          std::cerr << "[clipGetImage] h3 is null" << std::endl;
          return kOfxStatErrBadHandle;
        }

        clipInstance = reinterpret_cast<ClipInstance*>(h1);

        if (!clipInstance) {
          std::cerr << "[clipGetImage] clipInstance is null" << std::endl;
          *h3 = NULL;
          return kOfxStatErrBadHandle;
        }
        if (!clipInstance->verifyMagic()) {
          std::cerr << "[clipGetImage] verifyMagic failed for clip=" << clipInstance->getName() << std::endl;
          *h3 = NULL;
          return kOfxStatErrBadHandle;
        }

        Image* image = clipInstance->getImage(time,h2);
        if(!image) {
          std::cerr << "[clipGetImage] getImage returned null for clip=" << clipInstance->getName() << " time=" << time << std::endl;
          *h3 = NULL;
          return kOfxStatFailed;
        }

        *h3 = image->getPropHandle();
        if (!*h3) {
          std::cerr << "[clipGetImage] getPropHandle returned null for clip=" << clipInstance->getName() << std::endl;
          return kOfxStatErrBadHandle;
        }

        return kOfxStatOK;
        } catch (const std::exception &e) {
          std::cerr << "[clipGetImage] exception: " << e.what() << " clip=" << (clipInstance ? clipInstance->getName() : "null") << std::endl;
          *h3 = NULL;
          return kOfxStatErrBadHandle;
        } catch (...) {
          std::cerr << "[clipGetImage] unknown exception clip=" << (clipInstance ? clipInstance->getName() : "null") << std::endl;
          *h3 = NULL;
          return kOfxStatErrBadHandle;
        }
      }

      static OfxStatus clipReleaseImage(OfxPropertySetHandle h1)
      {
        try {
        Property::Set *pset = reinterpret_cast<Property::Set*>(h1);

        if (!pset || !pset->verifyMagic()) {
          return kOfxStatErrBadHandle;
        }

        Image *image = dynamic_cast<Image*>(pset);

        if(image){
          // clip::image has a virtual destructor for derived classes
          image->releaseReference();

          return kOfxStatOK;
        }

        return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus clipGetHandle(OfxImageEffectHandle imageEffect,
                                     const char *name,
                                     OfxImageClipHandle *clip,
                                     OfxPropertySetHandle *propertySet)
      {
        try {
        if (!clip) {
          return kOfxStatErrBadHandle;
        }

        ImageEffect::Base *effectBase = reinterpret_cast<ImageEffect::Base*>(imageEffect);

        if (!effectBase || !effectBase->verifyMagic()) {
          return kOfxStatErrBadHandle;
        }

        ImageEffect::Instance *effectInstance = reinterpret_cast<ImageEffect::Instance*>(effectBase);

        if(effectInstance){
          ClipInstance* instance = effectInstance->getClip(name);
          if(!instance)
            return kOfxStatErrBadHandle;
          *clip = instance->getHandle();
          if(propertySet)
            *propertySet = instance->getPropHandle();
          return kOfxStatOK;
        }

        return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus clipGetRegionOfDefinition(OfxImageClipHandle clip,
                                                 OfxTime time,
                                                 OfxRectD *bounds)
      {
        ClipInstance *clipInstance = nullptr;
        try {
        if (!bounds) {
          std::cerr << "[clipGetRegionOfDefinition] bounds is null" << std::endl;
          return kOfxStatErrBadHandle;
        }

        clipInstance = reinterpret_cast<ClipInstance*>(clip);

        if (!clipInstance) {
          std::cerr << "[clipGetRegionOfDefinition] clipInstance is null" << std::endl;
          bounds->x1 = bounds->y1 = bounds->x2 = bounds->y2 = 0.;
          return kOfxStatErrBadHandle;
        }
        if (!clipInstance->verifyMagic()) {
          std::cerr << "[clipGetRegionOfDefinition] verifyMagic failed for clip=" << clipInstance->getName() << std::endl;
          bounds->x1 = bounds->y1 = bounds->x2 = bounds->y2 = 0.;
          return kOfxStatErrBadHandle;
        }

        *bounds = clipInstance->getRegionOfDefinition(time);
        if (bounds->x2 < bounds->x1 || bounds->y2 < bounds->y1) {
          std::cerr << "[clipGetRegionOfDefinition] invalid RoD for clip=" << clipInstance->getName() << std::endl;
          return kOfxStatFailed;
        }

        return kOfxStatOK;
        } catch (const std::exception &e) {
          std::cerr << "[clipGetRegionOfDefinition] exception: " << e.what() << " clip=" << (clipInstance ? clipInstance->getName() : "null") << std::endl;
          bounds->x1 = bounds->y1 = bounds->x2 = bounds->y2 = 0.;
          return kOfxStatErrBadHandle;
        } catch (...) {
          std::cerr << "[clipGetRegionOfDefinition] unknown exception clip=" << (clipInstance ? clipInstance->getName() : "null") << std::endl;
          bounds->x1 = bounds->y1 = bounds->x2 = bounds->y2 = 0.;
          return kOfxStatErrBadHandle;
        }
      }

      // should processing be aborted?
      static int abort(OfxImageEffectHandle imageEffect)
      {
        try {
        ImageEffect::Base *effectBase = reinterpret_cast<ImageEffect::Base*>(imageEffect);

        if (!effectBase || !effectBase->verifyMagic()) {
          return kOfxStatErrBadHandle;
        }

        ImageEffect::Instance *effectInstance = dynamic_cast<ImageEffect::Instance*>(effectBase);

        if(effectInstance) 
          return effectInstance->abort();
        else 
          return kOfxStatErrBadHandle;        
        } catch (...) {
          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus imageMemoryAlloc(OfxImageEffectHandle instanceHandle, 
                                        size_t nBytes,
                                        OfxImageMemoryHandle *memoryHandle)
      {
        try {
        if (!memoryHandle) {
          return kOfxStatErrBadHandle;
        }

        ImageEffect::Base *effectBase = reinterpret_cast<ImageEffect::Base*>(instanceHandle);
        ImageEffect::Instance *effectInstance = reinterpret_cast<ImageEffect::Instance*>(effectBase);
        Memory::Instance* memory;

        if(effectInstance){

          if (!effectInstance->verifyMagic()) {
            *memoryHandle = NULL;
            return kOfxStatErrBadHandle;
          }

          memory = effectInstance->imageMemoryAlloc(nBytes);
        }
        else {
          memory = gImageEffectHost->imageMemoryAlloc(nBytes);
        }

        if (memory) {
          *memoryHandle = memory->getHandle();
          return kOfxStatOK;
        } else {
          *memoryHandle = NULL;
          return kOfxStatErrMemory;
        }
        } catch (std::bad_alloc&) {
          *memoryHandle = NULL;
          return kOfxStatErrMemory;
        } catch (...) {
          *memoryHandle = NULL;
          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus imageMemoryFree(OfxImageMemoryHandle memoryHandle){
        try {
        Memory::Instance *memoryInstance = reinterpret_cast<Memory::Instance*>(memoryHandle);

        if(memoryInstance && memoryInstance->verifyMagic()) {
          memoryInstance->freeMem();
          delete memoryInstance;
          return kOfxStatOK;
        }
        else 
          return kOfxStatErrBadHandle; 
        } catch (...) {
          return kOfxStatErrBadHandle;
        }
      }

      static
      OfxStatus imageMemoryLock(OfxImageMemoryHandle memoryHandle,
                                void **returnedPtr){
        try {
        if (!returnedPtr) {
          return kOfxStatErrBadHandle;
        }

        Memory::Instance *memoryInstance = reinterpret_cast<Memory::Instance*>(memoryHandle);

        if(memoryInstance && memoryInstance->verifyMagic()) {
          memoryInstance->lock();          
          *returnedPtr = memoryInstance->getPtr();

          return (*returnedPtr) ? kOfxStatOK : kOfxStatErrMemory;
        }

        *returnedPtr = NULL;

        return kOfxStatErrBadHandle; 
        } catch (...) {
          *returnedPtr = NULL;
          return kOfxStatErrBadHandle;
        }
      }
      
      static OfxStatus imageMemoryUnlock(OfxImageMemoryHandle memoryHandle){
        try {
        Memory::Instance *memoryInstance = reinterpret_cast<Memory::Instance*>(memoryHandle);

        if(memoryInstance && memoryInstance->verifyMagic()){
          memoryInstance->unlock();

          return kOfxStatOK;
        }

        return kOfxStatErrBadHandle; 
        } catch (...) {
          return kOfxStatErrBadHandle;
        }
      }

      static const struct OfxImageEffectSuiteV1 gImageEffectSuite = {
        getPropertySet,
        getParamSet,
        clipDefine,
        clipGetHandle,
        clipGetPropertySet,
        clipGetImage,
        clipReleaseImage,
        clipGetRegionOfDefinition,
        abort,
        imageMemoryAlloc,
        imageMemoryFree,
        imageMemoryLock,
        imageMemoryUnlock
      };

#   ifdef OFX_SUPPORTS_OPENGLRENDER
      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      /// The OpenGL render suite functions

      static OfxStatus clipLoadTexture(OfxImageClipHandle h1,
                                       OfxTime time,
                                       const char   *format,
                                       const OfxRectD *h2,
                                       OfxPropertySetHandle *h3)
      {
        try {
        if (!h3) {
          return kOfxStatErrBadHandle;
        }

        ClipInstance *clipInstance = reinterpret_cast<ClipInstance*>(h1);

        if (!clipInstance || !clipInstance->verifyMagic()) {
          return kOfxStatErrBadHandle;
        }

        if(clipInstance){
          Texture* texture = clipInstance->loadTexture(time,format,h2);
          if(!texture) {
            *h3 = NULL;

            return kOfxStatFailed;
          }

          *h3 = texture->getPropHandle();

          return kOfxStatOK;
        }
        
        return kOfxStatErrBadHandle;
        } catch (...) {
          *h3 = NULL;

          return kOfxStatErrBadHandle;
        }
      }

      static OfxStatus clipFreeTexture(OfxPropertySetHandle h1)
      {
        try {
        Property::Set *pset = reinterpret_cast<Property::Set*>(h1);

        if (!pset || !pset->verifyMagic()) {
          return kOfxStatErrBadHandle;
        }

        Texture *texture = dynamic_cast<Texture*>(pset);

        if(texture){
          // clip::texture has a virtual destructor for derived classes
          texture->releaseReference();
          return kOfxStatOK;
        }
        else 
          return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatErrBadHandle;
        }
      }

      static OfxStatus flushResources( )
      {
        return gImageEffectHost->flushOpenGLResources();
      }

      static const struct OfxImageEffectOpenGLRenderSuiteV1 gOpenGLRenderSuite = {
        clipLoadTexture,
        clipFreeTexture,
        flushResources
      };
#   endif

      /// message suite function for an image effect
      static OfxStatus message(void *handle, const char *type, const char *id, const char *format, ...)
      {
        try {
        ImageEffect::Instance *effectInstance = reinterpret_cast<ImageEffect::Instance*>(handle);
        OfxStatus stat;
        if(effectInstance){
          va_list args;
          va_start(args,format);
          stat = effectInstance->vmessage(type,id,format,args);
          va_end(args);
        }
        else{
          va_list args;
          va_start(args,format);
          vprintf(format,args);
          va_end(args);
          stat = kOfxStatErrBadHandle;
        }
        return stat;
        } catch (...) {
          return kOfxStatFailed;
        }
      }

      static OfxStatus setPersistentMessage(void *handle, const char *type, const char *id, const char *format, ...)
      {
        try {
          ImageEffect::Instance *effectInstance = reinterpret_cast<ImageEffect::Instance*>(handle);
          OfxStatus stat;
          if(effectInstance){
            va_list args;
            va_start(args,format);
            stat = effectInstance->setPersistentMessage(type,id,format,args);
            va_end(args);
          }
          else{
            va_list args;
            va_start(args,format);
            vprintf(format,args);
            va_end(args);
            stat = kOfxStatErrBadHandle;
          }
          return stat;
        } catch (...) {
          return kOfxStatFailed;
        }
      }

      static OfxStatus clearPersistentMessage(void *handle)
      {
        try {
          ImageEffect::Instance *effectInstance = reinterpret_cast<ImageEffect::Instance*>(handle);
          OfxStatus stat;
          if(effectInstance){
            stat = effectInstance->clearPersistentMessage();
          }
          else{
            stat = kOfxStatErrBadHandle;
          }
          return stat;
        } catch (...) {
          return kOfxStatFailed;
        }
      }

      /// message suite for an image effect plugin (backward-compatible with OfxMessageSuiteV1)
      static const struct OfxMessageSuiteV2 gMessageSuite = {
        message,
        setPersistentMessage,
        clearPersistentMessage
      };


      ////////////////////////////////////////////////////////////////////////////////
      /// make an overlay interact for an image effect
      OverlayInteract::OverlayInteract(ImageEffect::Instance &effect, int bitDepthPerComponent, bool hasAlpha)
        :  Interact::Instance(effect.getOverlayDescriptor(bitDepthPerComponent, hasAlpha),
                              (void *)(effect.getHandle()))
        , _instance(effect)
      {        
      }

      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      // Progress suite functions

      /// begin progressing
      static OfxStatus ProgressStartV1(void *effectInstance,
                                       const char *label)
      {
        if (!effectInstance)
          return kOfxStatErrBadHandle;
        Instance *me = reinterpret_cast<Instance *>(effectInstance);
        me->progressStart(label, "");
        return kOfxStatOK;
      }
      

      /// begin progressing
      static OfxStatus ProgressStart(void *effectInstance,
                                     const char *message,
                                     const char *messageid)
      {
        if (!effectInstance)
          return kOfxStatErrBadHandle;
        Instance *me = reinterpret_cast<Instance *>(effectInstance);
        me->progressStart(message, messageid);
        return kOfxStatOK;
      }
      
      /// finish progressing
      static OfxStatus ProgressEnd(void *effectInstance)
      {
        if (!effectInstance)
          return kOfxStatErrBadHandle;
        Instance *me = reinterpret_cast<Instance *>(effectInstance);
        me->progressEnd();
        return kOfxStatOK;
      }

      /// update progressing
      static OfxStatus ProgressUpdate(void *effectInstance, double progress)
      {
        if (!effectInstance)
          return kOfxStatErrBadHandle;
        Instance *me = reinterpret_cast<Instance *>(effectInstance);
        bool v = me->progressUpdate(progress);
        return v ? kOfxStatOK : kOfxStatReplyNo;          
      }

      /// our progress suite
      struct OfxProgressSuiteV1 gProgressSuiteV1 = {
        ProgressStartV1,
        ProgressUpdate,
        ProgressEnd
      };

      struct OfxProgressSuiteV2 gProgressSuiteV2 = {
        ProgressStart,
        ProgressUpdate,
        ProgressEnd
      };


      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      // timeline suite functions

      /// timeline suite function
      static OfxStatus TimeLineGetTime(void *effectInstance, double *time)
      {
        if (!effectInstance)
          return kOfxStatErrBadHandle;
        Instance *me = reinterpret_cast<Instance *>(effectInstance);
        *time = me->timeLineGetTime();
        return kOfxStatOK;
      }

      /// timeline suite function
      static OfxStatus TimeLineGotoTime(void *effectInstance, double time)
      {
        if (!effectInstance)
          return kOfxStatErrBadHandle;
        Instance *me = reinterpret_cast<Instance *>(effectInstance);
        me->timeLineGotoTime(time);
        return kOfxStatOK;
      }
      
      /// timeline suite function
      static OfxStatus TimeLineGetBounds(void *effectInstance, double *firstTime, double *lastTime)
      {
        if (!effectInstance)
          return kOfxStatErrBadHandle;
        Instance *me = reinterpret_cast<Instance *>(effectInstance);
        me->timeLineGetBounds(*firstTime, *lastTime);
        return kOfxStatOK;
      }

      /// our progress suite
      struct OfxTimeLineSuiteV1 gTimelineSuite = {
        TimeLineGetTime,
        TimeLineGotoTime,
        TimeLineGetBounds
      };

      ////////////////////////////////////////////////////////////////////////////////
#ifdef OFX_SUPPORTS_MULTITHREAD
      // Forward all multithread suite calls to the host implementation.
 
      static OfxStatus multiThread(OfxThreadFunctionV1 func,
                                   unsigned int nThreads,
                                   void *customArg)
      {
        return gImageEffectHost->multiThread(func, nThreads, customArg);
      }

      static OfxStatus multiThreadNumCPUs(unsigned int *nCPUs)
      {
        return gImageEffectHost->multiThreadNumCPUS(nCPUs);
      }

      static OfxStatus multiThreadIndex(unsigned int *threadIndex){
        return gImageEffectHost->multiThreadIndex(threadIndex);
      }

      static int multiThreadIsSpawnedThread(void){
        return gImageEffectHost->multiThreadIsSpawnedThread();
      }

      static OfxStatus mutexCreate(OfxMutexHandle *mutex, int lockCount)
      {
        return gImageEffectHost->mutexCreate(mutex, lockCount);
      }

      static OfxStatus mutexDestroy(const OfxMutexHandle mutex)
      {
        return gImageEffectHost->mutexDestroy(mutex);
      }

      static OfxStatus mutexLock(const OfxMutexHandle mutex){
        return gImageEffectHost->mutexLock(mutex);
      }
       
      static OfxStatus mutexUnLock(const OfxMutexHandle mutex){
        return gImageEffectHost->mutexUnLock(mutex);
      }       

      static OfxStatus mutexTryLock(const OfxMutexHandle mutex){
        return gImageEffectHost->mutexTryLock(mutex);
      }
#else // !OFX_SUPPORTS_MULTITHREAD
      /// a simple multithread suite
      static OfxStatus multiThread(OfxThreadFunctionV1 func,
                                   unsigned int /*nThreads*/,
                                   void *customArg)
      {
        if (!func)
          return kOfxStatFailed;
        func(0,1,customArg);
        return kOfxStatOK;
      }

      static OfxStatus multiThreadNumCPUs(unsigned int *nCPUs)
      {
        if (!nCPUs)
          return kOfxStatFailed;
        *nCPUs = 1;
        return kOfxStatOK;
      }

      static OfxStatus multiThreadIndex(unsigned int *threadIndex){
        if (!threadIndex)
          return kOfxStatFailed;
        *threadIndex = 0;
        return kOfxStatOK;
      }

      static int multiThreadIsSpawnedThread(void){
        return false;
      }

      static OfxStatus mutexCreate(OfxMutexHandle *mutex, int /*lockCount*/)
      {
        if (!mutex)
          return kOfxStatFailed;
        // do nothing single threaded
        *mutex = 0;
        return kOfxStatOK;
      }

      static OfxStatus mutexDestroy(const OfxMutexHandle mutex)
      {
        if (mutex != 0)
          return kOfxStatErrBadHandle;
        // do nothing single threaded
        return kOfxStatOK;
      }

      static OfxStatus mutexLock(const OfxMutexHandle mutex){
        if (mutex != 0)
          return kOfxStatErrBadHandle;
        // do nothing single threaded
        return kOfxStatOK;
      }
       
      static OfxStatus mutexUnLock(const OfxMutexHandle mutex){
        if (mutex != 0)
          return kOfxStatErrBadHandle;
        // do nothing single threaded
        return kOfxStatOK;
      }       

      static OfxStatus mutexTryLock(const OfxMutexHandle mutex){
        if (mutex != 0)
          return kOfxStatErrBadHandle;
        // do nothing single threaded
        return kOfxStatOK;
      }
#endif // !OFX_SUPPORTS_MULTITHREAD
       
      static const struct OfxMultiThreadSuiteV1 gMultiThreadSuite = {
        multiThread,
        multiThreadNumCPUs,
        multiThreadIndex,
        multiThreadIsSpawnedThread,
        mutexCreate,
        mutexDestroy,
        mutexLock,
        mutexUnLock,
        mutexTryLock
      };
      

      ////////////////////////////////////////////////////////////////////////////////
      /// The image effect host

      /// properties for the image effect host
      static const Property::PropSpec hostStuffs[] = {
        { kOfxImageEffectHostPropIsBackground, Property::eInt, 1, true, "0" },
        { kOfxImageEffectPropSupportsOverlays, Property::eInt, 1, true, "1" },
        { kOfxImageEffectPropSupportsMultiResolution, Property::eInt, 1, true, "1" },
        { kOfxImageEffectPropSupportsTiles, Property::eInt, 1, true, "1" },
        { kOfxImageEffectPropTemporalClipAccess, Property::eInt, 1, true, "1"  },
      
        

        /// xxx this needs defaulting manually
        { kOfxImageEffectPropSupportedComponents, Property::eString, 0, true, "" },
        /// xxx this needs defaulting manually

        { kOfxImageEffectPropSupportedPixelDepths, Property::eString, 0, true, "" },

        /// xxx this needs defaulting manually
  
        { kOfxImageEffectPropSupportedContexts, Property::eString, 0, true, "" },
        /// xxx this needs defaulting manually

        { kOfxImageEffectPropSupportsMultipleClipDepths, Property::eInt, 1, true, "1" },
        { kOfxImageEffectPropSupportsMultipleClipPARs, Property::eInt, 1, true, "0" },
        { kOfxImageEffectPropSetableFrameRate, Property::eInt, 1, true, "0" },
        { kOfxImageEffectPropSetableFielding, Property::eInt, 1, true, "0"  },
        { kOfxParamHostPropSupportsCustomInteract, Property::eInt, 1, true, "0" },
        { kOfxParamHostPropSupportsStringAnimation, Property::eInt, 1, true, "0" },
        { kOfxParamHostPropSupportsChoiceAnimation, Property::eInt, 1, true, "0"  },
        { kOfxParamHostPropSupportsBooleanAnimation, Property::eInt, 1, true, "0" },
        { kOfxParamHostPropSupportsCustomAnimation, Property::eInt, 1, true, "0" },
        { kOfxPropHostOSHandle, Property::ePointer, 1, true, NULL },
#ifdef OFX_SUPPORTS_PARAMETRIC
        { kOfxParamHostPropSupportsParametricAnimation, Property::eInt, 1, true, "0"},
#endif
        { kOfxParamHostPropMaxParameters, Property::eInt, 1, true, "-1" },
        { kOfxParamHostPropMaxPages, Property::eInt, 1, true, "0" },
        { kOfxParamHostPropPageRowColumnCount, Property::eInt, 2, true, "0" },
        { kOfxImageEffectInstancePropSequentialRender, Property::eInt, 1, true, "0" }, // OFX 1.2
#ifdef OFX_SUPPORTS_OPENGLRENDER
        { kOfxImageEffectPropOpenGLRenderSupported, Property::eString, 1, true, "false"}, // OFX 1.3
	{ kOfxImageEffectPropCudaRenderSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropCudaStreamSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropMetalRenderSupported, Property::eString, 1, false, "false" },
	{ kOfxImageEffectPropOpenCLRenderSupported, Property::eString, 1, false, "false" },
#endif
        { kOfxImageEffectPropRenderQualityDraft, Property::eInt, 1, true, "0" }, // OFX 1.4
        { kOfxImageEffectHostPropNativeOrigin, Property::eString, 0, true, kOfxHostNativeOriginBottomLeft }, // OFX 1.4
        Property::propSpecEnd
      };

      /// ctor
      Host::Host()
      {
        /// add the properties for an image effect host, derived classs to set most of them
        _properties.addProperties(hostStuffs);
      }

      /// optionally over-ridden function to register the creation of a new descriptor in the host app
      void Host::initDescriptor(Descriptor* /*desc*/)
      {
      }

      /// Use this in any dialogue etc... showing progress
      void Host::loadingStatus(const std::string &)
      {
      }

      bool Host::pluginSupported(ImageEffectPlugin */*plugin*/, std::string &/*reason*/) const
      {
        return true;
      }

      // override this to use your own memory instance - must inherrit from memory::instance
      Memory::Instance* Host::newMemoryInstance(size_t /*nBytes*/) {
        return 0;
      }

      // return an memory::instance calls makeMemoryInstance that can be overriden
      Memory::Instance* Host::imageMemoryAlloc(size_t nBytes){
        Memory::Instance* instance = newMemoryInstance(nBytes);
        if(instance)
          return instance;
        else{
          Memory::Instance* instance = new Memory::Instance;
          instance->alloc(nBytes);
          return instance;
        }
      }

      /// our suite fetcher
      const void *Host::fetchSuite(const char *suiteName, int suiteVersion)
      {
        if (strcmp(suiteName, kOfxImageEffectSuite)==0) {
          if(suiteVersion==1)
            return (void *)&gImageEffectSuite;
          else
            return NULL;
        }
        else if (strcmp(suiteName, kOfxParameterSuite)==0) {
          return Param::GetSuite(suiteVersion);
        }
        else if (strcmp(suiteName, kOfxMessageSuite)==0) {
          // version 2 is backward-compatible
          if(suiteVersion==1 || suiteVersion==2)
            return (void *)&gMessageSuite;
          else 
            return NULL;
        }
        else if (strcmp(suiteName, kOfxInteractSuite)==0) {
          return Interact::GetSuite(suiteVersion);
        }
        else if (strcmp(suiteName, kOfxProgressSuite)==0) {
          if(suiteVersion==1) 
            return (void*)&gProgressSuiteV1;
          else if(suiteVersion==2)
            return (void*)&gProgressSuiteV2;
          else
            return 0;
        }
        else if (strcmp(suiteName, kOfxTimeLineSuite)==0) {
          if(suiteVersion==1) 
            return (void*)&gTimelineSuite;
          else
            return 0;
        }
        else if (strcmp(suiteName, kOfxMultiThreadSuite)==0) {
          if(suiteVersion == 1)
            return (void*)&gMultiThreadSuite;
          else 
            return NULL;
        }
#     ifdef OFX_SUPPORTS_OPENGLRENDER
        else if (strcmp(suiteName, kOfxOpenGLRenderSuite)==0) {
          if(suiteVersion == 1)
            return (void*)&gOpenGLRenderSuite;
          else 
            return NULL;
        }
#     endif
#     ifdef OFX_SUPPORTS_PARAMETRIC
        else if (strcmp(suiteName, kOfxParametricParameterSuite)==0) {
          return ParametricParam::GetSuite(suiteVersion);
        }
#     endif
        else  /// otherwise just grab the base class one, which is props and memory
          return OFX::Host::Host::fetchSuite(suiteName, suiteVersion);
      }

    } // ImageEffect

  } // Host

} // OFX
