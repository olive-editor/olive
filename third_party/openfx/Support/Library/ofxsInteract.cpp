

/** @brief This file contains code that skins the ofx interact suite (for image effects) */


#include "ofxsSupportPrivate.h"
#include <algorithm> // for find

/** @brief The core 'OFX Support' namespace, used by plugin implementations. All code for these are defined in the common support libraries.
*/
namespace OFX {

  /** @brief fetch a pixel scale out of the property set */
  static OfxPointD getPixelScale(const PropertySet &props)
  {
    OfxPointD pixelScale;
    pixelScale.x = props.propGetDouble(kOfxInteractPropPixelScale, 0);
    pixelScale.y = props.propGetDouble(kOfxInteractPropPixelScale, 1);
    return pixelScale;
  }

  /** @brief fetch a render scale out of the property set */
  static OfxPointD getRenderScale(const PropertySet &props)
  {
    OfxPointD v;
    v.x = props.propGetDouble(kOfxImageEffectPropRenderScale, 0);
    v.y = props.propGetDouble(kOfxImageEffectPropRenderScale, 1);
    return v;
  }

  /** @brief fetch a background colour out of the property set */
  static OfxRGBColourD getBackgroundColour(const PropertySet &props)
  {
    OfxRGBColourD backGroundColour;
    backGroundColour.r = props.propGetDouble(kOfxInteractPropBackgroundColour, 0);
    backGroundColour.g = props.propGetDouble(kOfxInteractPropBackgroundColour, 1);
    backGroundColour.b = props.propGetDouble(kOfxInteractPropBackgroundColour, 2);
    return backGroundColour;
  }

  /** @brief retrieves the image effect pointer from the interact handle */
  static ImageEffect *retrieveEffectFromInteractHandle(OfxInteractHandle handle)
  {
    // get the properties set on this handle
    OfxPropertySetHandle propHandle;
    OfxStatus stat = OFX::Private::gInteractSuite->interactGetPropertySet(handle, &propHandle);
    throwSuiteStatusException(stat);
    PropertySet interactProperties(propHandle);

    // get the effect handle from this handle
    OfxImageEffectHandle effectHandle = (OfxImageEffectHandle) interactProperties.propGetPointer(kOfxPropEffectInstance);

    // get the effect properties 
    return OFX::Private::retrieveImageEffectPointer(effectHandle);
  }

  /** @brief ctor */
  Interact::Interact(OfxInteractHandle handle)
    : _interactHandle(handle)
    , _effect(0)
  {
    // get the properties set on this handle
    OfxPropertySetHandle propHandle;
    OfxStatus stat = OFX::Private::gInteractSuite->interactGetPropertySet(handle, &propHandle);
    throwSuiteStatusException(stat);
    _interactProperties.propSetHandle(propHandle);

    // set othe instance data on the property handle to point to this interact
    _interactProperties.propSetPointer(kOfxPropInstanceData, (void *)this);

    // get the effect handle from this handle        
    _effect = retrieveEffectFromInteractHandle(handle);
  }

  /** @brief ctor */
  Interact::~Interact()
  {
  }

  /** @brief The bitdepth of each component in the openGL frame buffer */
  int 
    Interact::getBitDepth(void) const
  {
    return _interactProperties.propGetInt(kOfxInteractPropBitDepth);
  }

  /** @brief Does the openGL frame buffer have an alpha */
  bool
    Interact::hasAlpha(void) const
  {
    return _interactProperties.propGetInt(kOfxInteractPropHasAlpha) != 0;
  }

  /** @brief Returns the size of a real screen pixel under the interact's cannonical projection */
  OfxPointD 
    Interact::getPixelScale(void) const
  {
    OfxPointD v;
    v.x = _interactProperties.propGetDouble(kOfxInteractPropPixelScale, 0);
    v.y = _interactProperties.propGetDouble(kOfxInteractPropPixelScale, 1);
    return v;
  }

  /** @brief The suggested colour to draw a widget in an interact */
  bool
    Interact::getSuggestedColour(OfxRGBColourD &c) const
  {
    // OFX 1.2/1.3 specs say that the host should return kOfxStatReplyDefault if there is no suggested color
    OfxStatus stat = OFX::Private::gPropSuite->propGetDouble(_interactProperties.propSetHandle(), kOfxInteractPropSuggestedColour, 0, &c.r);
    if (stat != kOfxStatOK) {
      return false; // host gave no suggestion (replied kOfxStatReplyDefault or property is unknown to host)
    }
    stat = OFX::Private::gPropSuite->propGetDouble(_interactProperties.propSetHandle(), kOfxInteractPropSuggestedColour, 1, &c.g);
    if (stat != kOfxStatOK) {
      return false; // host gave no suggestion (replied kOfxStatReplyDefault or property is unknown to host)
    }
    stat = OFX::Private::gPropSuite->propGetDouble(_interactProperties.propSetHandle(), kOfxInteractPropSuggestedColour, 2, &c.b);
    if (stat != kOfxStatOK) {
      return false; // host gave no suggestion (replied kOfxStatReplyDefault or property is unknown to host)
    }
    return true;
  }

