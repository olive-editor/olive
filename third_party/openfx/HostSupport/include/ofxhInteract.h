


#ifndef OFX_INTERACT_H
#define OFX_INTERACT_H

#include "ofxInteract.h"
#include "ofxOld.h" // old plugins may rely on deprecated properties being present
#include "ofxhPropertySuite.h"

namespace OFX {

  namespace Host {

    namespace Interact {
      
      /// fetch a versioned suite for our interact
      const void *GetSuite(int version);
      
      class Base {
      public:
        virtual ~Base() {
        }

        /// grab a handle on the parameter for passing to the C API
        OfxInteractHandle getHandle() {return (OfxInteractHandle)this;}
        
        /// get the property handle for this instance/descriptor
        virtual OfxPropertySetHandle getPropHandle() = 0;
      };

      /// state the interact can be in
      enum State {
        eUninitialised,
        eDescribed,
        eCreated,
        eFailed
      };
  
      /// Descriptor for an interact. Interacts all share a single description
      class Descriptor : public Base {
      protected:
        Property::Set        _properties;  ///< its props
        State                _state;       ///< how is it feeling today
        OfxPluginEntryPoint *_entryPoint;  ///< the entry point for this overlay

      public:
        /// CTOR
        Descriptor();

        /// dtor
        virtual ~Descriptor();

        /// set the main entry points
        void setEntryPoint(OfxPluginEntryPoint *entryPoint) {_entryPoint = entryPoint;}

        /// call describe on this descriptor, returns true if all went well
        bool describe(int bitDepthPerComponent, bool hasAlpha);

        /// grab a handle on the properties of this parameter for the C api
        OfxPropertySetHandle getPropHandle() {return _properties.getHandle();}

        /// get prop set
        const Property::Set &getProperties() const {return _properties;}

        /// get a non const prop set
        Property::Set &getProperties() {return _properties;}

        /// call the entry point with action and the given args
        OfxStatus callEntry(const char *action, 
                            void *handle,
                            OfxPropertySetHandle inArgs, 
                            OfxPropertySetHandle outArgs);      

        /// what is it's state?
        State getState() const {return _state;}
      };

      /// a generic interact, it doesn't belong to anything in particular
      /// we need to generify this slighty more and remove the renderscale args
      /// into a derived class, as they only belong to image effect plugins
      class Instance : public Base, protected Property::GetHook {
      protected:
        Descriptor   &_descriptor;  ///< who we are
        Property::Set _properties;  ///< its props
        State         _state;       ///< how is it feeling today
        void         *_effectInstance; ///< this is ugly, we need a base class to all plugin instances at some point.
        Property::Set _argProperties;

        /// initialise the argument properties
        void initArgProp(OfxTime time, 
                         const OfxPointD   &renderScale);
                
        /// set pen props in the args
        void setPenArgProps(const OfxPointD &penPos,
                            const OfxPointI &penPosViewport,
                            double  pressure);
        
        /// set key args in the props
        void setKeyArgProps(int     key,
                            char*   keyString);
        
      public:
        Instance(Descriptor &desc, void *effectInstance);

        virtual ~Instance();

        /// what is it's state?
        State getState() const {return _state;}

        /// grab a handle on the properties of this parameter for the C api
        OfxPropertySetHandle getPropHandle() {return _properties.getHandle();}

        /// get prop set
        const Property::Set &getProperties() const {return _properties;}

        /// call the entry point in the descriptor with action and the given args
        virtual OfxStatus callEntry(const char *action,
                                    Property::Set *inArgs);
        
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        /// hooks to kOfxInteractPropViewportSize in the property set
        /// this is actually redundant and is to be deprecated
        virtual void getViewportSize(double &width, double &height) const = 0;
#endif

        // hooks to live kOfxInteractPropPixelScale in the property set
        virtual void getPixelScale(double& xScale, double& yScale) const = 0;

        // hooks to kOfxInteractPropBackgroundColour in the property set
        virtual void getBackgroundColour(double &r, double &g, double &b) const = 0;

        // hooks to kOfxInteractPropSuggestedColour and kOfxPropOverlayColour in the property set
        // return false if there is no color suggestion by the host.
        virtual bool getSuggestedColour(double &r, double &g, double &b) const = 0;

        /// implement
        virtual OfxStatus swapBuffers() = 0;

        /// implement this
        virtual OfxStatus redraw() = 0;

        /// returns the params the interact uses
        virtual void getSlaveToParam(std::vector<std::string>& params) const;

        // do nothing
        virtual int  getDimension(const std::string &name) const;
        
        // don't know what to do
        virtual void reset(const std::string &name);

        /// the gethook virutals for  pixel scale, background colour
        virtual double getDoubleProperty(const std::string &name, int index) const;

        /// for pixel scale and background colour
        virtual void getDoublePropertyN(const std::string &name, double *first, int n) const;

        /// call create instance
        virtual OfxStatus createInstanceAction();
        
        // interact action - kOfxInteractActionDraw 
        // 
        // Params -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        virtual OfxStatus drawAction(OfxTime time, const OfxPointD &renderScale);

        // interact action - kOfxInteractActionPenMotion
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    penX              - the X position
        //    penY              - the Y position
        //    pressure          - the pen pressue 0 to 1
        virtual OfxStatus penMotionAction(OfxTime time, 
                                          const OfxPointD &renderScale,
                                          const OfxPointD &penPos,
                                          const OfxPointI &penPosViewport,
                                          double pressure);

        // interact action - kOfxInteractActionPenUp
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    penX              - the X position
        //    penY              - the Y position
        //    pressure          - the pen pressue 0 to 1
        virtual OfxStatus penUpAction(OfxTime time, 
                                      const OfxPointD &renderScale,
                                      const OfxPointD &penPos,
                                      const OfxPointI &penPosViewport,
                                      double  pressure);

        // interact action - kOfxInteractActionPenDown
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    penX              - the X position
        //    penY              - the Y position
        //    pressure          - the pen pressue 0 to 1
        virtual OfxStatus penDownAction(OfxTime time, 
                                        const OfxPointD &renderScale,
                                        const OfxPointD &penPos,
                                        const OfxPointI &penPosViewport,
                                        double  pressure);

        // interact action - kOfxInteractActionkeyDown
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    key               - the pressed key
        //    keyString         - the pressed key string
        virtual OfxStatus keyDownAction(OfxTime time, 
                                        const OfxPointD &renderScale,
                                        int     key,
                                        char*   keyString);

        // interact action - kOfxInteractActionkeyUp
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    key               - the pressed key
        //    keyString         - the pressed key string
        virtual OfxStatus keyUpAction(OfxTime time, 
                                      const OfxPointD &renderScale,
                                      int     key,
                                      char*   keyString);

        // interact action - kOfxInteractActionkeyRepeat
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    key               - the pressed key
        //    keyString         - the pressed key string
        virtual OfxStatus keyRepeatAction(OfxTime time, 
                                          const OfxPointD &renderScale,
                                          int     key,
                                          char*   keyString);

        // interact action - kOfxInteractActionLoseFocus
        // 
        // Params -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        virtual OfxStatus gainFocusAction(OfxTime time, 
                                          const OfxPointD &renderScale);

        // interact action - kOfxInteractActionLoseFocus
        // 
        // Params -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        virtual OfxStatus loseFocusAction(OfxTime  time, 
                                          const OfxPointD &renderScale);
      };

    } // Interact 

  } // Host

} // OFX

#endif // OFX_INTERACT_H
