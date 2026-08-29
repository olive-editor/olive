

#ifndef _ofxsInteract_H_
#define _ofxsInteract_H_
// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

/** @file This file contains core code that wraps OFX 'objects' with C++ classes.

This file only holds code that is visible to a plugin implementation, and so hides much
of the direct OFX objects and any library side only functions.
*/
#include "ofxsParam.h"

#include <list>

/** @brief Nasty macro used to define empty protected copy ctors and assign ops */
#define mDeclareProtectedAssignAndCC(CLASS) \
  CLASS &operator=(const CLASS &) {assert(false); return *this;}      \
  CLASS(const CLASS &) {assert(false); } 

/** @brief The core 'OFX Support' namespace, used by plugin implementations. All code for these are defined in the common support libraries.
*/
namespace OFX {

  /** @brief forward declaration */
  class ImageEffect;

  /// all image effect interacts have these argumens
  struct InteractArgs {
    /// ctor
    InteractArgs(const PropertySet &props);
    double          time;              /**< @brief The current effect time to draw at */
    OfxPointD       renderScale;       /**< @brief The current render scale being applied to any image that would be fetched */
  };

  /** @brief struct to pass arguments into OFX::Interact::draw */
  struct DrawArgs : public InteractArgs {
    DrawArgs(const PropertySet &props);

#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
    OfxPointD       viewportSize;      /**< @brief The openGL viewport size for the instance */
#endif
    OfxPointD       pixelScale;        /**< @brief The current effect time to draw at */
    OfxRGBColourD   backGroundColour;  /**< @brief The current background colour, ignore the A */
  };

  /** @brief POD  to pass arguments into OFX::Interact pen actions */
  struct PenArgs : public InteractArgs {
    PenArgs(const PropertySet &props);

#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
    OfxPointD       viewportSize;      /**< @brief The openGL viewport size for the instance */
#endif
    OfxPointD       pixelScale;        /**< @brief The current effect time to draw at */
    OfxRGBColourD   backGroundColour;  /**< @brief The current background colour, ignore the A */
    OfxPointD       penPosition;       /**< @brief The current pen position */
    OfxPointD       penViewportPosition;/**< @brief The current pen position in viewport coordinates */
    double          penPressure;       /**< @brief The normalised pressure on the pen */
  };

  /** @brief  struct to pass arguments into OFX::Interact key actions 

  Note 
  - some keys cannot be represented as UTF8 strings (eg: the key pad page down key kOfxKey_KP_Page_Up), in which case the key string will be set to "".
  - some UTF8 symbols (generally non-English language ones) cannot be represented by one of the keySymbols, in which case the UTF8 string will be set to some non empty value, but the keySymbol will be set to kOfxKey_Unknown.
  - in no case will keyString be set to "" and keySymbol be set to kOfxKey_Unknown.
  */
  struct KeyArgs : public InteractArgs {
    KeyArgs(const PropertySet &props);

    int             keySymbol;         /**< @brief The key represented as one of the entries in ofxKeySyms.h, see note in OFX::KeyArgs */
    std::string     keyString;         /**< @brief That key as a UTF8 string, see note in OFX::KeyArgs */
  };

  /** @brief struct to pass arguments into OFX::Interact focus actions */
  struct FocusArgs : public InteractArgs {
    FocusArgs(const PropertySet &props);

    OfxPointD       viewportSize;      /**< @brief The openGL viewport size for the instance */
    OfxPointD       pixelScale;        /**< @brief The current effect time to draw at */
    OfxRGBColourD   backGroundColour;  /**< @brief The current background colour, ignore the A */
  };

  /** @brief Wraps up an OFX interact object for an Image Effect. It won't work for any other plug-in type at present (it would need to be broken into a hierarchy of classes).
  */
  class Interact {
  protected :
    OfxInteractHandle  _interactHandle;     /**< @brief The handle for this interact */
    PropertySet        _interactProperties; /**< @brief The property set on this interact */
    std::list<Param *> _slaveParams;        /**< @brief List of params we are currently slaved to */
    ImageEffect *_effect;                   /**< @brief The instance we are associated with */

  public : 
    /** @brief ctor */
    Interact(OfxInteractHandle handle);

    /** @brief virtual destructor */
    virtual ~Interact(); 

    PropertySet &getProperties() { return _interactProperties; }

    /** @brief The bitdepth of each component in the openGL frame buffer */
    int getBitDepth(void) const;

    /** @brief Does the openGL frame buffer have an alpha */
    bool hasAlpha(void) const;

    /** @brief Returns the size of a real screen pixel under the interact's cannonical projection */
    OfxPointD getPixelScale(void) const;

    /** @brief The suggested colour to draw a widget in an interact. Returns false if there is no suggestion. */
    bool getSuggestedColour(OfxRGBColourD &c) const;

    /** @brief the background colour */
    OfxRGBColourD getBackgroundColour(void) const;

    /** @brief Set a param that the interact should be redrawn on if its value changes */
    void addParamToSlaveTo(Param *p);

    /** @brief Remova a param that the interact should be redrawn on if its value changes */
    void removeParamToSlaveTo(Param *p);

    /** @brief Request a redraw */
    void requestRedraw(void) const;

    /** @brief Swap a buffer in the case of a double bufferred interact, this is possibly a silly one */
    void swapBuffers(void) const;

