

#ifndef _ofxParam_h_
#define _ofxParam_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

#include "ofxCore.h"
#include "ofxProperty.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief string value to the ::kOfxPropType property for all parameters */
#define kOfxParameterSuite "OfxParameterSuite"

/** @brief string value on the ::kOfxPropType property for all parameter definitions (ie: the handle returned in describe) */
#define kOfxTypeParameter "OfxTypeParameter"

/** @brief string value on the ::kOfxPropType property for all parameter instances */
#define kOfxTypeParameterInstance "OfxTypeParameterInstance"

/** @brief Blind declaration of an OFX param 
    
*/
typedef struct OfxParamStruct *OfxParamHandle;

/** @brief Blind declaration of an OFX parameter set

 */
typedef struct OfxParamSetStruct *OfxParamSetHandle;


/**
   \defgroup ParamTypeDefines Parameter Type definitions 

These strings are used to identify the type of the parameter when it is defined, they are also on the ::kOfxParamPropType in any parameter instance.
*/
/*@{*/

/** @brief String to identify a param as a single valued integer */
#define kOfxParamTypeInteger "OfxParamTypeInteger"
/** @brief String to identify a param as a Single valued floating point parameter  */
#define kOfxParamTypeDouble "OfxParamTypeDouble"
/** @brief String to identify a param as a Single valued boolean parameter */
#define kOfxParamTypeBoolean "OfxParamTypeBoolean"
/** @brief String to identify a param as a Single valued, 'one-of-many' parameter */
#define kOfxParamTypeChoice "OfxParamTypeChoice"
/** @brief String to identify a param as a string-valued 'one-of-many' parameter. \since Version 1.5 */
#define kOfxParamTypeStrChoice "OfxParamTypeStrChoice"
/** @brief String to identify a param as a Red, Green, Blue and Alpha colour parameter */
#define kOfxParamTypeRGBA "OfxParamTypeRGBA"
/** @brief String to identify a param as a Red, Green and Blue colour parameter */
#define kOfxParamTypeRGB "OfxParamTypeRGB"
/** @brief String to identify a param as a Two dimensional floating point parameter */
#define kOfxParamTypeDouble2D "OfxParamTypeDouble2D"
/** @brief String to identify a param as a Two dimensional integer point parameter */
#define kOfxParamTypeInteger2D "OfxParamTypeInteger2D"
/** @brief String to identify a param as a Three dimensional floating point parameter */
#define kOfxParamTypeDouble3D "OfxParamTypeDouble3D"
/** @brief String to identify a param as a Three dimensional integer parameter */
#define kOfxParamTypeInteger3D "OfxParamTypeInteger3D"
/** @brief String to identify a param as a String (UTF8) parameter */
#define kOfxParamTypeString "OfxParamTypeString"
/** @brief String to identify a param as a Plug-in defined parameter */
#define kOfxParamTypeCustom "OfxParamTypeCustom"
/** @brief String to identify a param as a Plug-in defined opaque data parameter */
#define kOfxParamTypeBytes "OfxParamTypeBytes"
/** @brief String to identify a param as a Grouping parameter */
#define kOfxParamTypeGroup "OfxParamTypeGroup"
/** @brief String to identify a param as a page parameter */
#define kOfxParamTypePage "OfxParamTypePage"
/** @brief String to identify a param as a PushButton parameter */
#define kOfxParamTypePushButton "OfxParamTypePushButton"
/*@}*/

/** @brief Provides information for a parameter of type kOfxParamTypeBytes
 */
typedef struct OfxBytes
{
    /** @brief a pointer to the data buffer */
    const unsigned char *data;
    /** @brief the length of the data buffer, in bytes */
    size_t length;
} OfxBytes;

/**
   \addtogroup PropertiesAll
*/
/*@{*/
/**
   \defgroup ParamPropDefines Parameter Property Definitions 

These are the list of properties used by the parameters suite.
*/
/*@{*/

/** @brief Indicates if the host supports animation of custom parameters 

    - Type - int X 1
    - Property Set - host descriptor (read only)
    - Value Values - 0 or 1
*/
#define kOfxParamHostPropSupportsCustomAnimation "OfxParamHostPropSupportsCustomAnimation"

/** @brief Indicates if the host supports animation of string params 

    - Type - int X 1 
    - Property Set - host descriptor (read only)
    - Valid Values - 0 or 1
*/
#define kOfxParamHostPropSupportsStringAnimation "OfxParamHostPropSupportsStringAnimation"

/** @brief Indicates if the host supports animation of boolean params

    - Type - int X 1
    - Property Set - host descriptor (read only)
    - Valid Values - 0 or 1
*/
#define kOfxParamHostPropSupportsBooleanAnimation "OfxParamHostPropSupportsBooleanAnimation"

/** @brief Indicates if the host supports animation of choice params 

    - Type - int X 1
    - Property Set - host descriptor (read only)
    - Valid Values - 0 or 1
*/
#define kOfxParamHostPropSupportsChoiceAnimation "OfxParamHostPropSupportsChoiceAnimation"

/** @brief Indicates if the host supports custom interacts for parameters

    - Type - int X 1
    - Property Set - host descriptor (read only)
    - Valid Values - 0 or 1

Currently custom interacts for parameters can only be drawn using OpenGL.
APIs will be added later to support using the new Draw Suite.
*/
#define kOfxParamHostPropSupportsCustomInteract "OfxParamHostPropSupportsCustomInteract"

/** @brief Indicates the maximum numbers of parameters available on the host.

    - Type - int X 1
    - Property Set - host descriptor (read only)

If set to -1 it implies unlimited number of parameters.
*/
#define kOfxParamHostPropMaxParameters "OfxParamHostPropMaxParameters"

/** @brief Indicates the maximum number of parameter pages.

    - Type - int X 1
    - Property Set - host descriptor (read only)

    If there is no limit to the number of pages on a host, set this to -1.
    
Hosts that do not support paged parameter layout should set this to zero.
*/
#define kOfxParamHostPropMaxPages "OfxParamHostPropMaxPages"

