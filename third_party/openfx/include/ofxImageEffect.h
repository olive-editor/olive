

#pragma once
#ifndef _ofxImageEffect_h_
#define _ofxImageEffect_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

#include "ofxCore.h"
#include "ofxParam.h"
#include "ofxInteract.h"
#include "ofxMessage.h"
#include "ofxMemory.h"
#include "ofxMultiThread.h"
#include "ofxInteract.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @brief String used to label OFX Image Effect Plug-ins

    Set the pluginApi member of the OfxPluginHeader inside any OfxImageEffectPluginStruct
    to be this so that the host knows the plugin is an image effect.    
 */
#define kOfxImageEffectPluginApi "OfxImageEffectPluginAPI"

/** @brief The current version of the Image Effect API
 */
#define kOfxImageEffectPluginApiVersion 1

/** @brief Blind declaration of an OFX image effect
*/
typedef struct OfxImageEffectStruct *OfxImageEffectHandle;

/** @brief Blind declaration of an OFX image effect
*/
typedef struct OfxImageClipStruct *OfxImageClipHandle;

/** @brief Blind declaration for an handle to image memory returned by the image memory management routines */
typedef struct OfxImageMemoryStruct *OfxImageMemoryHandle;

/** @brief String to label something with unset components */
#define kOfxImageComponentNone "OfxImageComponentNone"

/** @brief String to label images with RGBA components */
#define kOfxImageComponentRGBA "OfxImageComponentRGBA"

/** @brief String to label images with RGB components */
#define kOfxImageComponentRGB "OfxImageComponentRGB"

/** @brief String to label images with only Alpha components */
#define kOfxImageComponentAlpha "OfxImageComponentAlpha"

/** @brief Use to define the generator image effect context. See \ref ::kOfxImageEffectPropContext
 */
#define kOfxImageEffectContextGenerator "OfxImageEffectContextGenerator"

/** @brief Use to define the filter effect image effect context See \ref ::kOfxImageEffectPropContext */
#define kOfxImageEffectContextFilter "OfxImageEffectContextFilter"

/** @brief Use to define the transition image effect context See \ref ::kOfxImageEffectPropContext */
#define kOfxImageEffectContextTransition "OfxImageEffectContextTransition"

/** @brief Use to define the paint image effect context  See \ref ::kOfxImageEffectPropContext */
#define kOfxImageEffectContextPaint "OfxImageEffectContextPaint"

/** @brief Use to define the general image effect context  See \ref ::kOfxImageEffectPropContext */
#define kOfxImageEffectContextGeneral "OfxImageEffectContextGeneral"

/** @brief Use to define the retimer effect context  See \ref ::kOfxImageEffectPropContext */
#define kOfxImageEffectContextRetimer "OfxImageEffectContextRetimer"

/** @brief Used as a value for ::kOfxPropType on image effect host handles */
#define kOfxTypeImageEffectHost "OfxTypeImageEffectHost"

/** @brief Used as a value for ::kOfxPropType on image effect plugin handles */
#define kOfxTypeImageEffect "OfxTypeImageEffect"

/** @brief Used as a value for ::kOfxPropType on image effect instance handles  */
#define kOfxTypeImageEffectInstance "OfxTypeImageEffectInstance"

/** @brief Used as a value for ::kOfxPropType on image effect clips */
#define kOfxTypeClip "OfxTypeClip"

/** @brief Used as a value for ::kOfxPropType on image effect images */
#define kOfxTypeImage "OfxTypeImage"

/**
   \addtogroup ActionsAll
*/
/*@{*/
/**
   \defgroup ImageEffectActions Image Effect Actions

These are the list of actions passed to an image effect plugin's main function. For more details on how to deal with actions, see \ref ImageEffectActions.
*/
/*@{*/

/** @brief

 The region of definition for an image effect is the rectangular section
 of the 2D image plane that it is capable of filling, given the state of
 its input clips and parameters. This action is used to calculate the RoD
 for a plugin instance at a given frame. For more details on regions of
 definition see \ref ImageEffectArchitectures "Image Effect Architectures"

 Note that hosts that have constant sized imagery need not call this
 action, only hosts that allow image sizes to vary need call this.

 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle

 @param  inArgs has the following properties
     - \ref kOfxPropTime the effect time for which a region of definition is being
     requested
     - \ref kOfxImageEffectPropRenderScale the render scale that should be used in any calculations in this
     action

 @param  outArgs has the following property which the plug-in may set
     - \ref kOfxImageEffectPropRegionOfDefinition  the calculated region of definition, initially set by the host
     to the default RoD (see below), in Canonical Coordinates.


 If the effect did not trap this, it means the host should use the
 default RoD instead, which depends on the context. This is...

 -  generator context - defaults to the project window,
 -  filter and paint contexts - defaults to the RoD of the 'Source' input
 clip at the given time,
 -  transition context - defaults to the union of the RoDs of the
 'SourceFrom' and 'SourceTo' input clips at the given time,
 -  general context - defaults to the union of the RoDs of all the non
 optional input clips and the 'Source' input clip (if it exists and it
 is connected) at the given time, if none exist, then it is the
 project window
 -  retimer context - defaults to the union of the RoD of the 'Source'
 input clip at the frame directly preceding the value of the
 'SourceTime' double parameter and the frame directly after it

@returns
     -  \ref kOfxStatOK  the action was trapped and the RoD was set in the outArgs property set
     -  \ref kOfxStatReplyDefault, the action was not trapped and the host should use the default values
     -  \ref kOfxStatErrMemory, in which case the action may be called again after a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate, plugin to post message
     -  \ref kOfxStatErrFatal

 */
#define kOfxImageEffectActionGetRegionOfDefinition        "OfxImageEffectActionGetRegionOfDefinition"

/** @brief

 This action allows a host to ask an effect, given a region I want to
 render, what region do you need from each of your input clips. In that
 way, depending on the host architecture, a host can fetch the minimal
 amount of the image needed as input. Note that there is a region of
 interest to be set in ``outArgs`` for each input clip that exists on the
 effect. For more details see \ref ImageEffectArchitectures "Image Effect
 Architectures"


 The default RoI is simply the value passed in on the
 \ref kOfxImageEffectPropRegionOfInterest
 ``inArgs`` property set. All the RoIs in the ``outArgs`` property set
 must initialised to this value before the action is called.


 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs has the following properties
     - \ref kOfxPropTime the effect time for which a region of definition is being requested
     - \ref kOfxImageEffectPropRenderScale the render scale that should be used in any calculations in this action
     - \ref kOfxImageEffectPropRegionOfInterest the region to be rendered in the output image, in Canonical Coordinates.

 @param  outArgs has a set of 4 dimensional double properties, one for each of the input clips to the effect.
 The properties are each named ``OfxImageClipPropRoI_`` with the clip name post pended, for example
 ``OfxImageClipPropRoI_Source``. These are initialised to the default RoI.


 @returns
     -  \ref kOfxStatOK, the action was trapped and at least one RoI was set in the outArgs property set
     -  \ref kOfxStatReplyDefault, the action was not trapped and the host should use the default values
     -  \ref kOfxStatErrMemory, in which case the action may be called again after a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate, plugin to post message
     -  \ref kOfxStatErrFatal


 */
#define kOfxImageEffectActionGetRegionsOfInterest         "OfxImageEffectActionGetRegionsOfInterest"

/** @brief
 This action allows a host to ask an effect what range of frames it can
 produce images over. Only effects instantiated in the \ref generalContext "General
 Context" can have this called on them. In all other
 the host is in strict control over the temporal duration of the effect.

 The default is:

 -  the union of all the frame ranges of the non optional input clips,
 -  infinite if there are no non optional input clips.

 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs is redundant and is null
 @param  outArgs has the following property
     - \ref kOfxImageEffectPropFrameRange the frame range an effect can produce images for


 \pre
     -  \ref kOfxActionCreateInstance has been called on the instance
     -  the effect instance has been created in the general effect context

 @returns
     -  \ref kOfxStatOK, the action was trapped and the \ref kOfxImageEffectPropFrameRange was set in the outArgs property set
     -  \ref kOfxStatReplyDefault, the action was not trapped and the host should use the default value
     -  \ref kOfxStatErrMemory, in which case the action may be called again after a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate, plugin to post message
     -  \ref kOfxStatErrFatal



 */
#define kOfxImageEffectActionGetTimeDomain                "OfxImageEffectActionGetTimeDomain"