    ////////////////////////////////////////////////////////////////////////////////
    // override the below in derived classes to do something useful

    /** @brief the function called to draw in the interact */
    virtual bool draw(const DrawArgs &args);

    /** @brief the function called to handle pen motion in the interact

    returns true if the interact trapped the action in some sense. This will block the action being passed to 
    any other interact that may share the viewer.
    */
    virtual bool penMotion(const PenArgs &args);

    /** @brief the function called to handle pen down events in the interact 

    returns true if the interact trapped the action in some sense. This will block the action being passed to 
    any other interact that may share the viewer.
    */
    virtual bool penDown(const PenArgs &args);

    /** @brief the function called to handle pen up events in the interact 

    returns true if the interact trapped the action in some sense. This will block the action being passed to 
    any other interact that may share the viewer.
    */
    virtual bool penUp(const PenArgs &args);

    /** @brief the function called to handle key down events in the interact 

    returns true if the interact trapped the action in some sense. This will block the action being passed to 
    any other interact that may share the viewer.
    */
    virtual bool keyDown(const KeyArgs &args);

    /** @brief the function called to handle key up events in the interact 

    returns true if the interact trapped the action in some sense. This will block the action being passed to 
    any other interact that may share the viewer.
    */
    virtual bool keyUp(const KeyArgs &args);

    /** @brief the function called to handle key down repeat events in the interact 

    returns true if the interact trapped the action in some sense. This will block the action being passed to 
    any other interact that may share the viewer.
    */
    virtual bool keyRepeat(const KeyArgs &args);

    /** @brief Called when the interact is given input focus */
    virtual void gainFocus(const FocusArgs &args);

    /** @brief Called when the interact is loses input focus */
    virtual void loseFocus(const FocusArgs &args);
  };

  /** @brief an interact for an image effect overlay */
  class OverlayInteract : public Interact {
  public :
    /** @brief ctor */
    OverlayInteract(OfxInteractHandle handle);

    /** @brief dtor */
    virtual ~OverlayInteract();
  };

  class InteractDescriptor
  {
  public:
    InteractDescriptor():_props(0) {}
    virtual ~InteractDescriptor() {}
    void setPropertySet(OFX::PropertySet* props){ _props = props; }
    virtual Interact* createInstance(OfxInteractHandle handle, ImageEffect *effect) = 0;
    void setHasAlpha();
    bool getHasAlpha() const;
    void setBitDepth();
    int getBitDepth() const;
    virtual OfxPluginEntryPoint* getMainEntry() = 0;
    virtual void describe() {}
  protected:
    OFX::PropertySet* _props;
  };

  typedef InteractDescriptor EffectOverlayDescriptor;

  class ParamInteractDescriptor : public InteractDescriptor
  {
  public:
    ParamInteractDescriptor():InteractDescriptor(){}
    virtual ~ParamInteractDescriptor() {}
    void setInteractSizeAspect(double asp);
    void setInteractMinimumSize(int x, int y);
    void setInteractPreferredSize(int x, int y);
    virtual void setParamName(const std::string& pName) { _paramName = pName; }
  protected:
    std::string _paramName;
  };

  class ParamInteract : public Interact 
  {
  public:
    ParamInteract(OfxInteractHandle handle, ImageEffect* effect);
    virtual ~ParamInteract() {}
    double getInteractSizeAspect() const;
    OfxPointI getInteractMinimumSize() const;
    OfxPointI getInteractPreferredSize() const;
    OfxPointI getInteractSize() const;
  protected:
    ImageEffect* _effect;
  };

  namespace Private
  {
    OfxStatus interactMainEntry(const char		*actionRaw,
      const void		*handleRaw,
      OfxPropertySetHandle	 inArgsRaw,
      OfxPropertySetHandle	 outArgsRaw,
      InteractDescriptor& desc);
  }

  template<class DESC>
  class InteractMainEntry
  {
  protected:
    static OfxStatus overlayInteractMainEntry(const char *action, const void* handle, OfxPropertySetHandle in, OfxPropertySetHandle out)
    {
      static DESC desc;
      return OFX::Private::interactMainEntry(action, handle, in, out, desc);
    }
  };

  template<class DESC, class INSTANCE>
  class DefaultEffectOverlayDescriptor : public EffectOverlayDescriptor, public InteractMainEntry<DESC>
  {
  public:
    Interact* createInstance(OfxInteractHandle handle, ImageEffect *effect) { return new INSTANCE(handle, effect); }
    virtual OfxPluginEntryPoint* getMainEntry() { return InteractMainEntry<DESC>::overlayInteractMainEntry; }
  };

  template<class DESC, class INSTANCE>
  class DefaultParamInteractDescriptor : public ParamInteractDescriptor, public InteractMainEntry<DESC>
  {
  public:
    Interact* createInstance(OfxInteractHandle handle, ImageEffect *effect) { return new INSTANCE(handle, effect, _paramNameStatic); }
    virtual OfxPluginEntryPoint* getMainEntry() { return InteractMainEntry<DESC>::overlayInteractMainEntry; }
    virtual void setParamName(const std::string& pName) { _paramNameStatic = pName; }
  protected:
    static std::string _paramNameStatic;
  };

  template<class DESC, class INSTANCE> std::string OFX::DefaultParamInteractDescriptor<DESC, INSTANCE>::_paramNameStatic;
};


#undef mDeclareProtectedAssignAndCC

#endif