/** @brief This indicates the number of parameter rows and coloumns on a page.

    - Type - int X 2
    - Property Set - host descriptor (read only)

If the host has supports paged parameter layout, used dimension 0 as the number of columns per page and dimension 1 as the number of rows per page.
*/
#define kOfxParamHostPropPageRowColumnCount "OfxParamHostPropPageRowColumnCount"

/** @brief Pseudo parameter name used to skip a row in a page layout.

Passed as a value to the \ref kOfxParamPropPageChild property.

See \ref ParametersInterfacesPagedLayouts for more details.
*/
#define kOfxParamPageSkipRow "OfxParamPageSkipRow"

/** @brief Pseudo parameter name used to skip a row in a page layout.

Passed as a value to the \ref kOfxParamPropPageChild property.

See \ref ParametersInterfacesPagedLayouts for more details.
*/
#define kOfxParamPageSkipColumn "OfxParamPageSkipColumn"

/** @brief Overrides the parameter's standard user interface with the given interact.

    - Type - pointer X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - NULL
    - Valid Values -  must point to a OfxPluginEntryPoint

If set, the parameter's normal interface is replaced completely by the interact gui.

Currently custom interacts for parameters can only be drawn using OpenGL.
APIs will be added later to support using the new Draw Suite.
*/
#define kOfxParamPropInteractV1 "OfxParamPropInteractV1"

/** @brief The size of a parameter instance's custom interface in screen pixels.
  
  - Type - double x 2
  - Property Set - plugin parameter instance (read only)

This is set by a host to indicate the current size of a custom interface if the plug-in has one. If not this is set to (0,0).
*/
#define kOfxParamPropInteractSize "OfxParamPropInteractSize"

/** @brief The preferred aspect ratio of a parameter's custom interface.

    - Type - double x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 1.0
    - Valid Values - greater than or equal to 0.0

If set to anything other than 0.0, the custom interface for this parameter will be of a size with this aspect ratio (x size/y size).
*/
#define kOfxParamPropInteractSizeAspect "OfxParamPropInteractSizeAspect"

/** @brief The minimum size of a parameter's custom interface, in screen pixels.

    - Type - double x 2
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 10,10
    - Valid Values - greater than (0, 0)

Any custom interface will not be less than this size.
*/
#define kOfxParamPropInteractMinimumSize "OfxParamPropInteractMinimumSize"

/** @brief The preferred size of a parameter's custom interface.

    - Type - int x 2
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 10,10
    - Valid Values - greater than (0, 0)

  A host should attempt to set a parameter's custom interface on a parameter to be this size if possible, otherwise it will be of ::kOfxParamPropInteractSizeAspect aspect but larger than ::kOfxParamPropInteractMinimumSize.
*/
#define kOfxParamPropInteractPreferedSize "OfxParamPropInteractPreferedSize"

/** @brief The type of a parameter.

   - Type - C string X 1
   - Property Set - plugin parameter descriptor (read only) and instance (read only)

This string will be set to the type that the parameter was create with.
*/
#define kOfxParamPropType "OfxParamPropType"

/** @brief Flags whether a parameter can animate.

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 1
    - Valid Values - 0 or 1

A plug-in uses this property to indicate if a parameter is able to animate.
*/
#define kOfxParamPropAnimates "OfxParamPropAnimates"

/** @brief Flags whether changes to a parameter should be put on the undo/redo stack

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 1
    - Valid Values - 0 or 1
*/
#define kOfxParamPropCanUndo "OfxParamPropCanUndo"

/** @brief States whether the plugin needs to resync its private data

    - Type - int X 1
    - Property Set - param set instance (read/write)
    - Default - 0
    - Valid Values - 
        - 0 - no need to sync
        - 1 - paramset is not synced

The plugin should set this flag to true whenever any internal state has not
been flushed to the set of params.

The host will examine this property each time it does a copy or save
operation on the instance.
 * If it is set to 1, the host will call SyncPrivateData and then set
   it to zero before doing the copy/save.
 * If it is set to 0, the host will assume that the param data
   correctly represents the private state, and will not call
   SyncPrivateData before copying/saving.
 * If this property is not set, the host will always call
   SyncPrivateData before copying or saving the effect (as if the
   property were set to 1 -- but the host will not create or
   modify the property).
*/
#define kOfxPropParamSetNeedsSyncing "OfxPropParamSetNeedsSyncing"
 
/** @brief Flags whether a parameter is currently animating.

    - Type - int x 1
    - Property Set - plugin parameter instance (read only)
    - Valid Values - 0 or 1

Set by a host on a parameter instance to indicate if the parameter has a non-constant value set on it. This can
be as a consequence of animation or of scripting modifying the value, or of a parameter being connected to
an expression in the host.
*/
#define kOfxParamPropIsAnimating "OfxParamPropIsAnimating"

/** @brief Flags whether the plugin will attempt to set the value of a parameter in some callback or analysis pass

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 0
    - Valid Values - 0 or 1

This is used to tell the host whether the plug-in is going to attempt to set the value of the parameter.

@deprecated - v1.4: deprecated - to be removed in 1.5
*/

#define kOfxParamPropPluginMayWrite "OfxParamPropPluginMayWrite"

/** @brief Flags whether the value of a parameter should persist.

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 1
    - Valid Values - 0 or 1

This is used to tell the host whether the value of the parameter is important and should be save in any description of the plug-in.
*/
#define kOfxParamPropPersistant "OfxParamPropPersistant"

/** @brief Flags whether changing a parameter's value forces an evalution (ie: render),

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write only)
    - Default - 1
    - Valid Values - 0 or 1

This is used to indicate if the value of a parameter has any affect on an effect's output, eg: the parameter may be purely for GUI purposes, and so changing its value should not trigger a re-render.
*/
#define kOfxParamPropEvaluateOnChange "OfxParamPropEvaluateOnChange"

/** @brief Flags whether a parameter should be exposed to a user,

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write)
    - Default - 0
    - Valid Values - 0 or 1

If secret, a parameter is not exposed to a user in any interface, but should otherwise behave as a normal parameter.

Secret params are typically used to hide important state detail that would otherwise be unintelligible to a user, for example the result of a statical analysis that might need many parameters to store.
*/
#define kOfxParamPropSecret "OfxParamPropSecret"