/** @brief

 This action lets the host ask the effect what frames are needed from
 each input clip to process a given frame. For example a temporal based
 degrainer may need several frames around the frame to render to do its
 work.

 This action need only ever be called if the plugin has set the
 \ref kOfxImageEffectPropTemporalClipAccess
 property on the plugin descriptor to be true. Otherwise the host assumes
 that the only frame needed from the inputs is the current one and this
 action is not called.

 Note that each clip can have it's required frame range specified, and
 that you can specify discontinuous sets of ranges for each clip, for
 example

 \code{.cpp}

     // The effect always needs the initial frame of the source as well as the previous and current frame
     double rangeSource[4];

     // required ranges on the source
     rangeSource[0] = 0; // we always need frame 0 of the source
     rangeSource[1] = 0;
     rangeSource[2] = currentFrame - 1; // we also need the previous and current frame on the source
     rangeSource[3] = currentFrame;

     gPropHost->propSetDoubleN(outArgs, "OfxImageClipPropFrameRange_Source", 4, rangeSource);

 \endcode

     Which sets two discontinuous range of frames from the 'Source' clip
     required as input.


 The default frame range is simply the single frame,
 kOfxPropTime..kOfxPropTime, found on the ``inArgs`` property set. All
 the frame ranges in the ``outArgs`` property set must initialised to
 this value before the action is called.

 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs has the following property
     - \ref kOfxPropTime the effect time for which we need to calculate the frames needed on input
     - \ref outArgs has a set of properties, one for each input clip, named
     ``OfxImageClipPropFrameRange_`` with the name of the clip post-pended.
     For example ``OfxImageClipPropFrameRange_Source``. All these properties
     are multi-dimensional doubles, with the dimension is a multiple of
     two. Each pair of values indicates a continuous range of frames that
     is needed on the given input. They are all initalised to the default value.


 @returns
     -  \ref kOfxStatOK, the action was trapped and at least one frame range in the outArgs property set
     -  \ref kOfxStatReplyDefault, the action was not trapped and the host should use the default values
     -  \ref kOfxStatErrMemory, in which case the action may be called again after a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate, plugin to post message
     -  \ref kOfxStatErrFatal
 */
#define kOfxImageEffectActionGetFramesNeeded              "OfxImageEffectActionGetFramesNeeded"

/** @brief

 This action allows a plugin to dynamically specify its preferences for
 input and output clips. Please see \ref ImageEffectClipPreferences "Image Effect Clip Preferences" for more details on the
 behaviour. Clip preferences are constant for the duration of an effect,
 so this action need only be called once per clip, not once per frame.

 This should be called once after creation of an instance, each time an
 input clip is changed, and whenever a parameter named in the
 \ref kOfxImageEffectPropClipPreferencesSlaveParam
 has its value changed.

 @param handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param inArgs is redundant and is set to NULL
 @param  outArgs has the following properties which the plugin can set
     -  a set of char \* X 1 properties, one for each of the input clips
     currently attached and the output clip, labelled with
     ``OfxImageClipPropComponents_`` post pended with the clip's name.
     This must be set to one of the component types which the host
     supports and the effect stated it can accept on that input

     -  a set of char \* X 1 properties, one for each of the input clips
     currently attached and the output clip, labelled with
     ``OfxImageClipPropDepth_`` post pended with the clip's name. This
     must be set to one of the pixel depths both the host and plugin
     supports

     -  a set of char \* X 1 properties, one for each of the input clips
     currently attached, labelled with
     ``OfxImageClipPropPreferredColourspaces_`` post pended with the clip's
     name. This must be set according to the requirements of the colour
     management style in use.
     
     -  a set of double X 1 properties, one for each of the input clips
     currently attached and the output clip, labelled with
     ``OfxImageClipPropPAR_`` post pended with the clip's name. This is
     the pixel aspect ratio of the input and output clips. This must be
     set to a positive non zero double value,

     - \ref kOfxImageEffectPropFrameRate the frame rate of the output clip, this must be set to a positive non zero double value
     - \ref kOfxImageClipPropFieldOrder the fielding of the output clip
     - \ref kOfxImageEffectPropPreMultiplication the premultiplication of the output clip
     - \ref kOfxImageClipPropContinuousSamples whether the output clip can produce different images at non-frame intervals, defaults to false,
     - \ref kOfxImageEffectFrameVarying whether the output clip can produces different images at
     different times, even if all parameters and inputs are constant,
     defaults to false.

@returns
     -  \ref kOfxStatOK, the action was trapped and at least one of the properties in the outArgs
     was changed from its default value
     -  \ref kOfxStatReplyDefault, the action was not trapped and the host should
     use the default values
     -  \ref kOfxStatErrMemory, in which case the action may be called again after
     a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate,
     plugin to post message
     -  \ref kOfxStatErrFatal
 */
#define kOfxImageEffectActionGetClipPreferences       "OfxImageEffectActionGetClipPreferences"

/** @brief

 Sometimes an effect can pass through an input uprocessed, for example a
 blur effect with a blur size of 0. This action can be called by a host
 before it attempts to render an effect to determine if it can simply
 copy input directly to output without having to call the render action
 on the effect.

 If the effect does not need to process any pixels, it should set the
 value of the \ref kOfxPropName to the clip that the host should us as the
 output instead, and the \ref kOfxPropTime property on ``outArgs`` to be
 the time at which the frame should be fetched from a clip.

 The default action is to call the render action on the effect.


 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs has the following properties
     - \ref kOfxPropTime the time at which to test for identity
     - \ref kOfxImageEffectPropFieldToRender the field to test for identity
     - \ref kOfxImageEffectPropRenderWindow the window (in \\ref PixelCoordinates) to test for identity under
     - \ref kOfxImageEffectPropRenderScale the scale factor being applied to the images being renderred

 @param  outArgs has the following properties which the plugin can set
     - \ref kOfxPropName
     this to the name of the clip that should be used if the effect is
     an identity transform, defaults to the empty string
     - \ref kOfxPropTime
     the time to use from the indicated source clip as an identity
     image (allowing time slips to happen), defaults to the value in
     \ref kOfxPropTime in inArgs



 @returns
     -  \ref kOfxStatOK, the action was trapped and the effect should not have its
     render action called, the values in outArgs
     indicate what frame from which clip to use instead
     -  \ref kOfxStatReplyDefault, the action was not trapped and the host should
     call the render action
     -  \ref kOfxStatErrMemory, in which case the action may be called again after
     a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate,
     plugin to post message
     -  \ref kOfxStatErrFatal

 */
#define kOfxImageEffectActionIsIdentity            "OfxImageEffectActionIsIdentity"

/** @brief

 This action is where an effect gets to push pixels and turn its input
 clips and parameter set into an output image. This is possibly quite
 complicated and covered in the \ref RenderingEffects "Rendering Image Effects" chapter.

 The render action *must* be trapped by the plug-in, it cannot return
 \ref kOfxStatReplyDefault. The pixels needs be pushed I'm afraid.

 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs has the following properties
     -  \ref kOfxPropTime the time at which to render
     -  \ref kOfxImageEffectPropFieldToRender the field to render
     -  \ref kOfxImageEffectPropRenderWindow the window (in \\ref PixelCoordinates) to render
     -  \ref kOfxImageEffectPropRenderScale the scale factor being applied to the images being renderred
     -  \ref kOfxImageEffectPropSequentialRenderStatus whether the effect is currently being rendered in strict frame order on a single instance
     -  \ref kOfxImageEffectPropInteractiveRenderStatus if the render is in response to a user modifying the effect in an interactive session
     -  \ref kOfxImageEffectPropRenderQualityDraft if the render should be done in draft mode (e.g. for faster scrubbing)
     -  \ref kOfxImageEffectPropNoSpatialAwareness if the plugin must render without spatial awareness (e.g. for LUT generation)

 @param  outArgs is redundant and should be set to NULL

\pre
     -  \ref kOfxActionCreateInstance has been called on the instance
     -  \ref kOfxImageEffectActionBeginSequenceRender has been called on the
     instance

 \post
     -  \ref kOfxImageEffectActionEndSequenceRender action will be called on the
     instance

 @returns
     -  \ref kOfxStatOK, the effect rendered happily
     -  \ref kOfxStatErrMemory, in which case the action may be called again after
     a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate,
     plugin to post message
     -  \ref kOfxStatErrFatal

 */
#define kOfxImageEffectActionRender                "OfxImageEffectActionRender"

/** @brief

 This action is passed to an image effect before it renders a range of
 frames. It is there to allow an effect to set things up for a long
 sequence of frames. Note that this is still called, even if only a
 single frame is being rendered in an interactive environment.

 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle

 @param  inArgs has the following properties
     - \ref kOfxImageEffectPropFrameRange the range of frames (inclusive) that will be renderred
     - \ref kOfxImageEffectPropFrameStep what is the step between frames, generally set to 1 (for full frame renders) or 0.5 (for fielded renders)
     - \ref kOfxPropIsInteractive is this a single frame render due to user interaction in a GUI, or a proper full sequence render.
     - \ref kOfxImageEffectPropRenderScale the scale factor to apply to images for this call
     - \ref kOfxImageEffectPropSequentialRenderStatus whether the effect is currently being rendered in strict frame order on a single instance
     - \ref kOfxImageEffectPropInteractiveRenderStatus if the render is in response to a user modifying the effect in an interactive session

 @param  outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionCreateInstance has been called on the instance

 \post
     - \ref  kOfxImageEffectActionRender action will be called at least once on the instance
     - \ref  kOfxImageEffectActionEndSequenceRender action will be called on the
 instance

 @returns
     -  \ref kOfxStatOK, the action was trapped and handled cleanly by the effect,
     -  \ref kOfxStatReplyDefault, the action was not trapped, but all is well
     anyway,
     -  \ref kOfxStatErrMemory, in which case the action may be called again after
     a memory purge,
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate,
     plugin to post message,
     -  \ref kOfxStatErrFatal
 */
