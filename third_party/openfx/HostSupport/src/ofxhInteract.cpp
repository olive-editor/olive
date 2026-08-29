

// ofx
#include "ofxKeySyms.h"
#include "ofxCore.h"
#include "ofxImageEffect.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhImageEffect.h"
#include "ofxhInteract.h"
#include "ofxOld.h" // old plugins may rely on deprecated properties being present

namespace OFX {

  namespace Host {

    namespace Interact {

      //
      // descriptor
      //
      static const Property::PropSpec interactDescriptorStuffs[] = {
        { kOfxInteractPropHasAlpha , Property::eInt, 1, true, "0" },
        { kOfxInteractPropBitDepth , Property::eInt, 1, true, "0" },
        Property::propSpecEnd
      };

      Descriptor::Descriptor()
        : _properties(interactDescriptorStuffs) 
        , _state(eUninitialised)
        , _entryPoint(NULL)
      {
      }

      Descriptor::~Descriptor()
      {
      }

      /// call describe on this descriptor
      bool Descriptor::describe(int bitDepthPerComponent, bool hasAlpha)
      {
        if(_state == eUninitialised) {
          _properties.setIntProperty(kOfxInteractPropBitDepth, bitDepthPerComponent);
          _properties.setIntProperty(kOfxInteractPropHasAlpha, (int)(hasAlpha));

          OfxStatus stat = callEntry(kOfxActionDescribe, getHandle(), NULL, NULL);
          if(stat == kOfxStatOK || stat == kOfxStatReplyDefault) {
            _state = eDescribed;
          }
          else {
            _state = eFailed;
          }
        }
        return _state == eDescribed;
      }

      // call the interactive entry point
      OfxStatus Descriptor::callEntry(const char *action, 
                                      void *handle,
                                      OfxPropertySetHandle inArgs, 
                                      OfxPropertySetHandle outArgs)
      {
        if(_entryPoint && _state != eFailed) {
          return _entryPoint(action, handle, inArgs, outArgs);
        }
        else
          return kOfxStatFailed;
        
        return kOfxStatOK;
      }


      ////////////////////////////////////////////////////////////////////////////////
      static const Property::PropSpec interactInstanceStuffs[] = {
        { kOfxPropEffectInstance, Property::ePointer, 1, true, NULL },
        { kOfxPropInstanceData, Property::ePointer, 1, false, NULL },
        { kOfxInteractPropPixelScale, Property::eDouble, 2, true, "1.0f" },
        { kOfxInteractPropBackgroundColour , Property::eDouble, 3, true, "0.0f" },
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        { kOfxInteractPropViewportSize, Property::eDouble, 2, true, "100.0f" },
#endif
        { kOfxInteractPropSlaveToParam , Property::eString, 0, false, ""},
        { kOfxInteractPropSuggestedColour , Property::eDouble, 3, true, "1.0f" },
        Property::propSpecEnd
      };

      static const Property::PropSpec interactArgsStuffs[] = {
        { kOfxPropEffectInstance, Property::ePointer, 1, false, NULL },
        { kOfxPropTime, Property::eDouble, 1, false, "0.0" },
        { kOfxImageEffectPropRenderScale, Property::eDouble, 2, false, "0.0" },
        { kOfxInteractPropBackgroundColour , Property::eDouble, 3, false, "0.0f" },
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        { kOfxInteractPropViewportSize, Property::eDouble, 2, false, "0.0f" },
#endif
        { kOfxInteractPropPixelScale, Property::eDouble, 2, false, "1.0f" },
        { kOfxInteractPropPenPosition, Property::eDouble, 2, false, "0.0" },
        { kOfxInteractPropPenViewportPosition, Property::eInt, 2, false, "0" }, // new in OFX 1.2
        { kOfxInteractPropPenPressure, Property::eDouble, 1, false, "0.0" },
        { kOfxPropKeyString, Property::eString, 1, false, "" },
        { kOfxPropKeySym, Property::eInt, 1, false, "0" },
        Property::propSpecEnd
      };

      // instance

      Instance::Instance(Descriptor& desc, void *effectInstance) 
        : _descriptor(desc)
        , _properties(interactInstanceStuffs)
        , _state(desc.getState())
        , _effectInstance(effectInstance)
        , _argProperties(interactArgsStuffs)
      {
        _properties.setPointerProperty(kOfxPropEffectInstance, effectInstance);
        _properties.setChainedSet(&desc.getProperties()); /// chain it into the descriptor props
        _properties.setGetHook(kOfxInteractPropPixelScale, this);
        _properties.setGetHook(kOfxInteractPropBackgroundColour,this);
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        _properties.setGetHook(kOfxInteractPropViewportSize,this);
#endif
        _properties.setGetHook(kOfxInteractPropSuggestedColour,this);

        _argProperties.setGetHook(kOfxInteractPropPixelScale, this);
        _argProperties.setGetHook(kOfxInteractPropBackgroundColour,this);
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        _argProperties.setGetHook(kOfxInteractPropViewportSize,this);
#endif
      }

