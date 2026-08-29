

#pragma once
#ifndef __OFXGPURENDER_H__
#define __OFXGPURENDER_H__

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

/** @file ofxGPURender.h

	This file contains an optional suite for performing GPU-accelerated
	rendering of OpenFX Image Effect Plug-ins.  For details see
        \ref ofxGPURender.

        It allows hosts and plug-ins to support OpenGL, OpenCL, CUDA, and Metal.
        Additional GPU APIs, such a Vulkan, could use similar techniques.
*/

#include "ofxImageEffect.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup OpenGLRenderSuite OpenGL Render Suite
 * @{
 */

/** @brief The name of the OpenGL render suite, used to fetch from a host
via OfxHost::fetchSuite
*/
#define kOfxOpenGLRenderSuite			"OfxImageEffectOpenGLRenderSuite"
//#define kOfxOpenGLRenderSuite_ext		"OfxImageEffectOpenGLRenderSuite_ext"


#ifndef kOfxBitDepthHalf
/** @brief String used to label the OpenGL half float (16 bit floating
point) sample format */
  #define kOfxBitDepthHalf "OfxBitDepthHalf"
#endif

/** @brief Indicates whether a host or plug-in can support OpenGL accelerated
rendering

   - Type - C string X 1
   - Property Set - plug-in descriptor (read/write), host descriptor (read
only) - plug-in instance change (read/write)
   - Default - "false" for a plug-in
   - Valid Values - This must be one of
     - "false"  - in which case the host or plug-in does not support OpenGL
                  accelerated rendering
     - "true"   - which means a host or plug-in can support OpenGL accelerated
                  rendering, in the case of plug-ins this also means that it
                  is capable of CPU based rendering in the absence of a GPU
     - "needed" - only for plug-ins, this means that an plug-in has to have
                  OpenGL support, without which it cannot work.

V1.4: It is now expected from host reporting v1.4 that the plug-in can during instance change switch from true to false and false to true.

*/
#define kOfxImageEffectPropOpenGLRenderSupported "OfxImageEffectPropOpenGLRenderSupported"


/** @brief Indicates the bit depths supported by a plug-in during OpenGL renders.

    This is analogous to ::kOfxImageEffectPropSupportedPixelDepths. When a
    plug-in sets this property, the host will try to provide buffers/textures
    in one of the supported formats. Additionally, the target buffers where
    the plug-in renders to will be set to one of the supported formats.

    Unlike ::kOfxImageEffectPropSupportedPixelDepths, this property is
    optional. Shader-based effects might not really care about any
    format specifics when using OpenGL textures, so they can leave this unset
    and allow the host the decide the format.


   - Type - string X N
   - Property Set - plug-in descriptor (read only)
   - Default - none set
   - Valid Values - This must be one of
       - ::kOfxBitDepthNone (implying a clip is unconnected, not valid for an
         image)
       - ::kOfxBitDepthByte
       - ::kOfxBitDepthShort
       - ::kOfxBitDepthHalf
       - ::kOfxBitDepthFloat
*/
#define kOfxOpenGLPropPixelDepth "OfxOpenGLPropPixelDepth"


/** @brief Indicates that a plug-in SHOULD use OpenGL acceleration in
the current action

   When a plug-in and host have established they can both use OpenGL renders
   then when this property has been set the host expects the plug-in to render
   its result into the buffer it has setup before calling the render.  The
   plug-in can then also safely use the 'OfxImageEffectOpenGLRenderSuite'

   - Type - int X 1
   - Property Set - inArgs property set of the following actions...
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
   - Valid Values
      - 0 indicates that the plug-in cannot use the OpenGL suite
      - 1 indicates that the plug-in should render into the texture,
          and may use the OpenGL suite functions.

\note Once this property is set, the host and plug-in have agreed to
use OpenGL, so the effect SHOULD access all its images through the
OpenGL suite.

v1.4:  kOfxImageEffectPropOpenGLEnabled should probably be checked in Instance Changed prior to try to read image via clipLoadTexture

*/
#define kOfxImageEffectPropOpenGLEnabled "OfxImageEffectPropOpenGLEnabled"


/** @brief Indicates the texture index of an image turned into an OpenGL
texture by the host

   - Type - int X 1
   - Property Set - texture handle returned by
`        OfxImageEffectOpenGLRenderSuiteV1::clipLoadTexture (read only)

	This value should be cast to a GLuint and used as the texture index when
        performing OpenGL texture operations.

   The property set of the following actions should contain this property:
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
*/
#define kOfxImageEffectPropOpenGLTextureIndex "OfxImageEffectPropOpenGLTextureIndex"


/** @brief Indicates the texture target enumerator of an image turned into
    an OpenGL texture by the host

   - Type - int X 1
   - Property Set - texture handle returned by
        OfxImageEffectOpenGLRenderSuiteV1::clipLoadTexture (read only)
	This value should be cast to a GLenum and used as the texture target
	when performing OpenGL texture operations.

   The property set of the following actions should contain this property:
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
*/
#define kOfxImageEffectPropOpenGLTextureTarget "OfxImageEffectPropOpenGLTextureTarget"


/** @name StatusReturnValues
OfxStatus returns indicating that a OpenGL render error has occurred:

 - If a plug-in returns ::kOfxStatGLRenderFailed, the host should retry the
   render with OpenGL rendering disabled.

 - If a plug-in returns ::kOfxStatGLOutOfMemory, the host may choose to free
   resources on the GPU and retry the OpenGL render, rather than immediately
   falling back to CPU rendering.
 */
/**
 * @{
 */
/** @brief GPU render ran out of memory */
#define kOfxStatGPUOutOfMemory  ((int) 1001)
/** @brief OpenGL render ran out of memory (same as ``kOfxStatGPUOutOfMemory``) */
#define kOfxStatGLOutOfMemory  ((int) 1001)
/** @brief GPU render failed in a non-memory-related way */
#define kOfxStatGPURenderFailed ((int) 1002)
/** @brief OpenGL render failed in a non-memory-related way (same as ``kOfxStatGPURenderFailed``) */
#define kOfxStatGLRenderFailed ((int) 1002) /* for backward compatibility */
/** @} */

/** @brief OFX suite that provides image to texture conversion for OpenGL
    processing
 */
typedef struct OfxImageEffectOpenGLRenderSuiteV1
{
  /** @brief loads an image from an OFX clip as a texture into OpenGL

      \arg \c clip   clip to load the image from
      \arg \c time   effect time to load the image from
      \arg \c format requested texture format (As in
            none,byte,word,half,float, etc..)
            When set to NULL, the host decides the format based on the
	    plug-in's ::kOfxOpenGLPropPixelDepth setting.
      \arg \c region region of the image to load (optional, set to NULL to
            get a 'default' region)
	    this is in the \ref CanonicalCoordinates.
      \arg \c textureHandle property set containing information about the
            texture

  An image is fetched from a clip at the indicated time for the given region
  and loaded into an OpenGL texture. When a specific format is requested, the
  host ensures it gives the requested format.
  When the clip specified is the "Output" clip, the format is ignored and
  the host must bind the resulting texture as the current color buffer
  (render target). This may also be done prior to calling the
  ::kOfxImageEffectActionRender action.
  If the \em region parameter is set to non-NULL, then it will be clipped to
  the clip's Region of Definition for the given time.
  The returned image will be \em at \em least as big as this region.
  If the region parameter is not set or is NULL, then the region fetched will be at
  least the Region of Interest the effect has previously specified, clipped to
  the clip's Region of Definition.
  Information about the texture, including the texture index, is returned in
  the \em textureHandle argument.
  The properties on this handle will be...
    - ::kOfxImageEffectPropOpenGLTextureIndex
    - ::kOfxImageEffectPropOpenGLTextureTarget
    - ::kOfxImageEffectPropPixelDepth
    - ::kOfxImageEffectPropComponents
    - ::kOfxImageEffectPropPreMultiplication
    - ::kOfxImageEffectPropRenderScale
    - ::kOfxImagePropPixelAspectRatio
    - ::kOfxImagePropBounds
    - ::kOfxImagePropRegionOfDefinition
    - ::kOfxImagePropRowBytes
    - ::kOfxImagePropField
    - ::kOfxImagePropUniqueIdentifier

  With the exception of the OpenGL specifics, these properties are the same
  as the properties in an image handle returned by clipGetImage in the image
  effect suite.
\pre
 - clip was returned by clipGetHandle
 - Format property in the texture handle

\post
 - texture handle to be disposed of by clipFreeTexture before the action
returns
 - when the clip specified is the "Output" clip, the format is ignored and
   the host must bind the resulting texture as the current color buffer
   (render target).
   This may also be done prior to calling the render action.

@returns
  - ::kOfxStatOK           - the image was successfully fetched and returned
                             in the handle,
  - ::kOfxStatFailed       - the image could not be fetched because it does
                             not exist in the clip at the indicated
                             time and/or region, the plug-in should continue
                             operation, but assume the image was black and
			     transparent.
  - ::kOfxStatErrBadHandle - the clip handle was invalid,
  - ::kOfxStatErrMemory    - not enough OpenGL memory was available for the
                             effect to load the texture.
                             The plug-in should abort the GL render and
			     return ::kOfxStatErrMemory, after which the host can
			     decide to retry the operation with CPU based processing.

\note
  - this is the OpenGL equivalent of clipGetImage from OfxImageEffectSuiteV1


*/

  OfxStatus (*clipLoadTexture)(OfxImageClipHandle clip,
                               OfxTime       time,
                               const char   *format,
                               const OfxRectD     *region,
                               OfxPropertySetHandle   *textureHandle);

  /** @brief Releases the texture handle previously returned by
clipLoadTexture

  For input clips, this also deletes the texture from OpenGL.
  This should also be called on the output clip; for the Output
  clip, it just releases the handle but does not delete the
  texture (since the host will need to read it).

  \pre
    - textureHandle was returned by clipGetImage

  \post
    - all operations on textureHandle will be invalid, and the OpenGL texture
      it referred to has been deleted (for source clips)

  @returns
    - ::kOfxStatOK - the image was successfully fetched and returned in the
         handle,
    - ::kOfxStatFailed - general failure for some reason,
    - ::kOfxStatErrBadHandle - the image handle was invalid,
*/
  OfxStatus (*clipFreeTexture)(OfxPropertySetHandle   textureHandle);


  /** @brief Request the host to minimize its GPU resource load

  When a plug-in fails to allocate GPU resources, it can call this function to
  request the host to flush its GPU resources if it holds any.
  After the function the plug-in can try again to allocate resources which then
  might succeed if the host actually has released anything.

  \pre
  \post
    - No changes to the plug-in GL state should have been made.

  @returns
    - ::kOfxStatOK           - the host has actually released some
resources,
    - ::kOfxStatReplyDefault - nothing the host could do..
 */
  OfxStatus (*flushResources)( );

} OfxImageEffectOpenGLRenderSuiteV1;


/** @brief Action called when an effect has just been attached to an OpenGL
context.

The purpose of this action is to allow a plug-in to set up any data it may need
to do OpenGL rendering in an instance. For example...
   - allocate a lookup table on a GPU,
   - create an OpenCL or CUDA context that is bound to the host's OpenGL
     context so it can share buffers.

The plug-in will be responsible for deallocating any such shared resource in the
\ref ::kOfxActionOpenGLContextDetached action.

A host cannot call ::kOfxActionOpenGLContextAttached on the same instance
without an intervening ::kOfxActionOpenGLContextDetached. A host can have a
plug-in swap OpenGL contexts by issuing a attach/detach for the first context
then another attach for the next context.

The arguments to the action are...
  \arg \c handle handle to the plug-in instance, cast to an
  \ref OfxImageEffectHandle
  \arg \c inArgs is redundant and set to NULL
  \arg \c outArgs is redundant and set to NULL

A plug-in can return...
  - ::kOfxStatOK, the action was trapped and all was well
  - ::kOfxStatReplyDefault, the action was ignored, but all was well anyway
  - ::kOfxStatErrMemory, in which case this may be called again after a memory
    purge
  - ::kOfxStatFailed, something went wrong, but no error code appropriate,
    the plug-in should to post a message if possible and the host should not
    attempt to run the plug-in in OpenGL render mode.
*/
#define kOfxActionOpenGLContextAttached "OfxActionOpenGLContextAttached"

/** @brief Action called when an effect is about to be detached from an
OpenGL context

The purpose of this action is to allow a plug-in to deallocate any resource
allocated in \ref ::kOfxActionOpenGLContextAttached just before the host
decouples a plug-in from an OpenGL context.
The host must call this with the same OpenGL context active as it
called with the corresponding ::kOfxActionOpenGLContextAttached.

The arguments to the action are...
  \arg \c handle handle to the plug-in instance, cast to an
  \ref OfxImageEffectHandle
  \arg \c inArgs is redundant and set to NULL
  \arg \c outArgs is redundant and set to NULL

A plug-in can return...
  - ::kOfxStatOK, the action was trapped and all was well
  - ::kOfxStatReplyDefault, the action was ignored, but all was well anyway
  - ::kOfxStatErrMemory, in which case this may be called again after a memory
    purge
  - ::kOfxStatFailed, something went wrong, but no error code appropriate,
    the plug-in should to post a message if possible and the host should not
    attempt to run the plug-in in OpenGL render mode.
*/
#define kOfxActionOpenGLContextDetached "kOfxActionOpenGLContextDetached"


/** @page ofxOpenGLRender OpenGL Acceleration of Rendering

@section ofxOpenGLRenderIntro Introduction

The OfxOpenGLRenderSuite allows image effects to use OpenGL commands
(hopefully backed by a GPU) to accelerate rendering
of their outputs. The basic scheme is simple....
  - An effect indicates it wants to use OpenGL acceleration by setting the
    ::kOfxImageEffectPropOpenGLRenderSupported flag on its descriptor
  - A host indicates it supports OpenGL acceleration by setting
    ::kOfxImageEffectPropOpenGLRenderSupported on its descriptor
  - In an effect's ::kOfxImageEffectActionGetClipPreferences action, an
    effect indicates what clips it will be loading images from onto the GPU's
    memory during an effect's ::kOfxImageEffectActionRender action.

@section ofxOpenGLRenderHouseKeeping OpenGL House Keeping

If a host supports OpenGL rendering then it flags this with the string
property ::kOfxImageEffectPropOpenGLRenderSupported on its descriptor property
set. Effects that cannot run without OpenGL support should examine this in
::kOfxActionDescribe action and return a ::kOfxStatErrMissingHostFeature
status flag if it is not set to "true".

Effects flag to a host that they support OpenGL rendering by setting the
string property ::kOfxImageEffectPropOpenGLRenderSupported on their effect
descriptor during the ::kOfxActionDescribe action. Effects can work in three
ways....
  - purely on CPUs without any OpenGL support at all, in which case they
    should set ::kOfxImageEffectPropOpenGLRenderSupported to be "false" (the
    default),
  - on CPUs but with optional OpenGL support, in which case they should set
    ::kOfxImageEffectPropOpenGLRenderSupported to be "true",
  - only with OpenGL support, in which case they should set
    ::kOfxImageEffectPropOpenGLRenderSupported to be "needed".

Hosts can examine this flag and respond to it appropriately.

Effects can use OpenGL accelerated rendering during the following
action...
  - ::kOfxImageEffectActionRender

If an effect has indicated that it optionally supports OpenGL acceleration,
it should check the property ::kOfxImageEffectPropOpenGLEnabled
passed as an in argument to the following actions,
  - ::kOfxImageEffectActionRender
  - ::kOfxImageEffectActionBeginSequenceRender
  - ::kOfxImageEffectActionEndSequenceRender

If this property is set to 0, then it should not attempt to use any calls to
the OpenGL suite or OpenGL calls whilst rendering.


@section ofxOpenGLRenderGettingTextures Getting Images as Textures

An effect could fetch an image into memory from a host via the standard
Image Effect suite "clipGetImage" call, then create an OpenGL
texture from that. However as several buffer copies and various other bits
of house keeping may need to happen to do this, it is more
efficient for a host to create the texture directly.

The OfxOpenGLRenderSuiteV1::clipLoadTexture function does this. The
arguments and semantics are similar to the
OfxImageEffectSuiteV2::clipGetImage function, with a few minor changes.

The effect is passed back a property handle describing the texture. Once the
texture is finished with, this should be disposed
of via the OfxOpenGLRenderSuiteV1::clipFreeTexture function, which will also
delete the associated OpenGL texture (for source clips).

The returned handle has a set of properties on it, analogous to the
properties returned on the image handle by
OfxImageEffectSuiteV2::clipGetImage. These are:
    - ::kOfxImageEffectPropOpenGLTextureIndex
    - ::kOfxImageEffectPropOpenGLTextureTarget
    - ::kOfxImageEffectPropPixelDepth
    - ::kOfxImageEffectPropComponents
    - ::kOfxImageEffectPropPreMultiplication
    - ::kOfxImageEffectPropRenderScale
    - ::kOfxImagePropPixelAspectRatio
    - ::kOfxImagePropBounds
    - ::kOfxImagePropRegionOfDefinition
    - ::kOfxImagePropRowBytes
    - ::kOfxImagePropField
    - ::kOfxImagePropUniqueIdentifier

The main difference between this and an image handle is that the
::kOfxImagePropData property is replaced by the
kOfxImageEffectPropOpenGLTextureIndex property.
This integer property should be cast to a GLuint and is the index to use for
the OpenGL texture.
Next to texture handle the texture target enumerator is given in
kOfxImageEffectPropOpenGLTextureTarget

Note, because the image is being directly loaded into a texture by the host
it need not obey the Clip Preferences action to remap the image to the pixel
depth the effect requested.

@section ofxOpenGLRenderOutput Render Output Directly with OpenGL

Effects can use the graphics context as they see fit. They may be doing
several render passes with fetch back from the card to main memory
via 'render to texture' mechanisms interleaved with passes performed on the
CPU. The effect must leave output on the graphics card in the provided output
image texture buffer.

The host will create a default OpenGL viewport that is the size of the
render window passed to the render action. The following
code snippet shows how the viewport should be rooted at the bottom left of
the output texture.

\verbatim
  // set up the OpenGL context for the render to texture
  ...

  // figure the size of the render window
  int dx = renderWindow.x2 - renderWindow.x1;
  int dy = renderWindow.y2 - renderWindow.y2;

  // setup the output viewport
  glViewport(0, 0, dx, dy);

\endverbatim

Prior to calling the render action the host may also choose to
bind the output texture as the current color buffer (render target), or they
may defer doing this until clipLoadTexture is called for the output clip.

After this, it is completely up to the effect to choose what OpenGL
operations to render with, including projections and so on.

@section ofxOpenGLRenderContext OpenGL Current Context

The host is only required to make the OpenGL context current (e.g.,
using wglMakeCurrent, for Windows) during the following actions:

   - ::kOfxImageEffectActionRender
   - ::kOfxImageEffectActionBeginSequenceRender
   - ::kOfxImageEffectActionEndSequenceRender
   - ::kOfxActionOpenGLContextAttached
   - ::kOfxActionOpenGLContextDetached

For the first 3 actions, Render through EndSequenceRender, the host is only
required to set the OpenGL context if ::kOfxImageEffectPropOpenGLEnabled is
set.  In other words, a plug-in should not expect the OpenGL context to be
current for other OFX calls, such as ::kOfxImageEffectActionDescribeInContext.

*/

/** @}*/ // end of OpenGLRender doc group

/**
 * @defgroup CudaRender CUDA Rendering
 * @version CUDA rendering was added in version 1.5.
 *
 * @{
 */
/** @brief Indicates whether a host or plug-in can support CUDA render

    - Type - string X 1
    - Property Set - plug-in descriptor (read/write), host descriptor (read only)
    - Default - "false" for a plug-in
    - Valid Values - This must be one of
      - "false"  - the host or plug-in does not support CUDA render
      - "true"   - the host or plug-in can support CUDA render
 */
#define kOfxImageEffectPropCudaRenderSupported "OfxImageEffectPropCudaRenderSupported"

/** @brief Indicates that a plug-in SHOULD use CUDA render in
the current action

   If a plug-in and host have both set
   kOfxImageEffectPropCudaRenderSupported="true" then the host MAY set
   this property to indicate that it is passing images as CUDA memory
   pointers.

   - Type - int X 1
   - Property Set - inArgs property set of the following actions...
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
   - Valid Values
      - 0 indicates that the kOfxImagePropData of each image of each clip
          is a CPU memory pointer.
      - 1 indicates that the kOfxImagePropData of each image of each clip
	      is a CUDA memory pointer.
*/
#define kOfxImageEffectPropCudaEnabled "OfxImageEffectPropCudaEnabled"

/**  @brief Indicates whether a host or plug-in can support CUDA streams

    - Type - string X 1
    - Property Set - plug-in descriptor (read/write), host descriptor (read only)
    - Default - "false" for a plug-in
    - Valid Values - This must be one of
      - "false"  - in which case the host or plug-in does not support CUDA streams
      - "true"   - which means a host or plug-in can support CUDA streams

*/
#define kOfxImageEffectPropCudaStreamSupported "OfxImageEffectPropCudaStreamSupported"

/**  @brief The CUDA stream to be used for rendering

    - Type - pointer X 1
    - Property Set - inArgs property set of the following actions...
       - ::kOfxImageEffectActionRender
       - ::kOfxImageEffectActionBeginSequenceRender
       - ::kOfxImageEffectActionEndSequenceRender

This property will only be set if the host and plug-in both support CUDA streams.

If set:

- this property contains a pointer to the stream of CUDA render (cudaStream_t).
  In order to use it, reinterpret_cast<cudaStream_t>(pointer) is needed.

- the plug-in SHOULD ensure that its render action enqueues any
  asynchronous CUDA operations onto the supplied queue.

- the plug-in SHOULD NOT wait for final asynchronous operations to
  complete before returning from the render action, and SHOULD NOT
  call cudaDeviceSynchronize() at any time.

If not set:

- the plug-in SHOULD ensure that any asynchronous operations it
  enqueues have completed before returning from the render action.
*/
#define kOfxImageEffectPropCudaStream "OfxImageEffectPropCudaStream"

/** @}*/ // end CudaRender doc group

/**
 * @defgroup MetalRender Apple Metal Rendering
 * @version Metal rendering was added in version 1.5.
 * @{
 */
/** @brief Indicates whether a host or plug-in can support Metal render

    - Type - string X 1
    - Property Set - plug-in descriptor (read/write), host descriptor (read only)
    - Default - "false" for a plug-in
    - Valid Values - This must be one of
      - "false"  - the host or plug-in does not support Metal render
      - "true"   - the host or plug-in can support Metal render
 */
#define kOfxImageEffectPropMetalRenderSupported "OfxImageEffectPropMetalRenderSupported"

/** @brief Indicates that a plug-in SHOULD use Metal render in
the current action

   If a plug-in and host have both set
   kOfxImageEffectPropMetalRenderSupported="true" then the host MAY
   set this property to indicate that it is passing images as Metal
   buffers.

   - Type - int X 1
   - Property Set - inArgs property set of the following actions...
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
   - Valid Values
      - 0 indicates that the kOfxImagePropData of each image of each clip
          is a CPU memory pointer.
      - 1 indicates that the kOfxImagePropData of each image of each clip
	      is a Metal id<MTLBuffer>.
*/
#define kOfxImageEffectPropMetalEnabled "OfxImageEffectPropMetalEnabled"

/**  @brief The command queue of Metal render

    - Type - pointer X 1
    - Property Set - inArgs property set of the following actions...
       - ::kOfxImageEffectActionRender
       - ::kOfxImageEffectActionBeginSequenceRender
       - ::kOfxImageEffectActionEndSequenceRender

This property contains a pointer to the command queue to be used for
Metal rendering (id<MTLCommandQueue>). In order to use it,
reinterpret_cast<id<MTLCommandQueue>>(pointer) is needed.

The plug-in SHOULD ensure that its render action enqueues any
asynchronous Metal operations onto the supplied queue.

The plug-in SHOULD NOT wait for final asynchronous operations to
complete before returning from the render action.
*/
#define kOfxImageEffectPropMetalCommandQueue "OfxImageEffectPropMetalCommandQueue"
/** @}*/ // end MetalRender doc group

/**
 * @defgroup OpenClRender OpenCL Rendering
 * @version OpenCL rendering was added in version 1.5.
 * @{
 */
/** @brief Indicates whether a host or plug-in can support OpenCL Buffers render

    - Type - string X 1
    - Property Set - plug-in descriptor (read/write), host descriptor (read only)
    - Default - "false" for a plug-in
    - Valid Values - This must be one of
      - "false"  - the host or plug-in does not support OpenCL Buffers render
      - "true"   - the host or plug-in can support OpenCL Buffers render
 */
#define kOfxImageEffectPropOpenCLRenderSupported "OfxImageEffectPropOpenCLRenderSupported"

 /** @brief Indicates whether a host or plug-in can support OpenCL Images render

    - Type - string X 1
    - Property Set - plug-in descriptor (read/write), host descriptor (read only)
    - Default - "false" for a plug-in
    - Valid Values - This must be one of
      - "false"  - in which case the host or plug-in does not support OpenCL Images render
      - "true"   - which means a host or plug-in can support OpenCL Images render
 */
#define kOfxImageEffectPropOpenCLSupported				"OfxImageEffectPropOpenCLSupported"

 /** @brief Indicates that a plug-in SHOULD use OpenCL render in
the current action

   If a plug-in and host have both set
   kOfxImageEffectPropOpenCLRenderSupported="true" or have both 
   set kOfxImageEffectPropOpenCLSupported="true" then the host MAY
   set this property to indicate that it is passing images as OpenCL
   Buffers or Images.

   When rendering using OpenCL Buffers, the cl_mem of the buffers are retrieved using ::kOfxImagePropData.
   When rendering using OpenCL Images, the cl_mem of the images are retrieved using ::kOfxImageEffectPropOpenCLImage.
   If both ::kOfxImageEffectPropOpenCLSupported (Buffers) and ::kOfxImageEffectPropOpenCLRenderSupported (Images) are
   enabled by the plug-in, it should use ::kOfxImageEffectPropOpenCLImage to determine which is being used by the host.

   - Type - int X 1
   - Property Set - inArgs property set of the following actions...
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
   - Valid Values
      - 0 indicates that a plug-in SHOULD use OpenCL render in
          the render action
      - 1 indicates that a plug-in SHOULD NOT use OpenCL render in
          the render action
*/
#define kOfxImageEffectPropOpenCLEnabled "OfxImageEffectPropOpenCLEnabled"

/**  @brief Indicates the OpenCL command queue that should be used for rendering

    - Type - pointer X 1
    - Property Set - inArgs property set of the following actions...
       - ::kOfxImageEffectActionRender
       - ::kOfxImageEffectActionBeginSequenceRender
       - ::kOfxImageEffectActionEndSequenceRender

This property contains a pointer to the command queue to be used for
OpenCL rendering (cl_command_queue). In order to use it,
reinterpret_cast<cl_command_queue>(pointer) is needed.

The plug-in SHOULD ensure that its render action enqueues any
asynchronous OpenCL operations onto the supplied queue.

The plug-in SHOULD NOT wait for final asynchronous operations to
complete before returning from the render action.
*/
#define kOfxImageEffectPropOpenCLCommandQueue "OfxImageEffectPropOpenCLCommandQueue"

/** @brief Indicates the image handle of an image supplied as an OpenCL Image by the host

- Type - pointer X 1
- Property Set - image handle returned by clipGetImage

This value should be cast to a cl_mem and used as the image handle when performing
OpenCL Images operations. The property should be used (not ::kOfxImagePropData) when
rendering with OpenCL Images (::kOfxImageEffectPropOpenCLSupported), and should be used
to determine whether Images or Buffers should be used if a plug-in supports both
::kOfxImageEffectPropOpenCLSupported and ::kOfxImageEffectPropOpenCLRenderSupported.
Note: the kOfxImagePropRowBytes property is not required to be set by the host, since
OpenCL Images do not have the concept of row bytes.
*/
#define kOfxImageEffectPropOpenCLImage					"OfxImageEffectPropOpenCLImage"


#define kOfxOpenCLProgramSuite "OfxOpenCLProgramSuite"

/** @brief OFX suite that allows a plug-in to get OpenCL programs compiled

This is an optional suite the host can provide for building OpenCL programs for the plug-in,
as an alternative to calling clCreateProgramWithSource / clBuildProgram. There are two advantages to
doing this: The host can add flags (such as -cl-denorms-are-zero) to the build call, and may also
cache program binaries for performance (however, if the source of the program or the OpenCL
environment changes, the host must recompile so some mechanism such as hashing must be used).
*/
typedef struct OfxOpenCLProgramSuiteV1 {
    /** @brief Compiles the OpenCL program */
    OfxStatus(*compileProgram)(const char   *pszProgramSource,
        int           fOptional,          // if non-zero, host may skip compiling on this call
        void         *pResult);           // cast to cl_program*
} OfxOpenCLProgramSuiteV1;


/** @page ofxOpenCLRender OpenCL Acceleration of Rendering

@section ofxOpenCLRenderIntro Introduction

The OpenCL extension enables plug-ins to use OpenCL commands (typically backed by a GPU) to accelerate rendering of their outputs. The basic scheme is simple....
- an plug-in indicates it wants to use OpenCL acceleration by setting the ::kOfxImageEffectPropOpenCLSupported (Images) and/or ::kOfxImageEffectPropOpenCLRenderSupported (Buffers) flags on it's descriptor.
- a host indicates it supports OpenCL acceleration by setting ::kOfxImageEffectPropOpenCLSupported (Images) and/or ::kOfxImageEffectPropOpenCLRenderSupported (Buffers) on it's descriptor.
- the host decides when to use OpenCL, and sets the ::kOfxImageEffectPropOpenCLEnabled property on the BeginRender/Render/EndRender calls to indicate this.
- when OpenCL Images are being used (::kOfxImageEffectPropOpenCLSupported) the clip image property ::kOfxImageEffectPropOpenCLImage will be set and non-null.
- when OpenCL Buffers are being used (::kOfxImageEffectPropOpenCLRenderSupported) the clip image property ::kOfxImagePropData will be set and non-null.

@section ofxOpenCLRenderDiscoveryAndEnabling Discovery and Enabling

If a host supports OpenCL rendering then it flags with the string property ::kOfxImageEffectPropOpenCLSupported (Images) and/or
::kOfxImageEffectPropOpenCLRenderSupported (Buffers) on its descriptor property set. Effects that cannot run without OpenCL support
should examine this in ::kOfxActionDescribe action and return a ::kOfxStatErrMissingHostFeature status flag if it is not set to "true".

Effects flag to a host that they support OpenCL rendering by setting the string property ::kOfxImageEffectPropOpenCLSupported (Images)
and/or ::kOfxImageEffectPropOpenCLRenderSupported (Buffers) on their effect descriptor during the ::kOfxActionDescribe action.
Effects can work in two ways....
- purely on CPUs without any OpenCL support at all, in which case they should set ::kOfxImageEffectPropOpenCLSupported (Images) and ::kOfxImageEffectPropOpenCLRenderSupported (Buffers) to be "false" (the default),
- on CPUs but with optional OpenCL support, in which case they should set ::kOfxImageEffectPropOpenCLSupported (Images) and/or ::kOfxImageEffectPropOpenCLRenderSupported (Buffers) to be "true"

Host may support just OpenCL Images, just OpenCL Buffers, or both, as indicated by which of these two properties they set "true".
Likewise plug-ins may support just OpenCL Images, just OpenCL Buffers, or both, as indicated by which of these two properties they set "true".
If both host and plug-in support both, it is up to the host which it uses. Typically, it will be based on what it uses natively (to avoid an extra copy operation).
If a plug-in supports both, it must use ::kOfxImageEffectPropOpenCLImage to determine if Images or Buffers are being used for a given render action.

Effects can use OpenCL render only during the following action:
- ::kOfxImageEffectActionRender

If a plug-in has indicated that it optionally supports OpenCL acceleration, it should check the property ::kOfxImageEffectPropOpenCLEnabled
passed as an in argument to the following actions,
- ::kOfxImageEffectActionRender
- ::kOfxImageEffectActionBeginSequenceRender
- ::kOfxImageEffectActionEndSequenceRender

If this property is set to 0, then it must not attempt to use OpenCL while rendering.
If this property is set to 1, then it must use OpenCL buffers or images while rendering.

If a call using OpenCL rendering fails, the host may re-attempt using CPU buffers instead, but this is not required, and might not be efficient.

@section ofxOpenCLRenderVersion OpenCL platform and device versions and feature support

Assume an in-order command queue. Do not assume a profiling command queue.

Effects should target OpenCL 1.1 API and OpenCL C kernel language support. Only minimum required features required
in OpenCL 1.1 should be used (for example, see "5.3.2.1 Minimum List of Supported Image Formats" for the list
of image types which can be expected to be supported across all devices). If you have specific requirements
for features beyond these minimums, you will need to check the device (e.g., using clGetDeviceInfo with
CL_DEVICE_EXTENSIONS) to see if your feature is available, and have a fallback if it's not.

Temporary buffers and images should not be kept past the render action. A separate extension for host managed caching is in the works.

Do not retain OpenCL objects without a matching release within the render action.

@section ofxOpenCLRenderMultipleDevices Multiple OpenCL Devices

This is very important: The host may support multiple OpenCL devices. Therefore the plug-in should keep a separate set of kernels per OpenCL content (e.g., using a map).
The OpenCL context can be found from the command queue using clGetCommandQueueInfo with CL_QUEUE_CONTEXT.
Failure to do this will cause crashes or incorrect results when the host switches to another OpenCL device.

*/

/** @}*/ // end OpenCLRender doc group


#ifdef __cplusplus
}
#endif

#endif /*__OFXGPURENDER_H__ */