#define kOfxImageEffectActionBeginSequenceRender   "OfxImageEffectActionBeginSequenceRender"

/** @brief

 This action is passed to an image effect after is has rendered a range
 of frames. It is there to allow an effect to free resources after a long
 sequence of frame renders. Note that this is still called, even if only
 a single frame is being rendered in an interactive environment.

 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs has the following properties
     - \ref kOfxImageEffectPropFrameRange the range of frames (inclusive) that will be rendered
     - \ref kOfxImageEffectPropFrameStep what is the step between frames, generally set to 1 (for full frame renders) or 0.5 (for fielded renders),
     - \ref kOfxPropIsInteractive
     - \ref is this a single frame render due to user interaction in a GUI, or a proper full sequence render.
     - \ref kOfxImageEffectPropRenderScale
     - \ref the scale factor to apply to images for this call
     - \ref kOfxImageEffectPropSequentialRenderStatus
     - \ref whether the effect is currently being rendered in strict frame order on a single instance
     - \ref kOfxImageEffectPropInteractiveRenderStatus
     - \ref if the render is in response to a user modifying the effect in an interactive session

 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionCreateInstance has been called on the instance
     -  \ref kOfxImageEffectActionEndSequenceRender action was called on the
     instance
     -  \ref kOfxImageEffectActionRender action was called at least once on the
     instance

 @returns
     -  \ref kOfxStatOK, the action was trapped and handled cleanly by the effect,
     -  \ref kOfxStatReplyDefault, the action was not trapped, but all is well
     anyway,
     -  \ref kOfxStatErrMemory, in which case the action may be called again after
     a memory purge,
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate,
     plugin to post message,
     -  \ref kOfxStatErrFatal

 */
#define kOfxImageEffectActionEndSequenceRender      "OfxImageEffectActionEndSequenceRender"

/** @brief

 This action is unique to OFX Image Effect plug-ins. Because a plugin is
 able to exhibit different behaviour depending on the context of use,
 each separate context will need to be described individually. It is
 within this action that image effects describe which parameters and
 input clips it requires.

 This action will be called multiple times, one for each of the contexts
 the plugin says it is capable of implementing. If a host does not
 support a certain context, then it need not call
 \ref kOfxImageEffectActionDescribeInContext for that context.

 This action *must* be trapped, it is not optional.

 @param  handle handle to the context descriptor, cast to an \ref OfxImageEffectHandle
 this may or may not be the same as passed to \ref kOfxActionDescribe

 @param  inArgs has the following property:
     - \ref kOfxImageEffectPropContext the context being described

 @param  outArgs is redundant and is set to NULL

\pre
     - \ref kOfxActionDescribe has been called on the descriptor handle,
     - \ref kOfxActionCreateInstance has not been called

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatErrMissingHostFeature, in which the context will be ignored
     by the host, the plugin may post a message
     -  \ref kOfxStatErrMemory, in which case the action may be called again after
     a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate,
     plugin to post message
     -  \ref kOfxStatErrFatal

 */
#define kOfxImageEffectActionDescribeInContext     "OfxImageEffectActionDescribeInContext"

/*@}*/
/*@}*/

/**
   \addtogroup PropertiesAll
*/
/*@{*/
/**
   \defgroup ImageEffectPropDefines Image Effect Property Definitions 

These are the list of properties used by the Image Effects API.
*/
/*@{*/
/** @brief Indicates to the host the contexts a plugin can be used in.

   - Type - string X N
   - Property Set - image effect descriptor passed to kOfxActionDescribe (read/write)
   - Default - this has no defaults, it must be set
   - Valid Values - This must be one of
      - ::kOfxImageEffectContextGenerator
      - ::kOfxImageEffectContextFilter
      - ::kOfxImageEffectContextTransition
      - ::kOfxImageEffectContextPaint
      - ::kOfxImageEffectContextGeneral
      - ::kOfxImageEffectContextRetimer
*/
#define kOfxImageEffectPropSupportedContexts "OfxImageEffectPropSupportedContexts"

/** @brief The plugin handle passed to the initial 'describe' action.

   - Type - pointer X 1
   - Property Set - plugin instance, (read only)

This value will be the same for all instances of a plugin.
*/
#define kOfxImageEffectPropPluginHandle "OfxImageEffectPropPluginHandle"

/** @brief Indicates if a host is a background render.

   - Type - int X 1
   - Property Set - host descriptor (read only)
   - Valid Values - This must be one of
       - 0 if the host is a foreground host, it may open the effect in an interactive session (or not)
       - 1 if the host is a background 'processing only' host, and the effect will never be opened in an interactive session.
*/
#define kOfxImageEffectHostPropIsBackground "OfxImageEffectHostPropIsBackground"

/** @brief Indicates whether only one instance of a plugin can exist at the same time

   - Type - int X 1
   - Property Set - plugin descriptor (read/write)
   - Default - 0
   - Valid Values - This must be one of
       - 0 - which means multiple instances can exist simultaneously,
       - 1 -  which means only one instance can exist at any one time.

Some plugins, for whatever reason, may only be able to have a single instance in existance at any one time. This plugin property is used to indicate that.
*/
#define kOfxImageEffectPluginPropSingleInstance "OfxImageEffectPluginPropSingleInstance"

/** @brief Indicates how many simultaneous renders the plugin can deal with.

   - Type - string X 1
   - Property Set - plugin descriptor (read/write)
   - Default - ::kOfxImageEffectRenderInstanceSafe
   - Valid Values - This must be one of
      - ::kOfxImageEffectRenderUnsafe - indicating that only a single 'render' call can be made at any time amoung all instances,
      - ::kOfxImageEffectRenderInstanceSafe - indicating that any instance can have a single 'render' call at any one time,
      - ::kOfxImageEffectRenderFullySafe - indicating that any instance of a plugin can have multiple renders running simultaneously
*/
#define kOfxImageEffectPluginRenderThreadSafety "OfxImageEffectPluginRenderThreadSafety"

/** @brief String used to label render threads as un thread safe, see, \ref ::kOfxImageEffectPluginRenderThreadSafety */
#define kOfxImageEffectRenderUnsafe "OfxImageEffectRenderUnsafe"
/** @brief String used to label render threads as instance thread safe, \ref ::kOfxImageEffectPluginRenderThreadSafety */
#define kOfxImageEffectRenderInstanceSafe "OfxImageEffectRenderInstanceSafe"
/** @brief String used to label render threads as fully thread safe, \ref ::kOfxImageEffectPluginRenderThreadSafety */
#define kOfxImageEffectRenderFullySafe "OfxImageEffectRenderFullySafe"

/** @brief Indicates whether a plugin lets the host perform per frame SMP threading

   - Type - int X 1
   - Property Set - plugin descriptor (read/write)
   - Default - 1
   - Valid Values - This must be one of
     - 0 - which means that the plugin will perform any per frame SMP threading
     - 1 - which means the host can call an instance's render function simultaneously at the same frame, but with different windows to render.
*/
#define kOfxImageEffectPluginPropHostFrameThreading "OfxImageEffectPluginPropHostFrameThreading"

/** @brief Indicates whether a host or plugin can support clips of differing component depths going into/out of an effect

   - Type - int X 1
   - Property Set - plugin descriptor (read/write), host descriptor (read only)
   - Default - 0 for a plugin
   - Valid Values - This must be one of
     - 0 - in which case the host or plugin does not support clips of multiple pixel depths,
     - 1 - which means a host or plugin is able to to deal with clips of multiple pixel depths,

If a host indicates that it can support multiple pixels depths, then it will allow the plugin to explicitly set 
the output clip's pixel depth in the ::kOfxImageEffectActionGetClipPreferences action. See \ref ImageEffectClipPreferences.
*/
#define kOfxImageEffectPropSupportsMultipleClipDepths "OfxImageEffectPropMultipleClipDepths"

/** @brief Indicates whether a host or plugin can support clips of differing pixel aspect ratios going into/out of an effect

   - Type - int X 1
   - Property Set - plugin descriptor (read/write), host descriptor (read only)
   - Default - 0 for a plugin
   - Valid Values - This must be one of
     - 0 - in which case the host or plugin does not support clips of multiple pixel aspect ratios
     - 1 - which means a host or plugin is able to to deal with clips of multiple pixel aspect ratios

If a host indicates that it can support multiple  pixel aspect ratios, then it will allow the plugin to explicitly set 
the output clip's aspect ratio in the ::kOfxImageEffectActionGetClipPreferences action. See \ref ImageEffectClipPreferences.
*/
#define kOfxImageEffectPropSupportsMultipleClipPARs "OfxImageEffectPropSupportsMultipleClipPARs"


/** @brief Indicates the set of parameters on which a value change will trigger a change to clip preferences

   - Type - string X N
   - Property Set - plugin descriptor (read/write)
   - Default - none set
   - Valid Values - the name of any described parameter

The plugin uses this to inform the host of the subset of parameters that affect the effect's clip preferences. A value change in any one of these will trigger a call to the clip preferences action.

The plugin can be slaved to multiple parameters (setting index 0, then index 1 etc...)
 */
