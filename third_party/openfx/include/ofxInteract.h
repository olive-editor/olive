

#ifndef _ofxInteract_h_
#define _ofxInteract_h_

#include "ofxCore.h"

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxInteract.h
Contains the API for ofx plugin defined GUIs and interaction.
*/

#define kOfxInteractSuite "OfxInteractSuite"


/** @brief Blind declaration of an OFX interactive gui 
*/
typedef struct OfxInteract *OfxInteractHandle;

/**
   \addtogroup PropertiesAll
*/
/*@{*/
/**
   \defgroup PropertiesInteract Interact Property Definitions

These are the list of properties used by the Interact API documented in \ref CustomInteractionPage.
*/
/*@{*/
/** @brief The set of parameters on which a value change will trigger a redraw for an interact.

   - Type - string X N
   - Property Set - interact instance property (read/write)
   - Default - no values set
   - Valid Values - the name of any parameter associated with this interact.

If the interact is representing the state of some set of OFX parameters, then is will
need to be redrawn if any of those parameters' values change. This multi-dimensional property
links such parameters to the interact.

The interact can be slaved to multiple parameters (setting index 0, then index 1 etc...)
 */
#define kOfxInteractPropSlaveToParam "OfxInteractPropSlaveToParam"

/** @brief The size of a real screen pixel under the interact's canonical projection.

   - Type - double X 2
   - Property Set - interact instance and actions (read only)

 */
#define kOfxInteractPropPixelScale "OfxInteractPropPixelScale"



/** @brief The background colour of the application behind an interact instance

    - Type - double X 3
    - Property Set - read only on the interact instance and in argument to the ::kOfxInteractActionDraw action
    - Valid Values - from 0 to 1

The components are in the order red, green then blue.

 */
#define kOfxInteractPropBackgroundColour "OfxInteractPropBackgroundColour"

/** @brief The suggested colour to draw a widget in an interact, typically for overlays.
 
    - Type - double X 3
    - Property Set - read only on the interact instance
    - Default - 1.0
    - Valid Values - greater than or equal to 0.0

Some applications allow the user to specify colours of any overlay via a colour picker, this
property represents the value of that colour. Plugins are at liberty to use this or not when
they draw an overlay.

If a host does not support such a colour, it should return kOfxStatReplyDefault
*/
#define kOfxInteractPropSuggestedColour "OfxInteractPropSuggestedColour"

/** @brief The position of the pen in an interact.

   - Type - double X 2
   - Property Set - read only in argument to the ::kOfxInteractActionPenMotion, ::kOfxInteractActionPenDown and ::kOfxInteractActionPenUp actions

This value passes the postion of the pen into an interact. This is in the interact's canonical coordinates.
 */
#define kOfxInteractPropPenPosition "OfxInteractPropPenPosition"

/** @brief The position of the pen in an interact in viewport coordinates.

   - Type - int X 2
   - Property Set - read only in argument to the ::kOfxInteractActionPenMotion, ::kOfxInteractActionPenDown and ::kOfxInteractActionPenUp actions

This value passes the postion of the pen into an interact. This is in the interact's openGL viewport coordinates, with 0,0 being at the bottom left.
 */
#define kOfxInteractPropPenViewportPosition "OfxInteractPropPenViewportPosition"

/** @brief The pressure of the pen in an interact.

   - Type - double X 1
   - Property Set - read only in argument to the ::kOfxInteractActionPenMotion, ::kOfxInteractActionPenDown and ::kOfxInteractActionPenUp actions
   - Valid Values - from 0 (no pressure) to 1 (maximum pressure)

This is used to indicate the status of the 'pen' in an interact. If a pen has only two states (eg: a mouse button), these should map to 0.0 and 1.0.
 */
#define kOfxInteractPropPenPressure "OfxInteractPropPenPressure"

/** @brief Indicates whether the dits per component in the interact's openGL frame buffer

   - Type - int X 1
   - Property Set - interact instance and descriptor (read only)

 */
#define kOfxInteractPropBitDepth "OfxInteractPropBitDepth"

/** @brief Indicates whether the interact's frame buffer has an alpha component or not

   - Type - int X 1
   - Property Set - interact instance and descriptor (read only)
   - Valid Values - This must be one of
       - 0 indicates no alpha component
       - 1 indicates an alpha component
 */
#define kOfxInteractPropHasAlpha "OfxInteractPropHasAlpha"

/*@}*/
/*@}*/

/**
   \addtogroup ActionsAll
*/
/*@{*/
/**
   \defgroup InteractActions Interact Actions

These are the list of actions passed to an interact's entry point function. For more details on how to deal with actions, see \ref InteractActions.
*/
/*@{*/

