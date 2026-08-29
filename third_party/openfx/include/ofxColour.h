

#ifndef _ofxColour_h_
#define _ofxColour_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

/** @file ofxColour.h
Contains the API for colourspace data exchange.
*/

/** Define OFX_NO_DEFAULT_COLORSPACE_HEADER if you want to manage the
colourspace headers yourself. If this is not defined, the latest ofx-native
config will be included.
*/
#ifndef OFX_NO_DEFAULT_COLORSPACE_HEADER
#include "ofx-native-v1.5_aces-v1.3_ocio-v2.3.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief What style of colour management does the host or plug-in offer?

   - Type - string X 1
   - Property Set - host descriptor (read only), plugin descriptor (read/write), image effect instance (read only)
   - Default - kOfxImageEffectColourManagementStyleNone
   - Valid Values - This must be one of
     - ::kOfxImageEffectColourManagementNone - no colour management
     - ::kOfxImageEffectColourManagementBasic - only basic colourspaces from the config header may be used
     - ::kOfxImageEffectColourManagementCore - only core colourspaces from the config header may be used
     - ::kOfxImageEffectColourManagementFull - any colourspace from the config header may be used
     - ::kOfxImageEffectColourManagementOCIO - any OCIO config may be used (implies use of the OCIO library)

Hosts should set this property if they will provide colourspace information 
to plug-ins.
Plug-ins should set this property if they can use host-provided colourspace 
information.
Collectively, the full, core and basic styles are referred to as native
colour management. OCIO is used as the reference for the colour management
API, but is not required to implement the native styles.

The colourspace strings used in the native styles are from an OFX-specific
OCIO config which is currently based on the OCIO ACES Studio built-in config,
studio-config-v2.1.0_aces-v1.3_ocio-v2.3, and stored for OFX purposes in
ofx-native-v1.5_aces-v1.3_ocio-v2.3.h (referred to as the config header).
Additionally, there is a scheme for cross-referencing between clips, and a set
of "basic colourspaces", which are designed to be generic names for a family of
colourspaces. When a basic colourspace is used, this means the host is free to
choose any colourspace with the same encoding and reference space (scene vs
display).

The assumption is that OCIO > Full > Core > Basic, so the highest style
supported by both host and plug-in should usually be chosen by the host.
The chosen style must be set by the host using this property on an image
effect instance.
*/
#define kOfxImageEffectPropColourManagementStyle "OfxImageEffectPropColourManagementStyle"

/* String used to indicate that no colour management is available. */
#define kOfxImageEffectColourManagementNone "OfxImageEffectColourManagementNone"
/* String used to indicate that basic colour management is available. */
#define kOfxImageEffectColourManagementBasic "OfxImageEffectColourManagementBasic"
/* String used to indicate that core colour management is available. */
#define kOfxImageEffectColourManagementCore "OfxImageEffectColourManagementCore"
/* String used to indicate that Full colour management is available. */
#define kOfxImageEffectColourManagementFull "OfxImageEffectColourManagementFull"
/* String used to indicate that OCIO colour management is available. */
#define kOfxImageEffectColourManagementOCIO "OfxImageEffectColourManagementOCIO"

/** @brief What native mode configs are supported?

   - Type - string X N
   - Property Set - host descriptor (read only), plugin descriptor (read/write)
   - Valid Values - A list of config identifiers. The only currently supported
                    value is ofx-native-v1.5_aces-v1.3_ocio-v2.3.

While the API for colour management is expected to be stable, the set of
colourspaces will evolve over time. This property must be set by both plug-ins
and hosts specifying the list of native mode configs that are available on each
side.
*/
#define kOfxImageEffectPropColourManagementAvailableConfigs "OfxImageEffectPropColourManagementAvailableConfigs"

/** @brief The native colour management config to be used for this instance

   - Type - string X 1
   - Property Set - image effect instance (read only)
   - Valid Values - any string provided by the plug-in in
                    kOfxImageEffectPropColourManagementAvailableConfigs

The host must set this property to indicate the native colour management config
the plug-in should be used to look up colourspace strings. It is important to
set this even in OCIO mode, to define the basic colourspaces.
*/
#define kOfxImageEffectPropColourManagementConfig "OfxImageEffectPropColourManagementConfig"