#define kOfxImageEffectPropClipPreferencesSlaveParam "OfxImageEffectPropClipPreferencesSlaveParam"

/** @brief Indicates whether the host will let a plugin set the frame rate of the output clip.

   - Type - int X 1
   - Property Set - host descriptor (read only)
   - Valid Values - This must be one of
     - 0 - in which case the plugin may not change the frame rate of the output clip,
     - 1 - which means a plugin is able to change the output clip's frame rate in the ::kOfxImageEffectActionGetClipPreferences action.

See \ref ImageEffectClipPreferences.

If a clip can be continously sampled, the frame rate will be set to 0.
*/
#define kOfxImageEffectPropSetableFrameRate "OfxImageEffectPropSetableFrameRate"

/** @brief Indicates whether the host will let a plugin set the fielding of the output clip.

   - Type - int X 1
   - Property Set - host descriptor (read only)
   - Valid Values - This must be one of
     - 0 - in which case the plugin may not change the fielding of the output clip,
     - 1 - which means a plugin is able to change the output clip's fielding in the ::kOfxImageEffectActionGetClipPreferences action.

See \ref ImageEffectClipPreferences.
*/
#define kOfxImageEffectPropSetableFielding "OfxImageEffectPropSetableFielding"

/** @brief Indicates whether a plugin needs sequential rendering, and a host support it

   - Type - int X 1
   - Property Set - plugin descriptor (read/write) or plugin instance (read/write), and host descriptor (read only)
   - Default - 0
   - Valid Values - 
     - 0 - for a plugin, indicates that a plugin does not need to be sequentially rendered to be correct, for a host, indicates that it cannot ever guarantee sequential rendering,
     - 1 - for a plugin, indicates that it needs to be sequentially rendered to be correct, for a host, indicates that it can always support sequential rendering of plugins that are sequentially rendered,
     - 2 - for a plugin, indicates that it is best to render sequentially, but will still produce correct results if not, for a host, indicates that it can sometimes render sequentially, and will have set ::kOfxImageEffectPropSequentialRenderStatus on the relevant actions

Some effects have temporal dependancies, some information from from the rendering of frame N-1 is needed to render frame N correctly. This property is set by an effect to indicate such a situation. Also, some effects are more efficient if they run sequentially, but can still render correct images even if they do not, eg: a complex particle system.

During an interactive session a host may attempt to render a frame out of sequence (for example when the user scrubs the current time), and the effect needs to deal with such a situation as best it can to provide feedback to the user.

However if a host caches output, any frame frame generated in random temporal order needs to be considered invalid and needs to be re-rendered when the host finally performs a first to last render of the output sequence.

In all cases, a host will set the kOfxImageEffectPropSequentialRenderStatus flag to indicate its sequential render status.
*/
#define kOfxImageEffectInstancePropSequentialRender "OfxImageEffectInstancePropSequentialRender"

/** @brief Property on all the render action that indicate the current sequential render status of a host

   - Type - int X 1
   - Property Set - read only property on the inArgs of the following actions...
     - ::kOfxImageEffectActionBeginSequenceRender
     - ::kOfxImageEffectActionRender
     - ::kOfxImageEffectActionEndSequenceRender
   - Valid Values - 
     - 0 - the host is not currently sequentially rendering,
     - 1 - the host is currentely rendering in a way so that it guarantees sequential rendering.

This property is set to indicate whether the effect is currently being rendered in frame order on a single effect instance. See ::kOfxImageEffectInstancePropSequentialRender for more details on sequential rendering.
*/
#define kOfxImageEffectPropSequentialRenderStatus "OfxImageEffectPropSequentialRenderStatus"

#define kOfxHostNativeOriginBottomLeft   "kOfxImageEffectHostPropNativeOriginBottomLeft"  
#define kOfxHostNativeOriginTopLeft      "kOfxImageEffectHostPropNativeOriginTopLeft"  
#define kOfxHostNativeOriginCenter       "kOfxImageEffectHostPropNativeOriginCenter"  
/** @brief Property that indicates the host native UI space - this is only a UI hint, has no impact on pixel processing

   - Type - UTF8 string X 1
   - Property Set - read only property (host)
    - Valid Values - 
     "kOfxImageEffectHostPropNativeOriginBottomLeft"  - 0,0 bottom left
     "kOfxImageEffectHostPropNativeOriginTopLeft" - 0,0 top left
	 "kOfxImageEffectHostPropNativeOriginCenter"  - 0,0 center (screen space)

This property is set to kOfxHostNativeOriginBottomLeft pre V1.4 and was to be discovered by plug-ins. This is useful for drawing overlay for points... so everything matches the rest of the app (for example expression linking to other tools, or simply match the reported location of the host viewer).

*/
#define kOfxImageEffectHostPropNativeOrigin  "OfxImageEffectHostPropNativeOrigin"


/** @brief Property that indicates if a plugin is being rendered in response to user interaction.

   - Type - int X 1
   - Property Set - read only property on the inArgs of the following actions...
     - ::kOfxImageEffectActionBeginSequenceRender
     - ::kOfxImageEffectActionRender
     - ::kOfxImageEffectActionEndSequenceRender
   - Valid Values - 
     - 0 - the host is rendering the instance due to some reason other than an interactive tweak on a UI,
     - 1 - the instance is being rendered because a user is modifying parameters in an interactive session.

This property is set to 1 on all render calls that have been triggered because a user is actively modifying an effect (or up stream effect) in an interactive session. This typically means that the effect is not being rendered as a part of a sequence, but as a single frame.
*/
#define kOfxImageEffectPropInteractiveRenderStatus "OfxImageEffectPropInteractiveRenderStatus"

/** @brief Indicates the effect group for this plugin.

   - Type - UTF8 string X 1
   - Property Set - plugin descriptor (read/write)
   - Default - ""

This is purely a user interface hint for the host so it can group related effects on any menus it may have.
*/
#define kOfxImageEffectPluginPropGrouping "OfxImageEffectPluginPropGrouping"

/** @brief Indicates whether a host support image effect \ref ImageEffectOverlays.

   - Type - int X 1
   - Property Set - host descriptor (read only)
   - Valid Values - This must be one of
       - 0 - the host won't allow a plugin to draw a GUI over the output image,
       - 1 - the host will allow a plugin to draw a GUI over the output image.
*/
#define kOfxImageEffectPropSupportsOverlays "OfxImageEffectPropSupportsOverlays"

/** @brief Sets the entry for an effect's overlay interaction

   - Type - pointer X 1
   - Property Set - plugin descriptor (read/write)
   - Default - NULL
   - Valid Values - must point to an ::OfxPluginEntryPoint

The entry point pointed to must be one that handles custom interaction actions.
*/
#define kOfxImageEffectPluginPropOverlayInteractV1 "OfxImageEffectPluginPropOverlayInteractV1"

/** @brief Sets the entry for an effect's overlay interaction. Unlike
 kOfxImageEffectPluginPropOverlayInteractV1, the overlay interact in the plug-in is expected
 to implement the kOfxInteractActionDraw using the OfxDrawSuiteV1.
 
   - Type - pointer X 1
   - Property Set - plugin descriptor (read/write)
   - Default - NULL
   - Valid Values - must point to an ::OfxPluginEntryPoint

The entry point pointed to must be one that handles custom interaction actions.
*/
#define kOfxImageEffectPluginPropOverlayInteractV2 "OfxImageEffectPluginPropOverlayInteractV2"

/** @brief Indicates whether a plugin or host support multiple resolution images.

   - Type - int X 1
   - Property Set - host descriptor (read only), plugin descriptor (read/write)
   - Default - 1 for plugins
   - Valid Values - This must be one of
       - 0 - the plugin or host does not support multiple resolutions
       - 1 - the plugin or host does support multiple resolutions

Multiple resolution images mean...
       - input and output images can be of any size
       - input and output images can be offset from the origin
*/
#define kOfxImageEffectPropSupportsMultiResolution "OfxImageEffectPropSupportsMultiResolution"

/** @brief Indicates whether a clip, plugin or host supports tiled images

   - Type - int X 1
   - Property Set - host descriptor (read only), plugin descriptor (read/write), clip descriptor (read/write), instance (read/write)
   - Default - to 1 for a plugin and clip
   - Valid Values - This must be one of 0 or 1

Tiled images mean that input or output images can contain pixel data that is only a subset of their full RoD.

If a clip or plugin does not support tiled images, then the host should supply full RoD images to the effect whenever it fetches one.

V1.4:  It is now possible (defined) to change OfxImageEffectPropSupportsTiles in Instance Changed 
*/
#define kOfxImageEffectPropSupportsTiles "OfxImageEffectPropSupportsTiles"


/** @brief Indicates support for random temporal access to images in a clip.

   - Type - int X 1
   - Property Set - host descriptor (read only), plugin descriptor (read/write), clip descriptor (read/write)
   - Default - to 0 for a plugin and clip
   - Valid Values - This must be one of 0 or 1

On a host, it indicates whether the host supports temporal access to images.

On a plugin, indicates if the plugin needs temporal access to images.

On a clip, it indicates that the clip needs temporal access to images.
*/
#define kOfxImageEffectPropTemporalClipAccess "OfxImageEffectPropTemporalClipAccess"