/** @brief

 This action is the first action passed to an interact. It is
 where an interact defines how it behaves and the resources it needs to
 function.
 If not trapped, the default action is for the host to carry on as normal
 Note that the handle passed in acts as a descriptor for, rather than an
 instance of the interact.

 @param  handle handle to the interact descriptor, cast to an \ref OfxInteractHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL


 \pre
     - The plugin has been loaded and the effect described.

 @returns
     -  \ref kOfxStatOK the action was trapped and all was well
     -  \ref kOfxStatErrMemory in which case describe may be called again after a memory purge
     -  \ref kOfxStatFailed something was wrong, the host should ignore the interact
     -  \ref kOfxStatErrFatal
 */
#define kOfxActionDescribeInteract kOfxActionDescribe


/** @brief

 This action is the first action passed to an interact instance after its creation.
 It is there to allow a plugin to create any per-instance data structures it may need.

 @param  handle handle to the interact instance, cast to an \ref OfxInteractHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL


 \pre
     - \ref kOfxActionDescribe has been called on this interact

 \post
     -  the instance pointer will be valid until the \ref kOfxActionDestroyInstance
     action is passed to the plug-in with the same instance handle

 @returns
     -  \ref kOfxStatOK the action was trapped and all was well
     -  \ref kOfxStatReplyDefault the action was ignored, but all was well anyway
     -  \ref kOfxStatErrFatal
     -  \ref kOfxStatErrMemory in which case this may be called again after a memory purge
     -  \ref kOfxStatFailed in which case the host should ignore this interact
 */
#define kOfxActionCreateInstanceInteract kOfxActionCreateInstance

/**@brief

 This action is the last passed to an interact's instance before its
 destruction. It is there to allow a plugin to destroy any per-instance
 data structures it may have created.

 @param handle handle to the interact instance, cast to an \ref OfxInteractHandle
 @param inArgs is redundant and is set to NULL
 @param outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionCreateInstance
     has been called on the handle,
     - the instance has not had any of its members destroyed yet

 \post
     -  the instance pointer is no longer valid and any operation on it will be undefined

 @returns
     To some extent, what is returned is moot, a bit like throwing an
     exception in a C++ destructor, so the host should continue destruction
     of the instance regardless
     - \ref kOfxStatOK the action was trapped and all was well
     - \ref kOfxStatReplyDefault the action was ignored as the effect had nothing to do
     - \ref kOfxStatErrFatal
     - \ref kOfxStatFailed something went wrong, but no error code appropriate.
 */
#define kOfxActionDestroyInstanceInteract kOfxActionDestroyInstance

/** @brief

 This action is issued to an interact whenever the host needs the plugin
 to redraw the given interact.

 The interact should either issue OpenGL calls to draw itself, or use DrawSuite calls.

 If this is called via kOfxImageEffectPluginPropOverlayInteractV2, drawing MUST use DrawSuite.
 
 If this is called via kOfxImageEffectPluginPropOverlayInteractV1, drawing SHOULD use OpenGL. Some existing plugins may use DrawSuite via kOfxImageEffectPluginPropOverlayInteractV1 if it's supported by the host, but this is discouraged.  

 Note that the interact may (in the case of custom parameter GUIS) or may
 not (in the case of image effect overlays) be required to swap buffers,
 that is up to the kind of interact.

 @param  handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param  inArgs has the following properties on an image effect plugin
     - \ref kOfxPropEffectInstance a handle to the effect for which the interact has been,
     - \ref kOfxInteractPropPixelScale the scale factor to convert cannonical pixels to screen pixels
     - \ref kOfxInteractPropBackgroundColour the background colour of the application behind the current view
     - \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched

 @param  outArgs is redundant and is set to NULL

\pre
     - \ref kOfxActionCreateInstance has been called on the instance handle
     - the openGL context for this interact has been set
     -  the projection matrix will correspond to the interact's cannonical view

 @returns
     - \ref kOfxStatOK the action was trapped and all was well
     - \ref kOfxStatReplyDefault the action was ignored
     - \ref kOfxStatErrFatal
     - \ref kOfxStatFailed something went wrong, the host should ignore this interact in future
 */
#define kOfxInteractActionDraw "OfxInteractActionDraw"