      Instance::~Instance()
      {
        /// call it directly incase CI failed and we should always tidy up after create instance
        callEntry(kOfxActionDestroyInstance,  NULL);
      }

      /// call the entry point in the descriptor with action and the given args
      OfxStatus Instance::callEntry(const char *action, Property::Set *inArgs)
      {
        if(_state != eFailed) {
          OfxPropertySetHandle inHandle = inArgs ? inArgs->getHandle() : NULL ;
          return _descriptor.callEntry(action, getHandle(), inHandle, NULL);
        }
        return kOfxStatFailed;
      }
      
      // do nothing
      int Instance::getDimension(const std::string &name) const
      {
        if(name == kOfxInteractPropPixelScale){
          return 2;
        }
        else if(name == kOfxInteractPropBackgroundColour){
          return 3;
        }
        else if(name == kOfxInteractPropSuggestedColour
                ){
          return 3;
        }
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        else if(name == kOfxInteractPropViewportSize){
          return 2;
        }
#endif
        else
          throw Property::Exception(kOfxStatErrValue);
      }
        
      // do nothing function
      void Instance::reset(const std::string &/*name*/)
      {
        // no-op
      }

      double Instance::getDoubleProperty(const std::string &name, int index) const
      {   
        if(name == kOfxInteractPropPixelScale){
          if(index>=2) throw Property::Exception(kOfxStatErrBadIndex);
          double first[2];
          getPixelScale(first[0],first[1]);
          return first[index];
        }
        else if(name == kOfxInteractPropBackgroundColour){
          if(index>=3) throw Property::Exception(kOfxStatErrBadIndex);
          double first[3];
          getBackgroundColour(first[0],first[1],first[2]);
          return first[index];
        }
        else if(name == kOfxInteractPropSuggestedColour
                ){
          if(index>=3) throw Property::Exception(kOfxStatErrBadIndex);
          double first[3];
          bool stat = getSuggestedColour(first[0],first[1],first[2]);
          if (!stat) throw Property::Exception(kOfxStatReplyDefault);
          return first[index];
        }
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        else if(name == kOfxInteractPropViewportSize){
          if(index>=2) throw Property::Exception(kOfxStatErrBadIndex);
          double first[2];
          getViewportSize(first[0],first[1]);
          return first[index];
        }
#endif
        else
          throw Property::Exception(kOfxStatErrUnknown);
      }

      void Instance::getDoublePropertyN(const std::string &name, double *first, int n) const
      {
        if(name == kOfxInteractPropPixelScale){
          if(n>2) throw Property::Exception(kOfxStatErrBadIndex);
          getPixelScale(first[0],first[1]);
        }
        else if(name == kOfxInteractPropBackgroundColour){
          if(n>3) throw Property::Exception(kOfxStatErrBadIndex);
          getBackgroundColour(first[0],first[1],first[2]);
        }
        else if(name == kOfxInteractPropSuggestedColour
                ){
          if(n>3) throw Property::Exception(kOfxStatErrBadIndex);
          bool stat = getSuggestedColour(first[0],first[1],first[2]);
          if (!stat) throw Property::Exception(kOfxStatReplyDefault);
        }
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        else if(name == kOfxInteractPropViewportSize){
          if(n>2) throw Property::Exception(kOfxStatErrBadIndex);
          getViewportSize(first[0],first[1]);
        }
#endif
        else
          throw Property::Exception(kOfxStatErrUnknown);
      }

      void Instance::getSlaveToParam(std::vector<std::string>& params) const
      {        
        int nSlaveParams = _properties.getDimension(kOfxInteractPropSlaveToParam);
                    
        for (int i=0; i<nSlaveParams; i++) {
          std::string param = _properties.getStringProperty(kOfxInteractPropSlaveToParam, i);
          params.push_back(param);
        }
      }    
      
      /// initialise the argument properties
      void Instance::initArgProp(OfxTime time, 
                                 const OfxPointD &renderScale)
      {
        double pixelScale[2];
        getPixelScale(pixelScale[0], pixelScale[1]);  
        _argProperties.setDoublePropertyN(kOfxInteractPropPixelScale, pixelScale, 2);
        _argProperties.setPointerProperty(kOfxPropEffectInstance, _effectInstance);
        _argProperties.setPointerProperty(kOfxPropInstanceData, _properties.getPointerProperty(kOfxPropInstanceData));
        _argProperties.setDoubleProperty(kOfxPropTime,time);
        _argProperties.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);
      }
               
      void Instance::setPenArgProps(const OfxPointD &penPos,
                                    const OfxPointI &penPosViewport,
                                    double  pressure)
      {
        _argProperties.setDoublePropertyN(kOfxInteractPropPenPosition, &penPos.x, 2);
        _argProperties.setIntPropertyN(kOfxInteractPropPenViewportPosition, &penPosViewport.x, 2); // new in OFX 1.2
        _argProperties.setDoubleProperty(kOfxInteractPropPenPressure, pressure);
      }