/** @brief The value to be used as the id of the parameter in a host scripting language.

    - Type - ASCII C string X 1,
    - Property Set - plugin parameter descriptor (read/write) and instance (read only),
    - Default - the unique name the parameter was created with.
    - Valid Values - ASCII string unique to all parameters in the plug-in.

Many hosts have a scripting language that they use to set values of parameters and more. If so, this is the name of a parameter in such scripts.
*/
#define kOfxParamPropScriptName "OfxParamPropScriptName"

/** @brief Specifies how modifying the value of a param will affect any output of an effect over time.

   - Type - C string X 1
   - Property Set - plugin parameter descriptor (read/write) and instance (read only),
   - Default - ::kOfxParamInvalidateValueChange
   - Valid Values - This must be one of
       - ::kOfxParamInvalidateValueChange
       - ::kOfxParamInvalidateValueChangeToEnd
       - ::kOfxParamInvalidateAll

Imagine an effect with an animating parameter in a host that caches
rendered output. Think of the what happens when you add a new key frame.
 -If the parameter represents something like an absolute position, the cache will only need to be invalidated for the range of frames that keyframe affects.
- If the parameter represents something like a speed which is integrated, the cache will be invalidated from the keyframe until the end of the clip.
- There are potentially other situations where the entire cache will need to be invalidated (though I can't think of one off the top of my head).
*/
#define kOfxParamPropCacheInvalidation "OfxParamPropCacheInvalidation"

 /** @brief Used as a value for the ::kOfxParamPropCacheInvalidation property */
#define kOfxParamInvalidateValueChange "OfxParamInvalidateValueChange"

 /** @brief Used as a value for the ::kOfxParamPropCacheInvalidation property */
#define kOfxParamInvalidateValueChangeToEnd "OfxParamInvalidateValueChangeToEnd"

 /** @brief Used as a value for the ::kOfxParamPropCacheInvalidation property */
#define kOfxParamInvalidateAll "OfxParamInvalidateAll"

/** @brief A hint to the user as to how the parameter is to be used.

   - Type - UTF8 C string X 1
   - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
   - Default - ""
*/
#define kOfxParamPropHint "OfxParamPropHint"

/** @brief The default value of a parameter.

   - Type - The type is dependant on the parameter type as is the dimension.
   - Property Set - plugin parameter descriptor (read/write) and instance (read/write only),
   - Default - 0 cast to the relevant type (or "" for strings and custom parameters)

The exact type and dimension is dependant on the type of the parameter. These are....
  - ::kOfxParamTypeInteger - integer property of one dimension
  - ::kOfxParamTypeDouble - double property of one dimension
  - ::kOfxParamTypeBoolean - integer property of one dimension
  - ::kOfxParamTypeChoice - integer property of one dimension
  - ::kOfxParamTypeStrChoice - string property of one dimension
  - ::kOfxParamTypeRGBA - double property of four dimensions
  - ::kOfxParamTypeRGB - double property of three dimensions
  - ::kOfxParamTypeDouble2D - double property of two dimensions
  - ::kOfxParamTypeInteger2D - integer property of two dimensions
  - ::kOfxParamTypeDouble3D - double property of three dimensions
  - ::kOfxParamTypeInteger3D - integer property of three dimensions
  - ::kOfxParamTypeString - string property of one dimension
  - ::kOfxParamTypeCustom - string property of one dimension
  - ::kOfxParamTypeBytes - pointer to OfxBytes struct of one dimension, or nullptr
  - ::kOfxParamTypeGroup - does not have this property
  - ::kOfxParamTypePage - does not have this property
  - ::kOfxParamTypePushButton - does not have this property
 */
#define kOfxParamPropDefault "OfxParamPropDefault"

/** @brief Describes how the double parameter should be interpreted by a host. 

   - Type - C string X 1
   - Default - ::kOfxParamDoubleTypePlain
   - Property Set - 1D, 2D and 3D float plugin parameter descriptor (read/write) and instance (read only),
   - Valid Values -This must be one of
      - ::kOfxParamDoubleTypePlain - parameter has no special interpretation,
      - ::kOfxParamDoubleTypeAngle - parameter is to be interpretted as an angle,
      - ::kOfxParamDoubleTypeScale - parameter is to be interpretted as a scale factor,
      - ::kOfxParamDoubleTypeTime  - parameter represents a time value (1D only),
      - ::kOfxParamDoubleTypeAbsoluteTime  - parameter represents an absolute time value (1D only),

      - ::kOfxParamDoubleTypeX - size wrt to the project's X dimension (1D only), in canonical coordinates,
      - ::kOfxParamDoubleTypeXAbsolute - absolute position on the X axis (1D only), in canonical coordinates,
      - ::kOfxParamDoubleTypeY - size wrt to the project's Y dimension(1D only), in canonical coordinates,
      - ::kOfxParamDoubleTypeYAbsolute - absolute position on the Y axis (1D only), in canonical coordinates,
      - ::kOfxParamDoubleTypeXY - size in 2D (2D only), in canonical coordinates,
      - ::kOfxParamDoubleTypeXYAbsolute - an absolute position on the image plane, in canonical coordinates.

Double parameters can be interpreted in several different ways, this property tells the host how to do so and thus gives hints
as to the interface of the parameter.
*/
#define kOfxParamPropDoubleType "OfxParamPropDoubleType"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating the parameter has no special interpretation and should be interpretted as a raw numeric value. */
#define kOfxParamDoubleTypePlain "OfxParamDoubleTypePlain"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating the parameter is to be interpreted as a scale factor. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeScale "OfxParamDoubleTypeScale"

/** @brief value for the ::kOfxParamDoubleTypeAngle property, indicating the parameter is to be interpreted as an angle. See \ref ::kOfxParamPropDoubleType.  */
#define kOfxParamDoubleTypeAngle "OfxParamDoubleTypeAngle"