/** @brief

 This action is issued whenever the pen moves an the interact's has
 focus. It should be issued whether the pen is currently up or down.
 No openGL calls should be issued by the plug-in during this action.


 @param  handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param  inArgs has the following properties on an image effect plugin
     - \ref kOfxPropEffectInstance a handle to the effect for which the interact has been,
     - \ref kOfxInteractPropPixelScale the scale factor to convert cannonical pixels to screen pixels
     - \ref kOfxInteractPropBackgroundColour the background colour of the application behind the current view
     - \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched
     - \ref kOfxInteractPropPenPosition postion of the pen in,
     - \ref kOfxInteractPropPenViewportPosition position of the pen in,
     - \ref kOfxInteractPropPenPressure the pressure of the pen,

 @param  outArgs is redundant and is set to NULL

\pre
     - \ref  kOfxActionCreateInstance
     has been called on the instance handle
     - the current instance handle has had
     \ref kOfxInteractActionGainFocus called on it

\post
     -  if the instance returns \ref kOfxStatOK the host should not
     pass the pen motion to any other interactive object it may own that
     shares the same view.

 @returns
     -  \ref kOfxStatOK the action was trapped and the host should not pass the event to other objects it may own
     -  \ref kOfxStatReplyDefault the action was not trapped and the host can deal with it if it wants
 */
#define kOfxInteractActionPenMotion "OfxInteractActionPenMotion"

/**@brief

 This action is issued when a pen transitions for the 'up' to the 'down'
 state.
 No openGL calls should be issued by the plug-in during this action.


 @param  handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param  inArgs has the following properties on an image effect plugin,
     - \ref kOfxPropEffectInstance a handle to the effect for which the interact has been,
     - \ref kOfxInteractPropPixelScale the scale factor to convert cannonical pixels to screen pixels
     - \ref kOfxInteractPropBackgroundColour the background colour of the application behind the current view
     - \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched
     - \ref kOfxInteractPropPenPosition position of the pen in
     - \ref kOfxInteractPropPenViewportPosition position of the pen in
     - \ref kOfxInteractPropPenPressure the pressure of the pen

 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionCreateInstance
     has been called on the instance handle,
     -  the current instance handle has had
     \ref kOfxInteractActionGainFocus
     called on it

\post
     -  if the instance returns \ref kOfxStatOK, the host should not
     pass the pen motion to any other interactive object it may own that
     shares the same view.

 @returns
     -  \ref kOfxStatOK, the action was trapped and the host should not pass the event to other objects it may own
     -  \ref kOfxStatReplyDefault , the action was not trapped and the host can deal with it if it wants
 */
#define kOfxInteractActionPenDown   "OfxInteractActionPenDown"

/**@brief

 This action is issued when a pen transitions for the 'down' to the 'up'
 state.
 No openGL calls should be issued by the plug-in during this action.

 @param  handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param  inArgs has the following properties on an image effect plugin,
 - \ref kOfxPropEffectInstance a handle to the effect for which the interact has been,
 - \ref kOfxInteractPropPixelScale the scale factor to convert cannonical pixels to screen pixels
 - \ref kOfxInteractPropBackgroundColour the background colour of the application behind the current view
 - \ref kOfxPropTime the effect time at which changed occured
 - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched
 - \ref kOfxInteractPropPenPosition position of the pen in
 - \ref kOfxInteractPropPenViewportPosition position of the pen in
 - \ref kOfxInteractPropPenPressure the pressure of the pen

 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionCreateInstance
     has been called on the instance handle,
     -  the current instance handle has had
     \ref kOfxInteractActionGainFocus called on it

 \post
     -  if the instance returns \ref kOfxStatOK, the host should not
     pass the pen motion to any other interactive object it may own that
     shares the same view.

 @returns
     -  \ref kOfxStatOK, the action was trapped and the host should not pass the event to other objects it may own
     -  \ref kOfxStatReplyDefault , the action was not trapped and the host can deal with it if it wants
 */
#define kOfxInteractActionPenUp     "OfxInteractActionPenUp"

/**@brief

 This action is issued when a key on the keyboard is depressed.
 No openGL calls should be issued by the plug-in during this action.

 @param handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param inArgs has the following properties on an image effect plugin
     -  \ref kOfxPropEffectInstance a handle to the effect for which the interact has been,
     - \ref kOfxPropKeySym single integer value representing the key that was manipulated,
     this may not have a UTF8 representation (eg: a return key)
     -  \ref kOfxPropKeyString UTF8 string representing a character key that was pressed, some
     keys have no UTF8 encoding, in which case this is ""
     -  \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched

 @param  outArgs is redundant and is set to NULL

\pre
     - \ref kOfxActionCreateInstance
     has been called on the instance handle,
     -  the current instance handle has had
     \ref kOfxInteractActionGainFocus called on it

 \post
     -  if the instance returns \ref kOfxStatOK, the host should not
     pass the pen motion to any other interactive object it may own that
     shares the same focus.

 @returns
     - \ref kOfxStatOK , the action was trapped and the host should not pass the event to other objects it may own
     - \ref kOfxStatReplyDefault , the action was not trapped and the host can deal with it if it wants
 */
#define kOfxInteractActionKeyDown   "OfxInteractActionKeyDown"