/** @brief The path to the OCIO config used for this instance

   - Type - string X 1
   - Property Set - image effect instance (read only)
   - Valid Values - Filesystem path to the config or URI starting ocio://

A host must set this property on any effect instances where it has negotiated 
OCIO colour management (kOfxImageEffectColourManagementOCIO).
Use of URIs for built-in configs, such as ocio://default is permitted.
*/
#define kOfxImageEffectPropOCIOConfig "OfxImageEffectPropOCIOConfig"

/** @brief The colourspace used for this clip

   - Type - string X 1
   - Property Set - clip instance (read/write)
   - Valid Values - colourspace that is permitted under the style in use.
                    For OCIO, any string acceptable to Config::getColorSpace()

Hosts should set this property to the colourspace of the input clip. Typically
it will be set to the working colourspace of the host but could be any valid 
colourspace.

Both host and plug-in should use the value of 
kOfxImageClipPropPreferredColourspace where reasonable.

In OCIO mode, a basic colourspace may have been requested via
kOfxImageClipPropPreferredColourspaces, but the actual colourspace used should
be reported in this property. If an OCIO host has added the basic colourspaces
to its config as roles or aliases, they would be permitted here.

Cross-referencing between clips is possible by setting this property to
"OfxColourspace_<clip>". For example a plug-in may set this property during
kOfxImageEffectActionGetOutputColourspace to "OfxColourspace_Source", telling
he host that the colourspace of the output matches the "Source" input clip.
In the basic style, plug-ins are recommended to use cross-referencing for
their output clip unless kOfxColourspaceRaw is required.

If a clip sets OfxImageClipPropIsMask or it only supports
OfxImageComponentAlpha, colour management is disabled and this property
must be unset.
*/
#define kOfxImageClipPropColourspace "OfxImageClipPropColourspace"

/** @brief The preferred colourspace for this clip

   - Type - string X N
   - Property Set - clip instance (read only) and ::kOfxImageEffectActionGetClipPreferences action out args property (read/write)
   - Valid Values - colourspace that is permitted under the style in use. 
                    For Basic, any colourspace from the config header where IsBasic is true.
                    For Core, any colourspace from the config header where IsCore is true.
                    For Full, any colourspace from the config header.
                    For OCIO, any string acceptable to Config::getColorSpace(), or a basic colourspace.

Plug-ins may set this property during kOfxImageEffectActionGetClipPreferences 
to request images in a colourspace which is convenient for them. The
property is an ordered set of colourspace identifiers, which may be any of the
names or aliases supported by the colour management style in use. If plug-ins
prefer more esoteric colourspaces, they are encouraged to also include basic
colourspacesas a fallback. For example a colour grading plug-in which supports
a specific camera and expects a log colourspace might list:

"arri_logc4", "arri_logc3_ei800", "ACEScct", "ofx_scene_log"

The host is free to choose any colourspace from this list, but should favour
the first mutually agreeable colourspace, and set kOfxImageClipPropColourspace
to tell the plug-in which colourspace has been selected. A host does not need
to convert into the first choice just because it can, as this might be
inefficient, and should avoid scene-to-display or display-to-scene conversions
where possible.

In the event that the host cannot supply images in a requested colourspace,
it may supply images in any valid colourspace. Plug-ins must check
kOfxImageClipPropColourspace to see if their request was satisfied.

In both native and OCIO modes, it is recommended to include basic colourspaces
in this list, because this gives the host more flexibility to avoid unnecessary
colourspace conversions.

Hosts can invoke kOfxImageEffectActionGetOutputColourspace to set this on an
output clip, which could be helpful in a generator context, and plug-ins should
follow the same logic as hosts when deciding which colourspace to use.

It might be much less costly for a host to perform a conversion 
than a plug-in, so in the example of a plug-in which performs all internal 
processing in scene linear, it is sensible for the plug-in to universally 
assert that preference by preferring kOfxColourspaceOfxSceneLinear, and for
the host to honour it if possible. However, if a plug-in is capable of
adapting to any input colourspace, it should not set this preference.

Cross-referencing between clips is possible by setting this property to
"OfxColourspace_<clip>". For example, a plug-in in a transition context may set
this property on its "SourceTo" clip to "OfxColourspace_SourceFrom", telling
the host it would like both input clips to be in the same colourspace.

If a plug-in has inputs which expect motion vectors, depth values or other
non-colour channels, it should set the preferred colourspace to
kOfxColourspaceRaw. Similarly, if a host requests outputs in a typical scene
colourspace, but the plug-in is producing motion vectors, it should ignore
the request and set kOfxImageClipPropColourspace to kOfxColourspaceRaw.
*/
#define kOfxImageClipPropPreferredColourspaces "OfxImageClipPropPreferredColourspaces"