/** @brief value for the ::kOfxParamDoubleTypeAngle property, indicating the parameter is to be interpreted as a time. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeTime "OfxParamDoubleTypeTime"

/** @brief value for the ::kOfxParamDoubleTypeAngle property, indicating the parameter is to be interpreted as an absolute time from the start of the effect. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeAbsoluteTime "OfxParamDoubleTypeAbsoluteTime"


/** @brief value for the ::kOfxParamPropDoubleType property, indicating a size in canonical coords in the X dimension. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeX  "OfxParamDoubleTypeX"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating a size in canonical coords in the Y dimension. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeY  "OfxParamDoubleTypeY"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating an absolute position in canonical coords in the X dimension. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeXAbsolute  "OfxParamDoubleTypeXAbsolute"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating an absolute position in canonical coords in the Y dimension. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeYAbsolute  "OfxParamDoubleTypeYAbsolute"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating a 2D size in canonical coords. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeXY  "OfxParamDoubleTypeXY"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating a 2D position in canonical coords. See \ref ::kOfxParamPropDoubleType. */
#define kOfxParamDoubleTypeXYAbsolute  "OfxParamDoubleTypeXYAbsolute"

/** @brief Describes in which coordinate system a spatial double parameter's default value is specified.

   - Type - C string X 1
   - Default - kOfxParamCoordinatesCanonical
   - Property Set - Non normalised spatial double parameters, ie: any double param who's ::kOfxParamPropDoubleType is set to one of...
      - kOfxParamDoubleTypeX 
      - kOfxParamDoubleTypeXAbsolute 
      - kOfxParamDoubleTypeY 
      - kOfxParamDoubleTypeYAbsolute 
      - kOfxParamDoubleTypeXY 
      - kOfxParamDoubleTypeXYAbsolute 
   - Valid Values - This must be one of
      - kOfxParamCoordinatesCanonical - the default is in canonical coords
      - kOfxParamCoordinatesNormalised - the default is in normalised coordinates

This allows a spatial param to specify what its default is, so by saying normalised and "0.5" it would be in the 'middle', by saying canonical and 100 it would be at value 100 independent of the size of the image being applied to.
*/
#define kOfxParamPropDefaultCoordinateSystem "OfxParamPropDefaultCoordinateSystem"

/** @brief Define the canonical coordinate system */
#define kOfxParamCoordinatesCanonical "OfxParamCoordinatesCanonical"

/** @brief Define the normalised coordinate system */
#define kOfxParamCoordinatesNormalised "OfxParamCoordinatesNormalised"

/** @brief A flag to indicate if there is a host overlay UI handle for the given parameter.

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read only) 
    - Valid Values - 0 or 1

If set to 1, then the host is flagging that there is some sort of native user overlay interface handle available for the given parameter.
*/
#define kOfxParamPropHasHostOverlayHandle "OfxParamPropHasHostOverlayHandle"

/** @brief A flag to indicate that the host should use a native UI overlay handle for the given parameter.

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write only) and instance (read only)
    - Default - 0
    - Valid Values - 0 or 1

If set to 1, then a plugin is flaging to the host that the host should use a native UI overlay handle for the given parameter. A plugin can use this to keep a native look and feel for parameter handles. A plugin can use ::kOfxParamPropHasHostOverlayHandle to see if handles are available on the given parameter.
*/
#define kOfxParamPropUseHostOverlayHandle "kOfxParamPropUseHostOverlayHandle"


/** @brief Enables the display of a time marker on the host's time line to indicate the value of the absolute time param.

    - Type - int x 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write)
    - Default - 0
    - Valid Values - 0 or 1

If a double parameter is has ::kOfxParamPropDoubleType set to ::kOfxParamDoubleTypeAbsoluteTime, then this indicates whether 
any marker should be made visible on the host's time line.

*/
#define kOfxParamPropShowTimeMarker "OfxParamPropShowTimeMarker"

/** @brief Sets the parameter pages and order of pages.

    - Type - C string X N
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - ""
    - Valid Values - the names of any page param in the plugin

This property sets the preferred order of parameter pages on a host. If this is never set, the preferred order is the order the parameters were declared in.
*/
#define kOfxPluginPropParamPageOrder "OfxPluginPropParamPageOrder"

/** @brief The names of the parameters included in a page parameter.

    - Type - C string X N
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - ""
    - Valid Values - the names of any parameter that is not a group or page, as well as ::kOfxParamPageSkipRow and ::kOfxParamPageSkipColumn

This is a property on parameters of type ::kOfxParamTypePage, and tells the page what parameters it contains. The parameters are added to the page from the top left, filling in columns as we go. The two pseudo param names ::kOfxParamPageSkipRow and ::kOfxParamPageSkipColumn are used to control layout.

Note parameters can appear in more than one page.
*/
#define kOfxParamPropPageChild "OfxParamPropPageChild"

/** @brief The name of a parameter's parent group.

    - Type - C string X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only),
    - Default - "", which implies the "root" of the hierarchy,
    - Valid Values - the name of a parameter with type of ::kOfxParamTypeGroup

Hosts that have hierarchical layouts of their params use this to recursively group parameter.

By default parameters are added in order of declaration to the 'root' hierarchy. This property is used to reparent params to a predefined param of type ::kOfxParamTypeGroup.
*/
#define kOfxParamPropParent "OfxParamPropParent"

/** @brief Whether the initial state of a group is open or closed in a hierarchical layout. 

    - Type - int X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 1
    - Valid Values - 0 or 1

This is a property on parameters of type ::kOfxParamTypeGroup, and tells the group whether it should be open or closed by default.

*/
#define kOfxParamPropGroupOpen "OfxParamPropGroupOpen"

/** @brief Used to enable a parameter in the user interface.

    - Type - int X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - 1
    - Valid Values - 0 or 1

When set to 0 a user should not be able to modify the value of the parameter. Note that the plug-in itself can still change the value of a disabled parameter.
*/
#define kOfxParamPropEnabled "OfxParamPropEnabled"

/** @brief A private data pointer that the plug-in can store its own data behind.

    - Type - pointer X 1
    - Property Set - plugin parameter instance (read/write),
    - Default - NULL

This data pointer is unique to each parameter instance, so two instances of the same parameter do not share the same data pointer. Use it to hang any needed private data structures.
 */
#define kOfxParamPropDataPtr "OfxParamPropDataPtr"