/** @brief Indicates the context a plugin instance has been created for.

   - Type - string X 1
   - Property Set - image effect instance (read only)
   - Valid Values - This must be one of
      - ::kOfxImageEffectContextGenerator
      - ::kOfxImageEffectContextFilter
      - ::kOfxImageEffectContextTransition
      - ::kOfxImageEffectContextPaint
      - ::kOfxImageEffectContextGeneral
      - ::kOfxImageEffectContextRetimer

 */
#define kOfxImageEffectPropContext "OfxImageEffectPropContext"

/** @brief Indicates the type of each component in a clip or image (after any mapping)

   - Type - string X 1
   - Property Set - clip instance (read only), image instance (read only)
   - Valid Values - This must be one of
       - kOfxBitDepthNone (implying a clip is unconnected, not valid for an image)
       - kOfxBitDepthByte
       - kOfxBitDepthShort
       - kOfxBitDepthHalf
       - kOfxBitDepthFloat

Note that for a clip, this is the value set by the clip preferences action, not the raw 'actual' value of the clip.
*/
#define kOfxImageEffectPropPixelDepth "OfxImageEffectPropPixelDepth"

/** @brief Indicates the current component type in a clip or image (after any mapping)

   - Type - string X 1
   - Property Set - clip instance (read only), image instance (read only)
   - Valid Values - This must be one of
     - kOfxImageComponentNone (implying a clip is unconnected, not valid for an image)
     - kOfxImageComponentRGBA
     - kOfxImageComponentRGB
     - kOfxImageComponentAlpha

Note that for a clip, this is the value set by the clip preferences action, not the raw 'actual' value of the clip.
*/
#define kOfxImageEffectPropComponents "OfxImageEffectPropComponents"

/** @brief Uniquely labels an image 

   - Type - ASCII string X 1
   - Property Set - image instance (read only)

This is host set and allows a plug-in to differentiate between images. This is especially
useful if a plugin caches analysed information about the image (for example motion vectors). The plugin can label the
cached information with this identifier. If a user connects a different clip to the analysed input, or the image has changed in some way
then the plugin can detect this via an identifier change and re-evaluate the cached information.
*/
#define kOfxImagePropUniqueIdentifier "OfxImagePropUniqueIdentifier"

/** @brief Clip and action argument property which indicates that the clip can be sampled continously

   - Type - int X 1
   - Property Set -  clip instance (read only), as an out argument to ::kOfxImageEffectActionGetClipPreferences action (read/write)
   - Default - 0 as an out argument to the ::kOfxImageEffectActionGetClipPreferences action
   - Valid Values - This must be one of...
     - 0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
     - 1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).

If this is set to true, then the frame rate of a clip is effectively infinite, so to stop arithmetic
errors the frame rate should then be set to 0.
*/
#define kOfxImageClipPropContinuousSamples "OfxImageClipPropContinuousSamples"

/** @brief  Indicates the type of each component in a clip before any mapping by clip preferences

   - Type - string X 1
   - Property Set - clip instance (read only)
   - Valid Values - This must be one of
       - kOfxBitDepthNone (implying a clip is unconnected image)
       - kOfxBitDepthByte
       - kOfxBitDepthShort
       - kOfxBitDepthHalf
       - kOfxBitDepthFloat

This is the actual value of the component depth, before any mapping by clip preferences.
*/
#define kOfxImageClipPropUnmappedPixelDepth "OfxImageClipPropUnmappedPixelDepth"

/** @brief Indicates the current 'raw' component type on a clip before any mapping by clip preferences

   - Type - string X 1
   - Property Set - clip instance (read only),
   - Valid Values - This must be one of
     - kOfxImageComponentNone (implying a clip is unconnected)
     - kOfxImageComponentRGBA
     - kOfxImageComponentRGB
     - kOfxImageComponentAlpha
*/
#define kOfxImageClipPropUnmappedComponents "OfxImageClipPropUnmappedComponents"

/** @brief Indicates the premultiplication state of a clip or image

   - Type - string X 1
   - Property Set - clip instance (read only), image instance (read only), out args property in the ::kOfxImageEffectActionGetClipPreferences action (read/write)
   - Valid Values - This must be one of
      - kOfxImageOpaque          - the image is opaque and so has no premultiplication state
      - kOfxImagePreMultiplied   - the image is premultiplied by its alpha
      - kOfxImageUnPreMultiplied - the image is unpremultiplied

See the documentation on clip preferences for more details on how this is used with the ::kOfxImageEffectActionGetClipPreferences action.
*/
#define kOfxImageEffectPropPreMultiplication "OfxImageEffectPropPreMultiplication"

/** Used to flag the alpha of an image as opaque */
#define kOfxImageOpaque  "OfxImageOpaque"

/** Used to flag an image as premultiplied */
#define kOfxImagePreMultiplied "OfxImageAlphaPremultiplied"

/** Used to flag an image as unpremultiplied */
#define kOfxImageUnPreMultiplied "OfxImageAlphaUnPremultiplied"


/** @brief Indicates the bit depths support by a plug-in or host
    
   - Type - string X N
   - Property Set - host descriptor (read only), plugin descriptor (read/write)
   - Default - plugin descriptor none set
   - Valid Values - This must be one of
       - kOfxBitDepthNone (implying a clip is unconnected, not valid for an image)
       - kOfxBitDepthByte
       - kOfxBitDepthShort
       - kOfxBitDepthHalf
       - kOfxBitDepthFloat

The default for a plugin is to have none set, the plugin \em must define at least one in its describe action.
*/
#define kOfxImageEffectPropSupportedPixelDepths "OfxImageEffectPropSupportedPixelDepths"

/** @brief Indicates the components supported by a clip or host,

   - Type - string X N
   - Property Set - host descriptor (read only), clip descriptor (read/write)
   - Valid Values - This must be one of
     - kOfxImageComponentNone (implying a clip is unconnected)
     - kOfxImageComponentRGBA
     - kOfxImageComponentRGB
     - kOfxImageComponentAlpha

This list of strings indicate what component types are supported by a host or are expected as input to a clip.

The default for a clip descriptor is to have none set, the plugin \em must define at least one in its define function
*/
#define kOfxImageEffectPropSupportedComponents "OfxImageEffectPropSupportedComponents"

/** @brief Indicates if a clip is optional.

   - Type - int X 1
   - Property Set - clip descriptor (read/write)
   - Default - 0
   - Valid Values - This must be one of 0 or 1

*/
#define kOfxImageClipPropOptional "OfxImageClipPropOptional"

/** @brief Indicates that a clip is intended to be used as a mask input

   - Type - int X 1
   - Property Set - clip descriptor (read/write)
   - Default - 0
   - Valid Values - This must be one of 0 or 1

Set this property on any clip which will only ever have single channel alpha images fetched from it. Typically on an optional clip such as a junk matte in a keyer.

This property acts as a hint to hosts indicating that they could feed the effect from a rotoshape (or similar) rather than an 'ordinary' clip.
*/
#define kOfxImageClipPropIsMask "OfxImageClipPropIsMask"
   
/** @brief The pixel aspect ratio of a clip or image.

   - Type - double X 1
   - Property Set - clip instance (read only), image instance (read only) and ::kOfxImageEffectActionGetClipPreferences action out args property (read/write)

*/
#define kOfxImagePropPixelAspectRatio "OfxImagePropPixelAspectRatio"

/** @brief The frame rate of a clip or instance's project.

   - Type - double X 1
   - Property Set - clip instance (read only), effect instance (read only) and  ::kOfxImageEffectActionGetClipPreferences action out args property (read/write)

For an input clip this is the frame rate of the clip.

For an output clip, the frame rate mapped via pixel preferences.

For an instance, this is the frame rate of the project the effect is in.

For the outargs property in the ::kOfxImageEffectActionGetClipPreferences action, it is used to change the frame rate of the ouput clip.
*/
#define kOfxImageEffectPropFrameRate "OfxImageEffectPropFrameRate"

/** @brief Indicates the original unmapped frame rate (frames/second) of a clip

   - Type - double X 1
   - Property Set - clip instance (read only), 

If a plugin changes the output frame rate in the pixel preferences action, this property allows a plugin to get to the original value.
*/
#define kOfxImageEffectPropUnmappedFrameRate "OfxImageEffectPropUnmappedFrameRate"

/** @brief The frame step used for a sequence of renders

   - Type - double X 1
   - Property Set - an in argument for the ::kOfxImageEffectActionBeginSequenceRender action (read only)
   - Valid Values - can be any positive value, but typically
      - 1 for frame based material
      - 0.5 for field based material
*/
#define kOfxImageEffectPropFrameStep "OfxImageEffectPropFrameStep"

/** @brief The frame range over which a clip has images.

   - Type - double X 2
   - Property Set - clip instance (read only)

Dimension 0 is the first frame for which the clip can produce valid data.

Dimension 1 is the last frame for which the clip can produce valid data.
*/
#define kOfxImageEffectPropFrameRange "OfxImageEffectPropFrameRange"