  /** @brief Request a redraw */
  void 
    Interact::requestRedraw(void) const
  {
    OfxStatus stat = OFX::Private::gInteractSuite->interactRedraw(_interactHandle);
    throwSuiteStatusException(stat);
  }

  /** @brief Swap a buffer in the case of a double bufferred interact, this is possibly a silly one */
  void 
    Interact::swapBuffers(void) const
  {
    OfxStatus stat = OFX::Private::gInteractSuite->interactSwapBuffers(_interactHandle);
    throwSuiteStatusException(stat);
  }

  /** @brief Set a param that the interact should be redrawn on if its value changes */
  void 
    Interact::addParamToSlaveTo(Param *p)
  {
    // do we have it already ?
    std::list<Param *>::iterator i;
    i = std::find(_slaveParams.begin(), _slaveParams.end(), p);
    if(i == _slaveParams.end()) {
      // we have a new one to add in here
      _slaveParams.push_back(p);

      // and set the property
      int n = _interactProperties.propGetDimension(kOfxInteractPropSlaveToParam);
      _interactProperties.propSetString(kOfxInteractPropSlaveToParam, p->getName(), n);
    }

  }

  /** @brief Remova a param that the interact should be redrawn on if its value changes */
  void 
    Interact::removeParamToSlaveTo(Param *p)
  {
    // do we have it already ?
    std::list<Param *>::iterator i;
    i = std::find(_slaveParams.begin(), _slaveParams.end(), p);
    if(i != _slaveParams.end()) {
      // clobber it from the list
      _slaveParams.erase(i);

      // reset the property to remove our dead one
      _interactProperties.propReset(kOfxInteractPropSlaveToParam);

      // and add them all in again
      int n = 0;
      for(i = _slaveParams.begin(); i != _slaveParams.end(); ++i, ++n) {
        _interactProperties.propSetString(kOfxInteractPropSlaveToParam, (*i)->getName(), n);
      }
    }
  }

  /** @brief the background colour */
  OfxRGBColourD Interact::getBackgroundColour(void) const
  {
    return OFX::getBackgroundColour(_interactProperties);
  }

  /** @brief the function called to draw in the interact */
  bool 
    Interact::draw(const DrawArgs &/*args*/)
  {
    return false;
  }

  /** @brief the function called to handle pen motion in the interact

  returns true if the interact trapped the action in some sense. This will block the action being passed to 
  any other interact that may share the viewer.
  */
  bool 
    Interact::penMotion(const PenArgs &/*args*/)
  {
    return false;
  }

  /** @brief the function called to handle pen down events in the interact 

  returns true if the interact trapped the action in some sense. This will block the action being passed to 
  any other interact that may share the viewer.
  */
  bool 
    Interact::penDown(const PenArgs &/*args*/)
  {
    return false;
  }

  /** @brief the function called to handle pen up events in the interact 

  returns true if the interact trapped the action in some sense. This will block the action being passed to 
  any other interact that may share the viewer.
  */
  bool 
    Interact::penUp(const PenArgs &/*args*/)
  {
    return false;
  }

  /** @brief the function called to handle key down events in the interact 

  returns true if the interact trapped the action in some sense. This will block the action being passed to 
  any other interact that may share the viewer.
  */
  bool 
    Interact::keyDown(const KeyArgs &/*args*/)
  {
    return false;
  }

  /** @brief the function called to handle key up events in the interact 

  returns true if the interact trapped the action in some sense. This will block the action being passed to 
  any other interact that may share the viewer.
  */
  bool 
    Interact::keyUp(const KeyArgs &/*args*/)
  {
    return false;
  }

  /** @brief the function called to handle key down repeat events in the interact 

  returns true if the interact trapped the action in some sense. This will block the action being passed to 
  any other interact that may share the viewer.
  */
  bool 
    Interact::keyRepeat(const KeyArgs &/*args*/)
  {
    return false;
  }