/** @brief Set options of a choice parameter.

    - Type - UTF8 C string X N
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - the property is empty with no options set.

This property contains the set of options that will be presented to a user
from a choice parameter. See @ref ParametersChoice for more details.
*/
#define kOfxParamPropChoiceOption "OfxParamPropChoiceOption"

/** @brief Set values the host should store for a choice parameter.

    - Type - int X N
    - Property Set - plugin parameter descriptor (read/write) and instance
(read/write),
    - Default - Zero-based ordinal list of same length as
`OfxParamPropChoiceOption`

This property specifies the order in which the options are presented.
See @ref "Choice Parameters" for more details.
This property is optional; if not set, the host will present the options in
their natural order.

This property is useful when changing order of choice param options, or adding
new options in the middle, in a new version of the plugin.

  @verbatim
  Plugin v1:
  Option = {"OptA", "OptB", "OptC"}
  Order = {1, 2, 3}

  Plugin v2:
  // will be shown as OptA / OptB / NewOpt / OptC
  Option = {"OptA", "OptB", "OptC", NewOpt"}
  Order = {1, 2, 4, 3}
  @endverbatim

Note that this only affects the host UI's display order; the project still
stores the index of the selected option as always. Plugins should never
reorder existing options if they desire backward compatibility.

Values may be arbitrary 32-bit integers. Behavior is undefined if the same
value occurs twice in the list; plugins should not do that.

\since Version 1.5
*/
#define kOfxParamPropChoiceOrder "OfxParamPropChoiceOrder"


/** @brief Set a enumeration string in a StrChoice (string-valued choice) parameter.

    - Type - UTF8 C string X N
    - Property Set - plugin parameter descriptor (read/write) and instance
(read/write),
    - Default - the property is empty with no options set.

This property contains the set of enumeration strings stored by the host in
the project corresponding to the options that will be presented to a user
from a StrChoice parameter. See @ref ParametersChoice for more details.

\since Version 1.5
*/
#define kOfxParamPropChoiceEnum "OfxParamPropChoiceEnum"

/** @brief Indicates if the host supports animation of string choice params.

    - Type - int X 1
    - Property Set - host descriptor (read only)
    - Valid Values - 0 or 1

\since Version 1.5
*/
#define kOfxParamHostPropSupportsStrChoiceAnimation "OfxParamHostPropSupportsStrChoiceAnimation"

/** @brief Indicates if the host supports the StrChoice param type.

    - Type - int X 1
    - Property Set - host descriptor (read only)
    - Valid Values - 0 or 1

\since Version 1.5
*/
#define kOfxParamHostPropSupportsStrChoice "OfxParamHostPropSupportsStrChoice"

/** @brief The minimum value for a numeric parameter.

    - Type - int or double X N
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - the smallest possible value corresponding to the parameter type (eg: INT_MIN for an integer, -DBL_MAX for a double parameter)

Setting this will also reset ::kOfxParamPropDisplayMin.
*/
#define kOfxParamPropMin "OfxParamPropMin"

/** @brief The maximum value for a numeric parameter.

    - Type - int or double X N
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - the largest possible value corresponding to the parameter type (eg: INT_MAX for an integer, DBL_MAX for a double parameter)

Setting this will also reset :;kOfxParamPropDisplayMax.
*/
#define kOfxParamPropMax "OfxParamPropMax"

/** @brief The minimum value for a numeric parameter on any user interface.

    - Type - int or double X N
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - the smallest possible value corresponding to the parameter type (eg: INT_MIN for an integer, -DBL_MAX for a double parameter)

If a user interface represents a parameter with a slider or similar, this should be the minumum bound on that slider.
*/
#define kOfxParamPropDisplayMin "OfxParamPropDisplayMin"

/** @brief The maximum value for a numeric parameter on any user interface.

    - Type - int or double X N
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - the largest possible value corresponding to the parameter type (eg: INT_MAX for an integer, DBL_MAX for a double parameter)

If a user interface represents a parameter with a slider or similar, this should be the maximum bound on that slider.
*/
#define kOfxParamPropDisplayMax "OfxParamPropDisplayMax"

/** @brief The granularity of a slider used to represent a numeric parameter.

    - Type - double X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - 1
    - Valid Values - any greater than 0.

This value is always in canonical coordinates for double parameters that are normalised.
*/
#define kOfxParamPropIncrement "OfxParamPropIncrement"

/** @brief How many digits after a decimal point to display for a double param in a GUI.

    - Type - int X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
    - Default - 2
    - Valid Values - any greater than 0.

This applies to double params of any dimension.
*/
#define kOfxParamPropDigits "OfxParamPropDigits"

/** @brief Label for individual dimensions on a multidimensional numeric parameter.

    - Type - UTF8 C string X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only),
    - Default - "x", "y" and "z"
    - Valid Values - any

Use this on 2D and 3D double and integer parameters to change the label on an individual dimension in any GUI for that parameter.
*/
#define kOfxParamPropDimensionLabel "OfxParamPropDimensionLabel"

/** @brief Will a value change on the parameter add automatic keyframes.

    - Type - int X 1
    - Property Set - plugin parameter instance (read only),
    - Valid Values - 0 or 1

This is set by the host simply to indicate the state of the property.
*/
#define kOfxParamPropIsAutoKeying "OfxParamPropIsAutoKeying"

/** @brief A pointer to a custom parameter's interpolation function.

    - Type - pointer X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only),
    - Default - NULL
    - Valid Values - must point to a ::OfxCustomParamInterpFuncV1

It is an error not to set this property in a custom parameter during a plugin's define call if the custom parameter declares itself to be an animating parameter.
 */
#define kOfxParamPropCustomInterpCallbackV1 "OfxParamPropCustomCallbackV1"

/** @brief Used to indicate the type of a string parameter.

    - Type - C string X 1
    - Property Set - plugin string parameter descriptor (read/write) and instance (read only),
    - Default - ::kOfxParamStringIsSingleLine
    - Valid Values - This must be one of the following
        - ::kOfxParamStringIsSingleLine
        - ::kOfxParamStringIsMultiLine
        - ::kOfxParamStringIsFilePath
        - ::kOfxParamStringIsDirectoryPath
        - ::kOfxParamStringIsLabel
        - ::kOfxParamStringIsRichTextFormat

*/
#define kOfxParamPropStringMode "OfxParamPropStringMode"