/** @brief The unmaped frame range over which an output clip has images.

   - Type - double X 2
   - Property Set - clip instance (read only)

Dimension 0 is the first frame for which the clip can produce valid data.

Dimension 1 is the last frame for which the clip can produce valid data.

If a plugin changes the output frame rate in the pixel preferences action, it will affect the frame range
of the output clip, this property allows a plugin to get to the original value.
*/
#define kOfxImageEffectPropUnmappedFrameRange "OfxImageEffectPropUnmappedFrameRange"

/** @brief Says whether the clip is actually connected at the moment.

   - Type - int X 1
   - Property Set - clip instance (read only)
   - Valid Values - This must be one of 0 or 1

An instance may have a clip may not be connected to an object that can produce image data. Use this to find out.

Any clip that is not optional will \em always be connected during a render action. However, during interface actions, even non optional clips may be unconnected.
 */
#define kOfxImageClipPropConnected "OfxImageClipPropConnected"

/** @brief Indicates whether an effect will generate different images from frame to frame.

   - Type - int X 1
   - Property Set - out argument to ::kOfxImageEffectActionGetClipPreferences action (read/write).
   - Default - 0
   - Valid Values - This must be one of 0 or 1

This property indicates whether a plugin will generate a different image from frame to frame, even if no parameters
or input image changes. For example a generater that creates random noise pixel at each frame.
 */
#define kOfxImageEffectFrameVarying "OfxImageEffectFrameVarying"

/** @brief The proxy render scale currently being applied.

    - Type - double X 2
    - Property Set - an image instance (read only) and as read only an in argument on the following actions,
        - ::kOfxImageEffectActionRender
        - ::kOfxImageEffectActionBeginSequenceRender
        - ::kOfxImageEffectActionEndSequenceRender
        - ::kOfxImageEffectActionIsIdentity
        - ::kOfxImageEffectActionGetRegionOfDefinition
        - ::kOfxImageEffectActionGetRegionsOfInterest
        - ::kOfxActionInstanceChanged
        - ::kOfxInteractActionDraw
        - ::kOfxInteractActionPenMotion
        - ::kOfxInteractActionPenDown
        - ::kOfxInteractActionPenUp
        - ::kOfxInteractActionKeyDown
        - ::kOfxInteractActionKeyUp
        - ::kOfxInteractActionKeyRepeat
        - ::kOfxInteractActionGainFocus
        - ::kOfxInteractActionLoseFocus

This should be applied to any spatial parameters to position them correctly. Not that the 'x' value does not include any pixel aspect ratios.
*/
#define kOfxImageEffectPropRenderScale "OfxImageEffectPropRenderScale"

/** @brief Indicates whether an effect can take quality shortcuts to improve speed.

   - Type - int X 1
   - Property Set - render calls, host (read-only)
   - Default - 0  - 0: Best Quality (1: Draft)
   - Valid Values - This must be one of 0 or 1

This property indicates that the host provides the plug-in the option to render in Draft/Preview mode. This is useful for applications that must support fast scrubbing. These allow a plug-in to take short-cuts for improved performance when the situation allows and it makes sense, for example to generate thumbnails with effects applied. 
For example switch to a cheaper interpolation type or rendering mode. A plugin should expect frames rendered in this manner that will not be stucked in host cache unless the cache is only used in the same draft situations.
If an host does not support that property a value of 0 is assumed.
Also note that some hosts do implement kOfxImageEffectPropRenderScale - these two properties can be used independently. 
 */
#define kOfxImageEffectPropRenderQualityDraft "OfxImageEffectPropRenderQualityDraft"

/** @brief Indicates that the plugin can render without spatial awareness, either inherently or by
disabling certain parameters at render time. 

If the plugin descriptor has this property set to "true", the plugin is expected to disable spatial effects when the host sets this property to "true" in the arguments passed to kOfxImageEffectActionBeginSequenceRender and kOfxImageEffectActionRender.

    - Type - string X 1
    - Property Set - plugin descriptor (read/write), render calls - host (read-only)
    - Default - "false"
    - Valid Values - This must be one of
      - "false"  - the plugin cannot render without spatial awareness and the host should bypass it for renders that require no spatial awareness.
      - "true"   - the plugin can render without spatial awareness. The host will indicate this type of render by setting kOfxImageEffectPropNoSpatialAwareness to "true" in the arguments passed to kOfxImageEffectActionBeginSequenceRender and kOfxImageEffectActionRender.
 */
#define kOfxImageEffectPropNoSpatialAwareness "OfxImageEffectPropNoSpatialAwareness"

/** @brief The extent of the current project in canonical coordinates.

    - Type - double X 2
    - Property Set - a plugin  instance (read only)

The extent is the size of the 'output' for the current project. See \ref NormalisedCoordinateSystem for more infomation on the project extent.

The extent is in canonical coordinates and only returns the top right position, as the extent is always rooted at 0,0.

For example a PAL SD project would have an extent of 768, 576.
 */
#define kOfxImageEffectPropProjectExtent "OfxImageEffectPropProjectExtent"

/** @brief The size of the current project in canonical coordinates.

    - Type - double X 2
    - Property Set - a plugin  instance (read only)

The size of a project is a sub set of the ::kOfxImageEffectPropProjectExtent. For example a project may be a PAL SD project, but only be a letter-box within that. The project size is the size of this sub window.

The project size is in canonical coordinates.

See \ref NormalisedCoordinateSystem for more infomation on the project extent.
 */
#define kOfxImageEffectPropProjectSize "OfxImageEffectPropProjectSize"

/** @brief The offset of the current project in canonical coordinates.

    - Type - double X 2
    - Property Set - a plugin  instance (read only)

The offset is related to the ::kOfxImageEffectPropProjectSize and is the offset from the origin of the project 'subwindow'. 

For example for a PAL SD project that is in letterbox form, the project offset is the offset to the bottom left hand corner of the letter box.
 
The project offset is in canonical coordinates.

See \ref NormalisedCoordinateSystem for more infomation on the project extent.
*/
#define kOfxImageEffectPropProjectOffset "OfxImageEffectPropProjectOffset"

/** @brief The pixel aspect ratio of the current project

    - Type - double X 1
    - Property Set - a plugin  instance (read only)

 */
#define kOfxImageEffectPropProjectPixelAspectRatio "OfxImageEffectPropPixelAspectRatio"

/**  @brief The duration of the effect

    - Type - double X 1
    - Property Set - a plugin  instance (read only)

This contains the duration of the plug-in effect, in frames.
 */
#define kOfxImageEffectInstancePropEffectDuration "OfxImageEffectInstancePropEffectDuration"

/** @brief Which spatial field occurs temporally first in a frame.

    - Type - string X 1
    - Property Set - a clip  instance (read only)
    - Valid Values - This must be one of
      - ::kOfxImageFieldNone  - the material is unfielded
      - ::kOfxImageFieldLower - the material is fielded, with image rows 0,2,4.... occuring first in a frame
      - ::kOfxImageFieldUpper - the material is fielded, with image rows line 1,3,5.... occuring first in a frame
 */
#define kOfxImageClipPropFieldOrder "OfxImageClipPropFieldOrder"

/**  @brief The pixel data pointer of an image.

    - Type - pointer X 1
    - Property Set - an image instance (read only)

This property contains one of:
  - a pointer to memory that is the lower left hand corner of an image
  - a pointer to CUDA memory, if the Render action arguments includes kOfxImageEffectPropCudaEnabled=1
  - an id<MTLBuffer>, if the Render action arguments includes kOfxImageEffectPropMetalEnabled=1
  - a cl_mem, if the Render action arguments includes kOfxImageEffectPropOpenCLEnabled=1

See \ref kOfxImageEffectPropCudaEnabled, \ref kOfxImageEffectPropMetalEnabled and \ref kOfxImageEffectPropOpenCLEnabled
*/
#define kOfxImagePropData "OfxImagePropData"

/** @brief The bounds of an image's pixels.

    - Type - integer X 4
    - Property Set - an image  instance (read only)

The bounds, in \ref PixelCoordinates, are of the addressable pixels in an image's data pointer.

The order of the values is x1, y1, x2, y2.

X values are x1 <= X < x2 
Y values are y1 <= Y < y2

For less than full frame images, the pixel bounds will be contained by the ::kOfxImagePropRegionOfDefinition bounds.
 */
#define kOfxImagePropBounds "OfxImagePropBounds"

/** @brief The full region of definition of an image.

    - Type - integer X 4
    - Property Set - an image  instance (read only)

An image's region of definition, in \ref PixelCoordinates, is the full frame area of the image plane that the image covers.

The order of the values is x1, y1, x2, y2.

X values are x1 <= X < x2 
Y values are y1 <= Y < y2

The ::kOfxImagePropBounds property contains the actuall addressable pixels in an image, which may be less than its full region of definition.
 */
#define kOfxImagePropRegionOfDefinition "OfxImagePropRegionOfDefinition"