/**@brief
 This action is issued when a key on the keyboard is released.
 No openGL calls should be issued by the plug-in during this action.

 @param handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param inArgs has the following properties on an image effect plugin
     -  \ref kOfxPropEffectInstance a handle to the effect for which the interact has been,
     - \ref kOfxPropKeySym single integer value representing the key that was manipulated,
     this may not have a UTF8 representation (eg: a return key)
     -  \ref kOfxPropKeyString UTF8 string representing a character key that was pressed, some
     keys have no UTF8 encoding, in which case this is ""
     -  \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched

 @param  outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionCreateInstance
     has been called on the instance handle,
     -  the current instance handle has had
     \ref kOfxInteractActionGainFocus called on it

 \post
     -  if the instance returns \ref kOfxStatOK, the host should not
     pass the pen motion to any other interactive object it may own that
     shares the same focus.

 @returns
     - \ref kOfxStatOK , the action was trapped and the host should not pass the event to other objects it may own
     - \ref kOfxStatReplyDefault , the action was not trapped and the host can deal with it if it wants
 */
#define kOfxInteractActionKeyUp     "OfxInteractActionKeyUp"

/**@brief
 This action is issued when a key on the keyboard is repeated.
 No openGL calls should be issued by the plug-in during this action.

 @param handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param inArgs has the following properties on an image effect plugin
     -  \ref kOfxPropEffectInstance a handle to the effect for which the interact has been,
     - \ref kOfxPropKeySym single integer value representing the key that was manipulated,
     this may not have a UTF8 representation (eg: a return key)
     -  \ref kOfxPropKeyString UTF8 string representing a character key that was pressed, some
     keys have no UTF8 encoding, in which case this is ""
     -  \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched

 @param  outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionCreateInstance
     has been called on the instance handle,
     -  the current instance handle has had
     \ref kOfxInteractActionGainFocus called on it

 \post
     -  if the instance returns \ref kOfxStatOK, the host should not
     pass the pen motion to any other interactive object it may own that
     shares the same focus.

 @returns
     - \ref kOfxStatOK , the action was trapped and the host should not pass the event to other objects it may own
     - \ref kOfxStatReplyDefault , the action was not trapped and the host can deal with it if it wants
 */
#define kOfxInteractActionKeyRepeat     "OfxInteractActionKeyRepeat"

/**@brief
 This action is issued when an interact gains input focus.
 No openGL calls should be issued by the plug-in during this action.

 @param handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param inArgs has the following properties on an image effect plugin
     - \ref kOfxPropEffectInstance a handle to the effect for which the interact is being used on,
     - \ref kOfxInteractPropPixelScale the scale factor to convert cannonical pixels to screen pixels,
     - \ref kOfxInteractPropBackgroundColour the background colour of the application behind the current view
     - \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched

 @param  outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionCreateInstance
 has been called on the instance handle,


 @returns
     - \ref kOfxStatOK  the action was trapped
     - \ref kOfxStatReplyDefault  the action was not trapped
 */
#define kOfxInteractActionGainFocus "OfxInteractActionGainFocus"

/**@brief
 This action is issued when an interact loses input focus.
 No openGL calls should be issued by the plug-in during this action.

 @param handle handle to an interact instance, cast to an \ref OfxInteractHandle
 @param inArgs has the following properties on an image effect plugin
     - \ref kOfxPropEffectInstance a handle to the effect for which the interact is being used on,
     - \ref kOfxInteractPropPixelScale the scale factor to convert cannonical pixels to screen pixels,
     - \ref kOfxInteractPropBackgroundColour the background colour of the application behind the current view
     - \ref kOfxPropTime the effect time at which changed occured
     - \ref kOfxImageEffectPropRenderScale the render scale applied to any image fetched

 @param  outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionCreateInstance
     has been called on the instance handle,


 @returns
     - \ref kOfxStatOK  the action was trapped
     - \ref kOfxStatReplyDefault  the action was not trapped
 */
#define kOfxInteractActionLoseFocus "OfxInteractActionLoseFocus"

/*@}*/
/*@}*/

/** @brief OFX suite that allows an effect to interact with an openGL window so as to provide custom interfaces.

*/
typedef struct OfxInteractSuiteV1 {	
  /** @brief Requests an openGL buffer swap on the interact instance */
  OfxStatus (*interactSwapBuffers)(OfxInteractHandle interactInstance);

  /** @brief Requests a redraw of the interact instance */
  OfxStatus (*interactRedraw)(OfxInteractHandle interactInstance);

  /** @brief Gets the property set handle for this interact handle */
  OfxStatus (*interactGetPropertySet)(OfxInteractHandle interactInstance,
				      OfxPropertySetHandle *property);
} OfxInteractSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