/** @brief Indicates string parameters of file or directory type need that file to exist already.

    - Type - int X 1
    - Property Set - plugin string parameter descriptor (read/write) and instance (read only),
    - Default - 1
    - Valid Values - 0 or 1

If set to 0, it implies the user can specify a new file name, not just a pre-existing one.
 */
#define kOfxParamPropStringFilePathExists    "OfxParamPropStringFilePathExists"

/** @brief Used to set a string parameter to be single line, 
    value to be passed to a ::kOfxParamPropStringMode property */
#define kOfxParamStringIsSingleLine    "OfxParamStringIsSingleLine"

/** @brief Used to set a string parameter to be multiple line, 
    value to be passed to a ::kOfxParamPropStringMode property */
#define kOfxParamStringIsMultiLine     "OfxParamStringIsMultiLine"

/** @brief Used to set a string parameter to be a file path,
    value to be passed to a ::kOfxParamPropStringMode property */
#define kOfxParamStringIsFilePath      "OfxParamStringIsFilePath"

/** @brief Used to set a string parameter to be a directory path,
    value to be passed to a ::kOfxParamPropStringMode property */
#define kOfxParamStringIsDirectoryPath "OfxParamStringIsDirectoryPath"

/** @brief Use to set a string parameter to be a simple label, 
    value to be passed to a ::kOfxParamPropStringMode property  */
#define kOfxParamStringIsLabel         "OfxParamStringIsLabel"

  /** @brief String value on the ::kOfxParamPropStringMode property of a
    string parameter (added in 1.3) */
#define kOfxParamStringIsRichTextFormat "OfxParamStringIsRichTextFormat"

/** @brief Used by interpolating custom parameters to get and set interpolated values.
    - Type - C string X 1 or 2

This property is on the \e inArgs property and \e outArgs property of a ::OfxCustomParamInterpFuncV1 and in both cases contains the encoded value of a custom parameter. As an \e inArgs property it will have two values, being the two keyframes to interpolate. As an \e outArgs property it will have a single value and the plugin should fill this with the encoded interpolated value of the parameter.
 */
#define kOfxParamPropCustomValue "OfxParamPropCustomValue"

/** @brief Used by interpolating custom parameters to indicate the time a key occurs at.

   - Type - double X 2
   - Property Set - inArgs parameter of a ::OfxCustomParamInterpFuncV1 (read only)

The two values indicate the absolute times the surrounding keyframes occur at. The keyframes are encoded in a ::kOfxParamPropCustomValue property.

 */
#define kOfxParamPropInterpolationTime "OfxParamPropInterpolationTime"

/** @brief Property used by ::OfxCustomParamInterpFuncV1 to indicate the amount of interpolation to perform

   - Type - double X 1
   - Property Set - inArgs parameter of a ::OfxCustomParamInterpFuncV1 (read only)
   - Valid Values - from 0 to 1

This property indicates how far between the two ::kOfxParamPropCustomValue keys to interpolate.
 */
#define kOfxParamPropInterpolationAmount "OfxParamPropInterpolationAmount"

/*@}*/
/*@}*/

/** @brief Function prototype for custom parameter interpolation callback functions

  \arg \c instance    the plugin instance that this parameter occurs in
  \arg \c inArgs      handle holding the following properties...
    - kOfxPropName - the name of the custom parameter to interpolate
    - kOfxPropTime - absolute time the interpolation is ocurring at
    - kOfxParamPropCustomValue - string property that gives the value of the two keyframes to interpolate, in this case 2D
    - kOfxParamPropInterpolationTime - 2D double property that gives the time of the two keyframes we are interpolating
    - kOfxParamPropInterpolationAmount - 1D double property indicating how much to interpolate between the two keyframes

  \arg \c outArgs     handle holding the following properties to be set
    - kOfxParamPropCustomValue - the value of the interpolated custom parameter, in this case 1D

This function allows custom parameters to animate by performing interpolation between keys.

The plugin needs to parse the two strings encoding keyframes on either side of the time 
we need a value for. It should then interpolate a new value for it, encode it into a string and set
the ::kOfxParamPropCustomValue property with this on the outArgs handle.

The interp value is a linear interpolation amount, however his may be derived from a cubic (or other) curve.
*/
typedef OfxStatus (OfxCustomParamInterpFuncV1)(OfxParamSetHandle instance,
					       OfxPropertySetHandle inArgs,
					       OfxPropertySetHandle outArgs);


/** @brief The OFX suite used to define and manipulate user visible parameters 
 */