  /** @brief Called when the interact is given input focus */
  void 
    Interact::gainFocus(const FocusArgs &/*args*/)
  {
  }

  /** @brief Called when the interact is loses input focus */
  void 
    Interact::loseFocus(const FocusArgs &/*args*/)
  {
  }

  ////////////////////////////////////////////////////////////////////////////////
  // overlay interact guff

  /** @brief ctor */
  OverlayInteract::OverlayInteract(OfxInteractHandle handle)
    : Interact(handle)
  {
    // add this interact into the list of overlays that the effect knows about
    if(_effect)
      _effect->addOverlayInteract(this);
  }

  /** @brief ctor */
  OverlayInteract::~OverlayInteract()
  {
    // add this interact into the list of overlays that the effect knows about
    if(_effect)
      _effect->removeOverlayInteract(this);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /** @brief ctor */
  InteractArgs::InteractArgs(const PropertySet &props)
  {
    time          = props.propGetDouble(kOfxPropTime);
    renderScale   = getRenderScale(props);
  }

  /** @brief ctor */
  DrawArgs::DrawArgs(const PropertySet &props)
    : InteractArgs(props)
  {
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
    viewportSize.x = props.propGetDouble(kOfxInteractPropViewportSize, 0, false);
    viewportSize.y = props.propGetDouble(kOfxInteractPropViewportSize, 1, false);
#endif
    backGroundColour = getBackgroundColour(props);
    pixelScale       = getPixelScale(props);
  }

  /** @brief ctor */
  PenArgs::PenArgs(const PropertySet &props)
    : InteractArgs(props)
  {
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
    viewportSize.x = props.propGetDouble(kOfxInteractPropViewportSize, 0, false);
    viewportSize.y = props.propGetDouble(kOfxInteractPropViewportSize, 1, false);
#endif
    pixelScale    = getPixelScale(props);
    backGroundColour = getBackgroundColour(props);
    penPosition.x = props.propGetDouble(kOfxInteractPropPenPosition, 0);
    penPosition.y = props.propGetDouble(kOfxInteractPropPenPosition, 1);
    try {
      penViewportPosition.x = props.propGetInt(kOfxInteractPropPenViewportPosition, 0);
      penViewportPosition.y = props.propGetInt(kOfxInteractPropPenViewportPosition, 1);
    } catch (OFX::Exception::PropertyUnknownToHost&) {
      // Introduced in OFX 1.2. Return (-1,-1) if not available
      penViewportPosition.x = penViewportPosition.y = -1.;
    }
    penPressure   = props.propGetDouble(kOfxInteractPropPenPressure);
  }

  /** @brief ctor */
  KeyArgs::KeyArgs(const PropertySet &props)
    : InteractArgs(props)
  {
    time          = props.propGetDouble(kOfxPropTime);
    renderScale   = getRenderScale(props);
    keyString     = props.propGetString(kOfxPropKeyString);
    keySymbol     = props.propGetInt(kOfxPropKeySym);
  }

  /** @brief ctor */
  FocusArgs::FocusArgs(const PropertySet &props)
    : InteractArgs(props)
  {
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
    viewportSize.x = props.propGetDouble(kOfxInteractPropViewportSize, 0, false);
    viewportSize.y = props.propGetDouble(kOfxInteractPropViewportSize, 1, false);
#endif
    pixelScale       = getPixelScale(props);
    backGroundColour = getBackgroundColour(props);
  }


  void ParamInteractDescriptor::setInteractSizeAspect(double asp)
  {
    _props->propSetDouble(kOfxParamPropInteractSizeAspect , asp);
  }

  void ParamInteractDescriptor::setInteractMinimumSize(int x, int y)
  {
    _props->propSetInt(kOfxParamPropInteractMinimumSize, x, 0);
    _props->propSetInt(kOfxParamPropInteractMinimumSize, y, 1);
  }

  void ParamInteractDescriptor::setInteractPreferredSize(int x, int y)
  {
    _props->propSetInt(kOfxParamPropInteractPreferedSize, x, 0);
    _props->propSetInt(kOfxParamPropInteractPreferedSize, y, 1);
  }

  ParamInteract::ParamInteract(OfxInteractHandle handle, ImageEffect* effect):Interact(handle), _effect(effect)
  {}

  OfxPointI ParamInteract::getInteractSize() const
  {
    OfxPointI ret;
    ret.x =  _interactProperties.propGetInt(kOfxParamPropInteractSize, 0);
    ret.y =  _interactProperties.propGetInt(kOfxParamPropInteractSize, 1);
    return ret;
  }

  namespace Private {
    /** @brief fetches our pointer out of the props on the handle */
    static
    Interact *retrieveInteractPointer(OfxInteractHandle handle)
    {
      Interact *instance;

      // get the prop set on the handle
      OfxPropertySetHandle propHandle;
      OfxStatus stat = OFX::Private::gInteractSuite->interactGetPropertySet(handle, &propHandle);
      throwSuiteStatusException(stat);

      // make our wrapper object
      PropertySet props(propHandle);

      // fetch the instance data out of the properties
      instance = (Interact *) props.propGetPointer(kOfxPropInstanceData);

      OFX::Log::error(instance == 0, "Instance data handle in effect instance properties is NULL!");

      // need to throw something here

      // and dance to the music
      return instance;
    }

    /** @brief The common entry point used by all interacts */
    static
    OfxStatus
      interactMainEntry(const std::string     &action,
      OfxInteractHandle      handle,
      PropertySet              inArgs,
      PropertySet              /*outArgs*/)
    {
      OfxStatus stat = kOfxStatReplyDefault;

      // get the interact pointer
      Interact *interact = retrieveInteractPointer(handle);

      // if one was not made, return and do nothing
      if(!interact)
        return stat;

      if(action == kOfxActionDestroyInstance) {
        delete interact;
        stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionDraw) {
        // make the draw args
        DrawArgs drawArgs(inArgs);
        if(interact->draw(drawArgs))
          stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionPenMotion) {

        // make the draw args
        PenArgs args(inArgs);
        if(interact->penMotion(args))
          stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionPenDown) {
        // make the draw args
        PenArgs args(inArgs);
        if(interact->penDown(args))
          stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionPenUp) {
        // make the draw args
        PenArgs args(inArgs);
        if(interact->penUp(args))
          stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionKeyDown) {
        // make the draw args
        KeyArgs args(inArgs);
        if(interact->keyDown(args))
          stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionKeyUp) {
        // make the draw args
        KeyArgs args(inArgs);
        if(interact->keyUp(args))
          stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionKeyRepeat) {
        // make the draw args
        KeyArgs args(inArgs);
        if(interact->keyRepeat(args))
          stat = kOfxStatOK;
      }
      else if(action ==   kOfxInteractActionGainFocus) {
        // make the draw args
        FocusArgs args(inArgs);
        interact->gainFocus(args);
      }
      else if(action ==   kOfxInteractActionLoseFocus) {
        // make the draw args
        FocusArgs args(inArgs);
        interact->loseFocus(args);
      }

      return stat;
    }

    /** @brief The main entry for image effect overlays */
    OfxStatus interactMainEntry(const char             *actionRaw,
      const void             *handleRaw,
      OfxPropertySetHandle    inArgsRaw,
      OfxPropertySetHandle    outArgsRaw,
      InteractDescriptor& desc)
    {
      OFX::Log::print("********************************************************************************");
      OFX::Log::print("START overlayInteractMainEntry (%s)", actionRaw);
      OFX::Log::indent();
      OfxStatus stat = kOfxStatReplyDefault;

      try {
        // Cast the raw handle to be an image effect handle, because that is what it is
        OfxInteractHandle handle = (OfxInteractHandle) handleRaw;

        // Turn the arguments into wrapper objects to make our lives easier
        OFX::PropertySet inArgs(inArgsRaw);
        OFX::PropertySet outArgs(outArgsRaw);

        // turn the action into a std::string
        std::string action(actionRaw);

        // figure the actions
        if (action == kOfxActionDescribe) {
          OfxPropertySetHandle propHandle;
          OfxStatus stat = OFX::Private::gInteractSuite->interactGetPropertySet(handle, &propHandle);
          throwSuiteStatusException(stat);
          PropertySet interactProperties(propHandle);
          desc.setPropertySet(&interactProperties);
          desc.describe();
        }
        else if (action == kOfxActionCreateInstance) 
        {
          // fetch the image effect we are being made for out of the interact's property handle
          ImageEffect *effect = retrieveEffectFromInteractHandle(handle);
          OFX::Interact* interact = desc.createInstance(handle, effect);
          (void)interact;
          // and all was well
          stat = kOfxStatOK;
        }
        else {
          stat = interactMainEntry(action, handle, inArgs, outArgs);
        }

      }
      catch(...)
      {
        stat = kOfxStatFailed;
      }

      OFX::Log::outdent();
      OFX::Log::print("STOP overlayInteractMainEntry (%s)", actionRaw);
      return stat;
    }

  }; // end namespace private

}; // end of namespace