      void Instance::setKeyArgProps(int     key,
                                    char*   keyString)
      {
        _argProperties.setIntProperty(kOfxPropKeySym,key);
        _argProperties.setStringProperty(kOfxPropKeyString,keyString);
      }

      OfxStatus Instance::createInstanceAction()
      {        
        OfxStatus stat = callEntry(kOfxActionCreateInstance, NULL);
        if(stat == kOfxStatOK || stat == kOfxStatReplyDefault) {
          _state = eCreated;
        }
        else {
          _state = eFailed;
        }
        return stat;
      }

      OfxStatus Instance::drawAction(OfxTime time,  
                                     const OfxPointD &renderScale)
      {        
        initArgProp(time, renderScale);
        return callEntry(kOfxInteractActionDraw, &_argProperties);
      }

      OfxStatus Instance::penMotionAction(OfxTime time, 
                                          const OfxPointD &renderScale,
                                          const OfxPointD &penPos,
                                          const OfxPointI &penPosViewport,
                                          double  pressure)
      {
        initArgProp(time, renderScale);
        setPenArgProps(penPos, penPosViewport, pressure);
        return callEntry(kOfxInteractActionPenMotion,&_argProperties);
      }

      OfxStatus Instance::penUpAction(OfxTime time, 
                                      const OfxPointD &renderScale,
                                      const OfxPointD &penPos,
                                      const OfxPointI &penPosViewport,
                                      double pressure)
      {
        initArgProp(time, renderScale);
        setPenArgProps(penPos, penPosViewport, pressure);
        return callEntry(kOfxInteractActionPenUp,&_argProperties);
      }

      OfxStatus Instance::penDownAction(OfxTime time, 
                                        const OfxPointD &renderScale,
                                        const OfxPointD &penPos,
                                        const OfxPointI &penPosViewport,
                                        double pressure)
      {
        initArgProp(time, renderScale);
        setPenArgProps(penPos, penPosViewport, pressure);
        return callEntry(kOfxInteractActionPenDown,&_argProperties);
      }

      OfxStatus Instance::keyDownAction(OfxTime time, 
                                        const OfxPointD &renderScale,
                                        int     key,
                                        char*   keyString)
      {
        initArgProp(time, renderScale);
        setKeyArgProps(key, keyString);
        return callEntry(kOfxInteractActionKeyDown,&_argProperties);
      }

      OfxStatus Instance::keyUpAction(OfxTime time, 
                                      const OfxPointD &renderScale,
                                      int     key,
                                      char*   keyString)
      {
        initArgProp(time, renderScale);
        setKeyArgProps(key, keyString);
        return callEntry(kOfxInteractActionKeyUp,&_argProperties);
      }

      OfxStatus Instance::keyRepeatAction(OfxTime time,
                                          const OfxPointD &renderScale,
                                          int     key,
                                          char*   keyString)
      {
        initArgProp(time, renderScale);
        setKeyArgProps(key, keyString);
        return callEntry(kOfxInteractActionKeyRepeat,&_argProperties);
      }
      
      OfxStatus Instance::gainFocusAction(OfxTime time,
                                          const OfxPointD &renderScale)
      {
        initArgProp(time, renderScale);
        return callEntry(kOfxInteractActionGainFocus,&_argProperties);
      }

      OfxStatus Instance::loseFocusAction(OfxTime  time,
                                          const OfxPointD &renderScale)
      {
        initArgProp(time, renderScale);
        return callEntry(kOfxInteractActionLoseFocus,&_argProperties);
      }

      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      // Interact suite functions

      static OfxStatus interactSwapBuffers(OfxInteractHandle handle)
      {
        try {
        Interact::Instance *interactInstance = reinterpret_cast<Interact::Instance*>(handle);
        if(interactInstance)
          return interactInstance->swapBuffers();
        else
          return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatFailed;
        }
      }
      
      static OfxStatus interactRedraw(OfxInteractHandle handle)
      {
        try {
        Interact::Instance *interactInstance = reinterpret_cast<Interact::Instance*>(handle);
        if(interactInstance)
          return interactInstance->redraw();
        else
          return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatFailed;
        }
      }
      
      static OfxStatus interactGetPropertySet(OfxInteractHandle handle, OfxPropertySetHandle *property)
      {
        try {
        Interact::Base *interact = reinterpret_cast<Interact::Base*>(handle);
        if (!property) {
          return kOfxStatErrBadHandle;
        }

        if (interact) {
          *property = interact->getPropHandle();

          return kOfxStatOK;
        }
        *property = NULL;

        return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatFailed;
        }
      }
      
      /// the interact suite
      static const OfxInteractSuiteV1 gSuite = {
        interactSwapBuffers,
        interactRedraw,
        interactGetPropertySet
      };

      /// function to get the sutie
      const void *GetSuite(int version) {
        if(version == 1)
          return (void *) &gSuite;
        return NULL;
      }


    } // Interact

  } // Host

} // OFX