/** @brief The display colourspace used in the plug-in's viewport

   - Type - string X 1
   - Property Set - image effect instance (read only)
   - Valid Values - any colourspace from the config header

Used with native colour management styles, this property is relevant 
for plug-ins which have their own viewport in a custom window. Plug-ins should 
not expect this to be available during a render event.
Hosts should set this property to a display colourspace which matches that 
used in its own viewport. For a multi-display system, choose the colourspace 
for the display device where a native window would appear by default.
A host which supports OCIO should use the OCIO-specific display and view
properties instead.
 
*/
#define kOfxImageEffectPropDisplayColourspace "OfxImageEffectPropDisplayColourspace"

/** @brief The OCIO display to be used in the plug-in's viewport

   - Type - string X 1
   - Property Set - image effect instance (read only)
   - Valid Values - OCIO display that is present in the config

This OCIO-specific property allows the host to specify which OCIO display should be used. 
If not defined, the default rules for choosing a display will be followed.
*/
#define kOfxImageEffectPropOCIODisplay "OfxImageEffectPropOCIODisplay"

/** @brief The OCIO view to be used in the plug-in's viewport

   - Type - string X 1
   - Property Set - image effect instance (read only)
   - Valid Values - OCIO view for the display specified by kOfxImageEffectPropOCIODisplay

This OCIO-specific property allows the host to specify which OCIO view should be used.
If not defined, the default rules for choosing a view will be followed.
*/
#define kOfxImageEffectPropOCIOView "OfxImageEffectPropOCIOView"

/** @brief

 This action allows a host to ask an effect, given a list of preferred
 colourspaces, what colourspace will be used for its output clip. This should
 be called after first gathering the plug-in's preferred input colourspaces
 via OfxImageEffectActionGetClipPreferences.

 Cross-references to input clip colourspaces are permitted, for example in a
 filter context, the host might request "OfxColourspace_Source".

 If a host wants to rely on the output clip colourspace, it must call this
 action whenever the effect instance is changed. Do not assume that the output
 colourspace will be the same across all instances of an effect, or even for
 the lifetime of an effect instance, because a plug-in may change its output
 colourspace based on a user changing a parameter. Assume that the plug-in will
 check kOfxImageClipPropColourspace on its input clips during this action in
 order to decide the output clip colourspace, so the action must be called
 again if the host changes any of the other colour properties.

 On a successful return from this action, the host must set
 kOfxImageClipPropColourspace on the instance's output clip to the value
 from outArgs.
 
 @param  handle handle to the instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs a property set containing the property
     - \ref kOfxImageClipPropPreferredColourspaces the host's list of preferred colourspaces

 @param  outArgs a property set containing the property
     - \ref kOfxImageClipPropColourspace the colourspace selected by the plug-in,
            which may be a cross-reference to an input clip.

 @returns
     -  \ref kOfxStatOK, the action was trapped and the colourspace was set in
             the outArgs property set
     -  \ref kOfxStatReplyDefault, the action was not trapped and the host
             should use the colourspace of the first input clip 
     -  \ref kOfxStatErrMemory, in which case the action may be called again after a memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate, plugin to post message
     -  \ref kOfxStatErrFatal
 */
#define kOfxImageEffectActionGetOutputColourspace "OfxImageEffectActionGetOutputColourspace"

#ifdef __cplusplus
}
#endif

#endif