typedef struct OfxParameterSuiteV1 {
  /** @brief Defines a new parameter of the given type in a describe action

  \arg \c paramSet    handle to the parameter set descriptor that will hold this parameter
  \arg \c paramType   type of the parameter to create, one of the kOfxParamType* #defines
  \arg \c name        unique name of the parameter
  \arg \c propertySet if not null, a pointer to the parameter descriptor's property set will be placed here.

  This function defines a parameter in a parameter set and returns a property set which is used to describe that parameter.

  This function does not actually create a parameter, it only says that one should exist in any subsequent instances. To fetch an
  parameter instance paramGetHandle must be called on an instance.

  This function can always be called in one of a plug-in's "describe" functions which defines the parameter sets common to all instances of a plugin.

@returns
  - ::kOfxStatOK             - the parameter was created correctly
  - ::kOfxStatErrBadHandle   - if the plugin handle was invalid
  - ::kOfxStatErrExists      - if a parameter of that name exists already in this plugin
  - ::kOfxStatErrUnknown     - if the type is unknown
  - ::kOfxStatErrUnsupported - if the type is known but unsupported
  */
  OfxStatus (*paramDefine)(OfxParamSetHandle paramSet,
			   const char *paramType,
			   const char *name,
			   OfxPropertySetHandle *propertySet);

  /** @brief Retrieves the handle for a parameter in a given parameter set

  \arg \c paramSet    instance of the plug-in to fetch the property handle from
  \arg \c name        parameter to ask about
  \arg \c param       pointer to a param handle, the value is returned here
  \arg \c propertySet if not null, a pointer to the parameter's property set will be placed here.

  Parameter handles retrieved from an instance are always distinct in each instance. The paramter handle is valid for the life-time of the instance. Parameter handles in instances are distinct from paramter handles in plugins. You cannot call this in a plugin's describe function, as it needs an instance to work on.

@returns
  - ::kOfxStatOK       - the parameter was found and returned
  - ::kOfxStatErrBadHandle  - if the plugin handle was invalid
  - ::kOfxStatErrUnknown    - if the type is unknown
  */
  OfxStatus (*paramGetHandle)(OfxParamSetHandle paramSet,
			      const char *name,
			      OfxParamHandle *param,
			      OfxPropertySetHandle *propertySet);

  /** @brief Retrieves the property set handle for the given parameter set

  \arg \c paramSet    parameter set to get the property set for
  \arg \c propHandle  pointer to a the property set handle, value is returedn her

  \note The property handle belonging to a parameter set is the same as the property handle belonging to the plugin instance.

@returns
  - ::kOfxStatOK       - the property set was found and returned
  - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
  - ::kOfxStatErrUnknown    - if the type is unknown
  */
  OfxStatus (*paramSetGetPropertySet)(OfxParamSetHandle paramSet,
				      OfxPropertySetHandle *propHandle);

  /** @brief Retrieves the property set handle for the given parameter

  \arg \c param       parameter to get the property set for
  \arg \c propHandle  pointer to a the property set handle, value is returedn her

  The property handle is valid for the lifetime of the parameter, which is the lifetime of the instance that owns the parameter

@returns
  - ::kOfxStatOK       - the property set was found and returned
  - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
  - ::kOfxStatErrUnknown    - if the type is unknown
  */
  OfxStatus (*paramGetPropertySet)(OfxParamHandle param,
				   OfxPropertySetHandle *propHandle);

  /** @brief Gets the current value of a parameter,

  \arg \c paramHandle parameter handle to fetch value from
  \arg \c ...         one or more pointers to variables of the relevant type to hold the parameter's value

  This gets the current value of a parameter. The varargs ... argument needs to be pointer to C variables
  of the relevant type for this parameter. Note that params with multiple values (eg Colour) take
  multiple args here. For example...

  @verbatim
  OfxParamHandle myDoubleParam, *myColourParam;
  ofxHost->paramGetHandle(instance, "myDoubleParam", &myDoubleParam);
  double myDoubleValue;
  ofxHost->paramGetValue(myDoubleParam, &myDoubleValue);
  ofxHost->paramGetHandle(instance, "myColourParam", &myColourParam);
  double myR, myG, myB;
  ofxHost->paramGetValue(myColourParam, &myR, &myG, &myB);
  @endverbatim

  \note \c paramGetValue should only be called from within a ::kOfxActionInstanceChanged or interact action and never from the render actions (which should always use paramGetValueAtTime).

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramGetValue)(OfxParamHandle  paramHandle,
			     ...);


  /** @brief Gets the value of a parameter at a specific time.

  \arg \c paramHandle parameter handle to fetch value from
  \arg \c time        at what point in time to look up the parameter
  \arg \c ...         one or more pointers to variables of the relevant type to hold the parameter's value

  This gets the current value of a parameter. The varargs needs to be pointer to C variables
  of the relevant type for this parameter. See OfxParameterSuiteV1::paramGetValue for notes on
  the varags list

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramGetValueAtTime)(OfxParamHandle  paramHandle,
				   OfxTime time,
				   ...);

  /** @brief Gets the derivative of a parameter at a specific time.

  \arg \c paramHandle parameter handle to fetch value from
  \arg \c time        at what point in time to look up the parameter
  \arg \c ...         one or more pointers to variables of the relevant type to hold the parameter's derivative

  This gets the derivative of the parameter at the indicated time. 

  The varargs needs to be pointer to C variables
  of the relevant type for this parameter. See OfxParameterSuiteV1::paramGetValue for notes on
  the varags list.

  Only double and colour params can have their derivatives found.

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramGetDerivative)(OfxParamHandle  paramHandle,
				  OfxTime time,
				  ...);

  /** @brief Gets the integral of a parameter over a specific time range,

  \arg \c paramHandle parameter handle to fetch integral from
  \arg \c time1       where to start evaluating the integral
  \arg \c time2       where to stop evaluating the integral
  \arg \c ...         one or more pointers to variables of the relevant type to hold the parameter's integral

  This gets the integral of the parameter over the specified time range.

  The varargs needs to be pointer to C variables
  of the relevant type for this parameter. See OfxParameterSuiteV1::paramGetValue for notes on
  the varags list.

  Only double and colour params can be integrated.

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramGetIntegral)(OfxParamHandle  paramHandle,
				OfxTime time1, OfxTime time2,
				...);

  /** @brief Sets the current value of a parameter

  \arg \c paramHandle parameter handle to set value in
  \arg \c ...         one or more variables of the relevant type to hold the parameter's value

  This sets the current value of a parameter. The varargs ... argument needs to be values
  of the relevant type for this parameter. Note that params with multiple values (eg Colour) take
  multiple args here. For example...
  @verbatim
  ofxHost->paramSetValue(instance, "myDoubleParam", double(10));
  ofxHost->paramSetValue(instance, "myColourParam", double(pix.r), double(pix.g), double(pix.b));
  @endverbatim

  \note \c paramSetValue should only be called from within a ::kOfxActionInstanceChanged or interact action.

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramSetValue)(OfxParamHandle  paramHandle,
			     ...);

  /** @brief Keyframes the value of a parameter at a specific time.

  \arg \c paramHandle parameter handle to set value in
  \arg \c time        at what point in time to set the keyframe
  \arg \c ...         one or more variables of the relevant type to hold the parameter's value

  This sets a keyframe in the parameter at the indicated time to have the indicated value.
  The varargs ... argument needs to be values of the relevant type for this parameter. See the note on 
  OfxParameterSuiteV1::paramSetValue for more detail

  \note \c paramSetValueAtTime should only be called from within a ::kOfxActionInstanceChanged or interact action.

  V1.3: This function can be called the ::kOfxActionInstanceChanged action and during image effect analysis render passes.
  V1.4: This function can be called the ::kOfxActionInstanceChanged action 
@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramSetValueAtTime)(OfxParamHandle  paramHandle,
				   OfxTime time,  // time in frames
				   ...);


/** @name Keyframe Handling

These functions allow the plug-in to delete and get information about keyframes.

To set keyframes, use paramSetValueAtTime().

paramGetKeyTime and paramGetKeyIndex use indices to refer to keyframes.
Keyframes are stored by the host in increasing time order, so time(kf[i]) < time(kf[i+1]).
Keyframe indices will change whenever keyframes are added, deleted, or moved in time,
whether by the host or by the plug-in.  They may vary between actions if the user
changes a keyframe.  The keyframe indices will not change within a single action.
 */
/** @{ */

  /** @brief Returns the number of keyframes in the parameter

  \arg \c paramHandle parameter handle to interrogate
  \arg \c numberOfKeys pointer to integer where the return value is placed

  V1.3: This function can be called the ::kOfxActionInstanceChanged action and during image effect analysis render passes.
  V1.4: This function can be called the ::kOfxActionInstanceChanged action 

  Returns the number of keyframes in the parameter.

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramGetNumKeys)(OfxParamHandle  paramHandle,
			       unsigned int  *numberOfKeys);

  /** @brief Returns the time of the nth key

  \arg \c paramHandle parameter handle to interrogate
  \arg \c nthKey      which key to ask about (0 to paramGetNumKeys -1), ordered by time
  \arg \c time	   pointer to OfxTime where the return value is placed

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  - ::kOfxStatErrBadIndex   - the nthKey does not exist
  */
  OfxStatus (*paramGetKeyTime)(OfxParamHandle  paramHandle,
			       unsigned int nthKey,
			       OfxTime *time);


  /** @brief Finds the index of a keyframe at/before/after a specified time.

  \arg \c paramHandle parameter handle to search
  \arg \c time        what time to search from
  \arg \c direction
    - == 0 indicates search for a key at the indicated time (some small delta)
    - > 0 indicates search for the next key after the indicated time
    - < 0 indicates search for the previous key before the indicated time
  \arg \c index	   pointer to an integer which in which the index is returned set to -1 if no key was found

@returns
  - ::kOfxStatOK            - all was OK
  - ::kOfxStatFailed        - if the search failed to find a key
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramGetKeyIndex)(OfxParamHandle  paramHandle,
				OfxTime time,
				int     direction,
				int    *index);

  /** @brief Deletes a keyframe if one exists at the given time.

  \arg \c paramHandle parameter handle to delete the key from
  \arg \c time        time at which a keyframe is

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  - ::kOfxStatErrBadIndex   - no key at the given time
  */
  OfxStatus (*paramDeleteKey)(OfxParamHandle  paramHandle,
			      OfxTime time);

  /** @brief Deletes all keyframes from a parameter.

  \arg \c paramHandle parameter handle to delete the keys from
  \arg \c name        parameter to delete the keyframes frome is

  V1.3: This function can be called the ::kOfxActionInstanceChanged action and during image effect analysis render passes.
  V1.4: This function can be called the ::kOfxActionInstanceChanged action 

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramDeleteAllKeys)(OfxParamHandle  paramHandle);

/** @} */

  /** @brief Copies one parameter to another, including any animation etc...

  \arg \c paramTo     parameter to set
  \arg \c paramFrom   parameter to copy from
  \arg \c dstOffset   temporal offset to apply to keys when writing to the paramTo
  \arg \c frameRange  if paramFrom has animation, and frameRange is not null, only this range of keys will be copied

  This copies the value of \e paramFrom to \e paramTo, including any animation it may have. All the previous values in \e paramTo will be lost.

  To choose all animation in \e paramFrom set \e frameRange to [0, 0]

   V1.3: This function can be called the ::kOfxActionInstanceChanged action and during image effect analysis render passes.
  V1.4: This function can be called the ::kOfxActionInstanceChanged action 

  \pre
  - Both parameters must be of the same type.

  \return
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the parameter handle was invalid
  */
  OfxStatus (*paramCopy)(OfxParamHandle  paramTo, OfxParamHandle  paramFrom, OfxTime dstOffset, const OfxRangeD *frameRange);


  /** @brief Used to group any parameter changes for undo/redo purposes

  \arg \c paramSet    the parameter set in which this is happening
  \arg \c name        label to attach to any undo/redo string UTF8

  If a plugin calls paramSetValue/paramSetValueAtTime on one or more parameters, either from custom GUI interaction
  or some analysis of imagery etc.. this is used to indicate the start of a set of a parameter
  changes that should be considered part of a single undo/redo block.

  \note \c paramEditBegin should only be called from within a ::kOfxActionInstanceChanged or interact action.

  See also OfxParameterSuiteV1::paramEditEnd

  \return
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the instance handle was invalid

  */
  OfxStatus (*paramEditBegin)(OfxParamSetHandle paramSet, const char *name); 

  /** @brief Used to group any parameter changes for undo/redo purposes

  \arg \c paramSet    parameter set in which this is happening

  If a plugin calls paramSetValue/paramSetValueAtTime on one or more parameters, either from custom GUI interaction
  or some analysis of imagery etc.. this is used to indicate the end of a set of parameter
  changes that should be considerred part of a single undo/redo block

  \note \c paramEditEnd should only be called from within a ::kOfxActionInstanceChanged or interact action.

  See also OfxParameterSuiteV1::paramEditBegin

@returns
  - ::kOfxStatOK       - all was OK
  - ::kOfxStatErrBadHandle  - if the instance handle was invalid

  */
  OfxStatus (*paramEditEnd)(OfxParamSetHandle paramSet);
 } OfxParameterSuiteV1;

#ifdef __cplusplus
}
#endif


/** @file ofxParam.h
 
  This header contains the suite definition to manipulate host side parameters.

  For more details go see @ref ParametersPage
*/


#endif