/** @brief The number of bytes in a row of an image.

    - Type - integer X 1
    - Property Set - an image  instance (read only)

For various alignment reasons, a row of pixels may need to be padded at the end with several bytes before the next row starts in memory. 

This property indicates the number of bytes in a row of pixels. This will be at least sizeof(PIXEL) * (bounds.x2-bounds.x1). Where bounds
is fetched from the ::kOfxImagePropBounds property.

Note that (for CPU images only, not CUDA/Metal/OpenCL Buffers, nor OpenGL textures accessed via the OpenGL Render Suite) row bytes can be negative, which allows hosts with a native top down row order to pass image into OFX without having to repack pixels.
Row bytes is not supported for OpenCL Images.
 */
#define kOfxImagePropRowBytes "OfxImagePropRowBytes"


/** @brief Which fields are present in the image

    - Type - string X 1
    - Property Set - an image instance (read only)
    - Valid Values - This must be one of
      - ::kOfxImageFieldNone  - the image is an unfielded frame
      - ::kOfxImageFieldBoth  - the image is fielded and contains both interlaced fields 
      - ::kOfxImageFieldLower - the image is fielded and contains a single field, being the lower field (rows 0,2,4...)
      - ::kOfxImageFieldUpper - the image is fielded and contains a single field, being the upper field (rows 1,3,5...)

 */
#define kOfxImagePropField "OfxImagePropField"

/** @brief Controls how a plugin renders fielded footage.

    - Type - integer X 1
    - Property Set - a plugin descriptor (read/write)
    - Default - 1
    - Valid Values - This must be one of
       - 0 - the plugin is to have its render function called twice, only if there is animation in any of its parameters
       - 1 - the plugin is to have its render function called twice always
*/
#define kOfxImageEffectPluginPropFieldRenderTwiceAlways "OfxImageEffectPluginPropFieldRenderTwiceAlways"

/** @brief Controls how a plugin fetched fielded imagery from a clip.

    - Type - string X 1
    - Property Set - a clip descriptor (read/write)
    - Default - kOfxImageFieldDoubled
    - Valid Values - This must be one of
       - kOfxImageFieldBoth    - fetch a full frame interlaced image
       - kOfxImageFieldSingle  - fetch a single field, making a half height image
       - kOfxImageFieldDoubled - fetch a single field, but doubling each line and so making a full height image 

This controls how a plug-in wishes to fetch images from a fielded clip, so it can tune it behaviour when it renders fielded footage.

Note that if it fetches kOfxImageFieldSingle and the host stores images natively as both fields interlaced, it can return a single image by doubling rowbytes and tweaking the starting address of the image data. This saves on a buffer copy.
 */
#define kOfxImageClipPropFieldExtraction "OfxImageClipPropFieldExtraction"

/** @brief Indicates which field is being rendered.

    - Type - string X 1
    - Property Set - a read only in argument property to ::kOfxImageEffectActionRender and ::kOfxImageEffectActionIsIdentity
    - Valid Values - this must be one of
      - kOfxImageFieldNone  - there are no fields to deal with, all images are full frame
      - kOfxImageFieldBoth  - the imagery is fielded and both scan lines should be renderred
      - kOfxImageFieldLower - the lower field is being rendered (lines 0,2,4...)
      - kOfxImageFieldUpper - the upper field is being rendered (lines 1,3,5...)
 */
#define kOfxImageEffectPropFieldToRender "OfxImageEffectPropFieldToRender"

/** @brief Used to indicate the region of definition of a plug-in

    - Type - double X 4
    - Property Set - a read/write out argument property to the ::kOfxImageEffectActionGetRegionOfDefinition action
    - Default - see ::kOfxImageEffectActionGetRegionOfDefinition

The order of the values is x1, y1, x2, y2.

This will be in \ref CanonicalCoordinates
 */
#define kOfxImageEffectPropRegionOfDefinition "OfxImageEffectPropRegionOfDefinition"

/** @brief The value of a region of interest.

    - Type - double X 4
    - Property Set - a read only in argument property to the ::kOfxImageEffectActionGetRegionsOfInterest action

A host passes this value into the region of interest action to specify the region it is interested in rendering.

The order of the values is x1, y1, x2, y2.

This will be in \ref CanonicalCoordinates.
 */
#define kOfxImageEffectPropRegionOfInterest "OfxImageEffectPropRegionOfInterest"

/**  @brief The region to be rendered.

    - Type - integer X 4
    - Property Set - a read only in argument property to the ::kOfxImageEffectActionRender and ::kOfxImageEffectActionIsIdentity actions

The order of the values is x1, y1, x2, y2.

This will be in \ref PixelCoordinates

 */
#define kOfxImageEffectPropRenderWindow "OfxImageEffectPropRenderWindow"

/** String used to label imagery as having no fields */
#define kOfxImageFieldNone "OfxFieldNone"
/** String used to label the lower field (scan lines 0,2,4...) of fielded imagery */
#define kOfxImageFieldLower "OfxFieldLower"
/** String used to label the upper field (scan lines 1,3,5...) of fielded imagery */
#define kOfxImageFieldUpper "OfxFieldUpper"
/** String used to label both fields of fielded imagery, indicating interlaced footage */
#define kOfxImageFieldBoth "OfxFieldBoth"
/** String used to label an image that consists of a single field, and so is half height */
#define kOfxImageFieldSingle "OfxFieldSingle"
/** String used to label an image that consists of a single field, but each scan line is double,
    and so is full height */
#define kOfxImageFieldDoubled "OfxFieldDoubled"

/*@}*/
/*@}*/

/** @brief String that is the name of the standard OFX output clip */
#define kOfxImageEffectOutputClipName "Output"

/** @brief String that is the name of the standard OFX single source input clip */
#define kOfxImageEffectSimpleSourceClipName "Source"

/** @brief String that is the name of the 'from' clip in the OFX transition context */
#define kOfxImageEffectTransitionSourceFromClipName "SourceFrom"

/** @brief String that is the name of the 'from' clip in the OFX transition context */
#define kOfxImageEffectTransitionSourceToClipName "SourceTo"

/** @brief the name of the mandated 'Transition' param for the transition context */
#define kOfxImageEffectTransitionParamName "Transition"

/** @brief the name of the mandated 'SourceTime' param for the retime context */
#define kOfxImageEffectRetimerParamName "SourceTime"




/** @brief the string that names image effect suites, passed to OfxHost::fetchSuite */
#define kOfxImageEffectSuite "OfxImageEffectSuite"

/** @brief The OFX suite for image effects

This suite provides the functions needed by a plugin to defined and use an image effect plugin.
 */
typedef struct OfxImageEffectSuiteV1 {  
  /** @brief Retrieves the property set for the given image effect

  \arg \c imageEffect   image effect to get the property set for
  \arg \c propHandle    pointer to a the property set pointer, value is returned here

  The property handle is for the duration of the image effect handle.

  @returns
  - ::kOfxStatOK       - the property set was found and returned
  - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
  - ::kOfxStatErrUnknown    - if the type is unknown
  */
  OfxStatus (*getPropertySet)(OfxImageEffectHandle imageEffect,
			      OfxPropertySetHandle *propHandle);

  /** @brief Retrieves the parameter set for the given image effect

  \arg \c imageEffect   image effect to get the property set for
  \arg \c paramSet     pointer to a the parameter set, value is returned here

  The param set handle is valid for the lifetime of the image effect handle.

  @returns
  - ::kOfxStatOK       - the property set was found and returned
  - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
  - ::kOfxStatErrUnknown    - if the type is unknown
  */
  OfxStatus (*getParamSet)(OfxImageEffectHandle imageEffect,
			   OfxParamSetHandle *paramSet);


  /** @brief Define a clip to the effect. 
      
   \arg \c pluginHandle handle passed into 'describeInContext' action
   \arg \c name unique name of the clip to define
   \arg \c propertySet property handle for the clip descriptor will be returned here

   This function defines a clip to a host, the returned property set is used to describe
   various aspects of the clip to the host. Note that this does not create a clip instance.
   
\pre
 - we are inside the describe in context action.

  @returns
  */
  OfxStatus (*clipDefine)(OfxImageEffectHandle imageEffect,
			  const char *name,	 
			  OfxPropertySetHandle *propertySet);

  /** @brief Get the propery handle of the named input clip in the given instance 
   
   \arg \c imageEffect an instance handle to the plugin
   \arg \c name        name of the clip, previously used in a clip define call
   \arg \c clip        where to return the clip
  \arg \c propertySet  if not NULL, the descriptor handle for a parameter's property set will be placed here.

  The propertySet will have the same value as would be returned by OfxImageEffectSuiteV1::clipGetPropertySet

      This return a clip handle for the given instance, note that this will \em not be the same as the
      clip handle returned by clipDefine and will be distanct to clip handles in any other instance
      of the plugin.

      Not a valid call in any of the describe actions.

\pre
 - create instance action called,
 - \e name passed to clipDefine for this context,
 - not inside describe or describe in context actions.
 
\post
 - handle will be valid for the life time of the instance.

  */
  OfxStatus (*clipGetHandle)(OfxImageEffectHandle imageEffect,
			     const char *name,
			     OfxImageClipHandle *clip,
			     OfxPropertySetHandle *propertySet);

  /** @brief Retrieves the property set for a given clip

  \arg \c clip         clip effect to get the property set for
  \arg \c propHandle   pointer to a the property set handle, value is returedn her

  The property handle is valid for the lifetime of the clip, which is generally the lifetime of the instance.

  @returns
  - ::kOfxStatOK       - the property set was found and returned
  - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
  - ::kOfxStatErrUnknown    - if the type is unknown
  */
  OfxStatus (*clipGetPropertySet)(OfxImageClipHandle clip,
				  OfxPropertySetHandle *propHandle);

  /** @brief Get a handle for an image in a clip at the indicated time and indicated region

      \arg \c clip  clip to extract the image from
      \arg \c time        time to fetch the image at
      \arg \c region      region to fetch the image from (optional, set to NULL to get a 'default' region)
                            this is in the \ref CanonicalCoordinates. 
      \arg \c imageHandle property set containing the image's data

  An image is fetched from a clip at the indicated time for the given region and returned in the imageHandle.

 If the \e region parameter is not set to NULL, then it will be clipped to the clip's Region of Definition for the given time. The returned image will be \em at \em least as big as this region. If the region parameter is not set, then the region fetched will be at least the Region of Interest the effect has previously specified, clipped the clip's Region of Definition.

If clipGetImage is called twice with the same parameters, then two separate image handles will be returned, each of which must be release. The underlying implementation could share image data pointers and use reference counting to maintain them.

\pre
 - clip was returned by clipGetHandle

\post
 - image handle is only valid for the duration of the action clipGetImage is called in
 - image handle to be disposed of by clipReleaseImage before the action returns

@returns
- ::kOfxStatOK - the image was successfully fetched and returned in the handle,
- ::kOfxStatFailed - the image could not be fetched because it does not exist in the clip at the indicated time and/or region, the plugin
                     should continue operation, but assume the image was black and transparent.
- ::kOfxStatErrBadHandle - the clip handle was invalid,
- ::kOfxStatErrMemory - the host had not enough memory to complete the operation, plugin should abort whatever it was doing.

  */
  OfxStatus (*clipGetImage)(OfxImageClipHandle clip,
			    OfxTime       time,
			    const OfxRectD     *region,
			    OfxPropertySetHandle   *imageHandle);
  
  /** @brief Releases the image handle previously returned by clipGetImage


\pre
 - imageHandle was returned by clipGetImage

\post
 - all operations on imageHandle will be invalid

@returns
- ::kOfxStatOK - the image was successfully fetched and returned in the handle,
- ::kOfxStatErrBadHandle - the image handle was invalid,
 */
  OfxStatus (*clipReleaseImage)(OfxPropertySetHandle imageHandle);
  

  /** @brief Returns the spatial region of definition of the clip at the given time

      \arg \c clipHandle  clip to extract the image from
      \arg \c time        time to fetch the image at
      \arg \c region      region to fetch the image from (optional, set to NULL to get a 'default' region)
                            this is in the \ref CanonicalCoordinates. 
      \arg \c imageHandle handle where the image is returned

  An image is fetched from a clip at the indicated time for the given region and returned in the imageHandle.

 If the \e region parameter is not set to NULL, then it will be clipped to the clip's Region of Definition for the given time. The returned image will be \em at \em least as big as this region. If the region parameter is not set, then the region fetched will be at least the Region of Interest the effect has previously specified, clipped the clip's Region of Definition.

\pre
 - clipHandle was returned by clipGetHandle

\post
 - bounds will be filled the RoD of the clip at the indicated time

@returns
- ::kOfxStatOK - the image was successfully fetched and returned in the handle,
- ::kOfxStatFailed - the image could not be fetched because it does not exist in the clip at the indicated time, the plugin
                     should continue operation, but assume the image was black and transparent.
- ::kOfxStatErrBadHandle - the clip handle was invalid,
- ::kOfxStatErrMemory - the host had not enough memory to complete the operation, plugin should abort whatever it was doing.


  */
  OfxStatus (*clipGetRegionOfDefinition)(OfxImageClipHandle clip,
					 OfxTime time,
					 OfxRectD *bounds);

  /** @brief Returns whether to abort processing or not.

      \arg \c imageEffect  instance of the image effect

  A host may want to signal to a plugin that it should stop whatever rendering it is doing and start again. 
  Generally this is done in interactive threads in response to users tweaking some parameter.

  This function indicates whether a plugin should stop whatever processing it is doing.
  
  @returns
     - 0 if the effect should continue whatever processing it is doing
     - 1 if the effect should abort whatever processing it is doing  
 */
  int (*abort)(OfxImageEffectHandle imageEffect);

  /** @brief Allocate memory from the host's image memory pool
      
  \arg \c instanceHandle  effect instance to associate with this memory allocation, may be NULL.
  \arg \c nBytes          number of bytes to allocate
  \arg \c memoryHandle    pointer to the memory handle where a return value is placed

  Memory handles allocated by this should be freed by OfxImageEffectSuiteV1::imageMemoryFree. 
  To access the memory behind the handle you need to call  OfxImageEffectSuiteV1::imageMemoryLock.

  See \ref ImageEffectsMemoryAllocation.

  @returns 
  - kOfxStatOK if all went well, a valid memory handle is placed in \e memoryHandle
  - kOfxStatErrBadHandle if instanceHandle is not valid, memoryHandle is set to NULL
  - kOfxStatErrMemory if there was not enough memory to satisfy the call, memoryHandle is set to NULL
  */   
  OfxStatus (*imageMemoryAlloc)(OfxImageEffectHandle instanceHandle, 
				size_t nBytes,
				OfxImageMemoryHandle *memoryHandle);
	
  /** @brief Frees a memory handle and associated memory.
      
  \arg \c memoryHandle memory handle returned by imageMemoryAlloc

  This function frees a memory handle and associated memory that was previously allocated via OfxImageEffectSuiteV1::imageMemoryAlloc

  If there are outstanding locks, these are ignored and the handle and memory are freed anyway.

  See \ref ImageEffectsMemoryAllocation.

  @returns
  - kOfxStatOK if the memory was cleanly deleted
  - kOfxStatErrBadHandle if the value of \e memoryHandle was not a valid pointer returned by OfxImageEffectSuiteV1::imageMemoryAlloc
  */   
  OfxStatus (*imageMemoryFree)(OfxImageMemoryHandle memoryHandle);

  /** @brief Lock the memory associated with a memory handle and make it available for use.

  \arg \c memoryHandle memory handle returned by imageMemoryAlloc
  \arg \c returnedPtr where to the pointer to the locked memory

  This function locks them memory associated with a memory handle and returns a pointer to it. The memory will be 16 byte aligned, to allow use of vector operations.
  
  Note that memory locks and unlocks nest.

  After the first lock call, the contents of the memory pointer to by \e returnedPtr is undefined. All subsequent calls to lock will return memory with the same contents as  the previous call.

  Also, if unlocked, then relocked, the memory associated with a memory handle may be at a different address.

  See also OfxImageEffectSuiteV1::imageMemoryUnlock and \ref ImageEffectsMemoryAllocation.
    
  @returns
  - kOfxStatOK if the memory was locked, a pointer is placed in \e returnedPtr
  - kOfxStatErrBadHandle if the value of \e memoryHandle was not a valid pointer returned by OfxImageEffectSuiteV1::imageMemoryAlloc, null is placed in \e *returnedPtr
  - kOfxStatErrMemory if there was not enough memory to satisfy the call, \e *returnedPtr is set to NULL
  */
  OfxStatus (*imageMemoryLock)(OfxImageMemoryHandle memoryHandle,
			       void **returnedPtr);

  /** @brief Unlock allocated image data

  \arg \c allocatedData pointer to memory previously returned by OfxImageEffectSuiteV1::imageAlloc

  This function unlocks a previously locked memory handle. Once completely unlocked, memory associated with a memoryHandle is no longer available for use. Attempting to use it results in undefined behaviour.

  Note that locks and unlocks nest, and to fully unlock memory you need to match the count of locks placed upon it. 

  Also note, if you unlock a completely unlocked handle, it has no effect (ie: the lock count can't be negative).
    
  If unlocked, then relocked, the memory associated with a memory handle may be at a different address, however the contents will remain the same.

  See also OfxImageEffectSuiteV1::imageMemoryLock and \ref ImageEffectsMemoryAllocation.
  
  @returns
  - kOfxStatOK if the memory was unlocked cleanly,
  - kOfxStatErrBadHandle if the value of \e memoryHandle was not a valid pointer returned by OfxImageEffectSuiteV1::imageMemoryAlloc, null is placed in \e *returnedPtr
  */
  OfxStatus (*imageMemoryUnlock)(OfxImageMemoryHandle memoryHandle);

} OfxImageEffectSuiteV1;



/**
   \addtogroup StatusCodes
*/
/*@{*/
/**
   \defgroup StatusCodesImageEffect Image Effect API Status Codes 

These are status codes returned by functions in the OfxImageEffectSuite and Image Effect plugin functions.

They range from 1000 until 1999
*/
/*@{*/
/** @brief Error code for incorrect image formats */
#define kOfxStatErrImageFormat ((int) 1000)


/*@}*/
/*@}*/

#ifdef __cplusplus
}
#endif


#endif

