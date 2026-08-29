

#ifndef _ofxCore_h_
#define _ofxCore_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


#include "stddef.h" // for size_t
#include <limits.h> // for INT_MIN & INT_MAX

#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxCore.h
Contains the core OFX architectural struct and function definitions. For more details on the basic OFX architecture, see \ref Architecture.
*/


/** @brief Platform independent export macro.
 *
 * This macro is to be used before any symbol that is to be
 * exported from a plug-in. This is OS/compiler dependent.
 */
#if defined(WIN32) || defined(WIN64)
	#define OfxExport extern __declspec(dllexport)
#else
	#define OfxExport extern
#endif

/** @brief Blind data structure to manipulate sets of properties through */
typedef struct OfxPropertySetStruct *OfxPropertySetHandle;

/** @brief OFX status return type */
typedef int OfxStatus;

/** @brief Generic host structure passed to OfxPlugin::setHost function

    This structure contains what is needed by a plug-in to bootstrap its connection
    to the host.
*/
typedef struct OfxHost {
  /** @brief Global handle to the host. Extract relevant host properties from this.
      This pointer will be valid while the binary containing the plug-in is loaded.
   */
  OfxPropertySetHandle host;

  /** @brief The function which the plug-in uses to fetch suites from the host.

      \arg \c host          the host the suite is being fetched from this \em must be the \e host member of the OfxHost struct containing fetchSuite.
      \arg \c suiteName     ASCII string labelling the host supplied API
      \arg \c suiteVersion  version of that suite to fetch

      Any API fetched will be valid while the binary containing the plug-in is loaded.

      Repeated calls to fetchSuite with the same parameters will return the same pointer.

      It is recommended that hosts should return the same host and suite pointers to all plugins
      in the same shared lib or bundle.

      returns
         - NULL if the API is unknown (either the api or the version requested),
	 - pointer to the relevant API if it was found
  */
  const void *(*fetchSuite)(OfxPropertySetHandle host, const char *suiteName, int suiteVersion);
} OfxHost;


/** @brief Entry point for plug-ins

  \arg \c action   ASCII c string indicating which action to take
  \arg \c instance object to which action should be applied, this will need to be cast to the appropriate blind data type depending on the \e action
  \arg \c inData   handle that contains action specific properties
  \arg \c outData  handle where the plug-in should set various action specific properties

  This is how the host generally communicates with a plug-in. Entry points are used to pass messages
  to various objects used within OFX. The main use is within the OfxPlugin struct.

  The exact set of actions is determined by the plug-in API that is being implemented, however all plug-ins
  can perform several actions. For the list of actions consult \ref ActionsAll.
 */
typedef  OfxStatus (OfxPluginEntryPoint)(const char *action, const void *handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);

/** @brief The structure that defines a plug-in to a host.
 *
 * This structure is the first element in any plug-in structure
 * using the OFX plug-in architecture. By examining its members
 * a host can determine the API that the plug-in implements,
 * the version of that API, its name and version.
 *
 * For details see \ref Architecture.
 *
 */
typedef struct OfxPlugin {
  /** Defines the type of the plug-in, this will tell the host what the plug-in does. e.g.: an image
      effects plug-in would be a "OfxImageEffectPlugin"
   */
  const char		*pluginApi;

  /** Defines the version of the pluginApi that this plug-in implements */
  int            apiVersion;

  /** String that uniquely labels the plug-in among all plug-ins that implement an API.
      It need not necessarily be human sensible, however the preference is to use reverse
      internet domain name of the developer, followed by a '.' then by a name that represents
      the plug-in.. It must be a legal ASCII string and have no whitespace in the
      name and no non printing chars.
      For example "uk.co.somesoftwarehouse.myPlugin"
  */
  const char 		*pluginIdentifier;

  /** Major version of this plug-in, this gets incremented when backwards compatibility is broken. */
  unsigned int 	 pluginVersionMajor;

  /**  Major version of this plug-in, this gets incremented when software is changed,
       but does not break backwards compatibility. */
  unsigned int   pluginVersionMinor;

  /** @brief Function the host uses to connect the plug-in to the host's api fetcher

      \arg \c fetchApi pointer to host's API fetcher

      Mandatory function.

      The very first function called in a plug-in. The plug-in \em must \em not call any OFX functions within this, it must only set its local copy of the host pointer.

      \pre
        - nothing else has been called

      \post
        - the pointer suite is valid until the plug-in is unloaded

      It is recommended that hosts should return the same host and suite pointers to all plugins
      in the same shared lib or bundle.
  */
  void     (*setHost)(OfxHost *host);

  /** @brief Main entry point for plug-ins

  Mandatory function.

  The exact set of actions is determined by the plug-in API that is being implemented, however all plug-ins
  can perform several actions. For the list of actions consult \ref ActionsAll.

   Preconditions
      - setHost has been called
   */
  OfxPluginEntryPoint *mainEntry;
} OfxPlugin;

/**
   \defgroup ActionsAll OFX Actions

These are the actions passed to a plug-in's 'main' function
*/
/*@{*/

/** @brief

 This action is the first action passed to a plug-in after the
 binary containing the plug-in has been loaded. It is there to allow a
 plug-in to create any global data structures it may need and is also
 when the plug-in should fetch suites from the host.

 The \ref handle, \ref inArgs and \ref outArgs arguments to the \ref mainEntry
 are redundant and should be set to NULL.

 

 \pre
 - The plugin's \ref OfxPlugin::setHost function has been called

 \post
 This action will not be called again while the binary containing the plug-in remains loaded.

 @returns
 -  \ref kOfxStatOK, the action was trapped and all was well,
 -  \ref kOfxStatReplyDefault, the action was ignored,
 -  \ref kOfxStatFailed, the load action failed, no further actions will be passed to the plug-in.
 Interpret if possible  kOfxStatFailed as plug-in indicating it does not want to load 
 Do not create an entry in the host's UI for plug-in then.  
 Plug-in also has the option to return 0 for OfxGetNumberOfPlugins or kOfxStatFailed if host supports OfxSetHost in which case kOfxActionLoad will never be called.
 -  \ref kOfxStatErrFatal, fatal error in the plug-in.
 */
#define  kOfxActionLoad "OfxActionLoad"

/** @brief

 The kOfxActionDescribe is the second action passed to a plug-in. It is
 where a plugin defines how it behaves and the resources it needs to
 function.

 Note that the handle passed in acts as a descriptor for, rather than an
 instance of the plugin. The handle is global and unique. The plug-in is
 at liberty to cache the handle away for future reference until the
 plug-in is unloaded.

 Most importantly, the effect must set what image effect contexts it is
 capable of working in.

 This action *must* be trapped, it is not optional.


 @param handle handle to the plug-in descriptor, cast to an \ref OfxImageEffectHandle
 @param inArgs is redundant and is set to NULL
 @param outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionLoad has been called

 \post
     -  \ref kOfxActionDescribe will not be called again, unless it fails and
     returns one of the error codes where the host is allowed to attempt
     the action again
     -  the handle argument, being the global plug-in description handle, is
     a valid handle from the end of a sucessful describe action until the
     end of the \ref kOfxActionUnload action (ie: the plug-in can cache it away
     without worrying about it changing between actions).
     -  \ref kOfxImageEffectActionDescribeInContext
     will be called once for each context that the host and plug-in
     mutually support.  If a plug-in does not report to support any context supported by host, 
	 host should not enable the plug-in.

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatErrMissingHostFeature, in which the plugin will be unloaded
     and ignored, plugin may post message
     -  \ref kOfxStatErrMemory, in which case describe may be called again after a
     memory purge
     -  \ref kOfxStatFailed, something wrong, but no error code appropriate,
     plugin to post message
     -  \ref kOfxStatErrFatal

 */
#define kOfxActionDescribe "OfxActionDescribe"

/** @brief

 This action is the last action passed to the plug-in before the
 binary containing the plug-in is unloaded. It is there to allow a
 plug-in to destroy any global data structures it may have created.

 The handle, inArgs and outArgs arguments to the main entry
 are redundant and should be set to NULL.

 \pre
     -  the \ref kOfxActionLoad action has been called
     -  all instances of a plugin have been destroyed

 \post
     - No other actions will be called.

 @returns
     -  \ref kOfxStatOK, the action was trapped all was well
     -  \ref kOfxStatReplyDefault, the action was ignored
     -  \ref kOfxStatErrFatal, in which case we the program will be forced to quit

 */
#define kOfxActionUnload "OfxActionUnload"

/** @brief

 This action is an action that may be passed to a plug-in
 instance from time to time in low memory situations. Instances recieving
 this action should destroy any data structures they may have and release
 the associated memory, they can later reconstruct this from the effect's
 parameter set and associated information.

 For Image Effects, it is generally a bad idea to call this after each
 render, but rather it should be called after
 \ref kOfxImageEffectActionEndSequenceRender
 Some effects, typically those flagged with the
 \ref kOfxImageEffectInstancePropSequentialRender
 property, may need to cache information from previously rendered frames
 to function correctly, or have data structures that are expensive to
 reconstruct at each frame (eg: a particle system). Ideally, such effect
 should free such structures during the
 \ref kOfxImageEffectActionEndSequenceRender action.

 @param  handle handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionCreateInstance has been called on the instance handle,

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatReplyDefault, the action was ignored
     -  \ref kOfxStatErrFatal,
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message
 */
#define kOfxActionPurgeCaches                 "OfxActionPurgeCaches"

/** @brief

 This action is called when a plugin should synchronise any private data
 structures to its parameter set. This generally occurs when an effect is
 about to be saved or copied, but it could occur in other situations as
 well.

 @param  handle handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL

 \pre
     - \ref kOfxActionCreateInstance has been called on the instance handle,

 \post
     -  Any private state data can be reconstructed from the parameter set,

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatReplyDefault, the action was ignored
     -  \ref kOfxStatErrFatal,
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message
 */
#define kOfxActionSyncPrivateData                 "OfxActionSyncPrivateData"

/** @brief

 This action is the first action passed to a plug-in's
 instance after its creation. It is there to allow a plugin to create any
 per-instance data structures it may need.

 @param  handle handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionDescribe has been called
     -  the instance is fully constructed, with all objects requested in the
     describe actions (eg, parameters and clips) have been constructed and
     have had their initial values set. This means that if the values are
     being loaded from an old setup, that load should have taken place
     before the create instance action is called.

 \post
     -  the instance pointer will be valid until the
     \ref kOfxActionDestroyInstance
     action is passed to the plug-in with the same instance handle

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatReplyDefault, the action was ignored, but all was well anyway
     -  \ref kOfxStatErrFatal
     -  \ref kOfxStatErrMemory, in which case this may be called again after a
     memory purge
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message if possible and the host should
     destroy the instanace handle and not attempt to proceed further
 */
#define kOfxActionCreateInstance        "OfxActionCreateInstance"

/** @brief


 This action is the last passed to a plug-in's instance before its
 destruction. It is there to allow a plugin to destroy any per-instance
 data structures it may have created.

 @param  handle
 handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionCreateInstance
     has been called on the handle,
     -  the instance has not had any of its members destroyed yet,

 \post
     -  the instance pointer is no longer valid and any operation on it will
     be undefined

 @returns
     To some extent, what is returned is moot, a bit like throwing an
     exception in a C++ destructor, so the host should continue destruction
     of the instance regardless.

     -  \ref kOfxStatOK, the action was trapped and all was well,
     -  \ref kOfxStatReplyDefault, the action was ignored as the effect had nothing
     to do,
     -  \ref kOfxStatErrFatal,
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message.

 */
#define kOfxActionDestroyInstance       "OfxActionDestroyInstance"

/** @brief

 This action signals that something has changed in a plugin's instance,
 either by user action, the host or the plugin itself. All change actions
 are bracketed by a pair of \ref kOfxActionBeginInstanceChanged and
 \ref kOfxActionEndInstanceChanged actions. The ``inArgs`` property set is
 used to determine what was the thing inside the instance that was
 changed.

 @param  handle handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs has the following properties
     - \ref  kOfxPropType The type of the thing that changed which will be one of..

     - \ref kOfxTypeParameter Indicating a parameter's value has changed
     in some way
     -  \ref kOfxTypeClip A clip to an image effect has changed in some
     way (for Image Effect Plugins only)

     -  \ref kOfxPropName the name of the thing that was changed in the instance
     -  \ref kOfxPropChangeReason what triggered the change, which will be one of...

     -  \ref kOfxChangeUserEdited - the user or host changed the instance
     somehow and caused a change to something, this includes
     undo/redos, resets and loading values from files or presets,

     -  \ref kOfxChangePluginEdited - the plugin itself has changed the
     value of the instance in some action
     -  \ref kOfxChangeTime - the time has changed and this has affected the
     value of the object because it varies over time

     -  \ref kOfxPropTime
     - the effect time at which the chang occured (for Image Effect Plugins only)
     -  \ref kOfxImageEffectPropRenderScale
     - the render scale currently being applied to any image fetched
     from a clip (for Image Effect Plugins only)

 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionCreateInstance has been called on the instance handle,
     -  \ref kOfxActionBeginInstanceChanged has been called on the instance
     handle.

 \post
     -  \ref kOfxActionEndInstanceChanged will be called on the instance handle.

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatReplyDefault, the action was ignored
     -  \ref kOfxStatErrFatal,
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message

 */
#define kOfxActionInstanceChanged "OfxActionInstanceChanged"

/** @brief

 The \ref kOfxActionBeginInstanceChanged and \ref kOfxActionEndInstanceChanged actions
 are used to bracket all \ref kOfxActionInstanceChanged actions, whether a
 single change or multiple changes. Some changes to a plugin instance can
 be grouped logically (eg: a 'reset all' button resetting all the
 instance's parameters), the begin/end instance changed actions allow a
 plugin to respond appropriately to a large set of changes. For example,
 a plugin that maintains a complex internal state can delay any changes
 to that state until all parameter changes have completed.

 @param  handle
 handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs has the following properties
     -  \ref kOfxPropChangeReason what triggered the change, which will be one of...
     -  \ref kOfxChangeUserEdited - the user or host changed the instance
     somehow and caused a change to something, this includes
     undo/redos, resets and loading values from files or presets,
     -  \ref kOfxChangePluginEdited - the plugin itself has changed the
     value of the instance in some action
     -  \ref kOfxChangeTime - the time has changed and this has affected the
     value of the object because it varies over time

 @param  outArgs is redundant and is set to NULL

 \post
     - For \ref kOfxActionBeginInstanceChanged , \ref kOfxActionCreateInstance has been called on the instance handle.
     - For \ref kOfxActionEndInstanceChanged , \ref kOfxActionBeginInstanceChanged has been called on the instance handle.
     - \ref kOfxActionCreateInstance has been called on the instance handle.

 \post
     - For \ref kOfxActionBeginInstanceChanged, \ref kOfxActionInstanceChanged will be called at least once on the instance handle.
     - \ref kOfxActionEndInstanceChanged will be called on the instance handle.

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatReplyDefault, the action was ignored
     -  \ref kOfxStatErrFatal,
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message
*/
#define kOfxActionBeginInstanceChanged "OfxActionBeginInstanceChanged"

/** @brief Action called after the end of a set of \ref kOfxActionEndInstanceChanged actions, used with ::kOfxActionBeginInstanceChanged to bracket a grouped set of changes,  see \ref kOfxActionBeginInstanceChanged*/
#define kOfxActionEndInstanceChanged "OfxActionEndInstanceChanged"

/** @brief

 This is called when an instance is *first* actively edited by a user,
 ie: and interface is open and parameter values and input clips can be
 modified. It is there so that effects can create private user interface
 structures when necassary. Note that some hosts can have multiple
 editors open on the same effect instance simulateously.


 @param  handle handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionCreateInstance has been called on the instance handle,

 \post
     -  \ref kOfxActionEndInstanceEdit will be called when the last editor is
     closed on the instance

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatReplyDefault, the action was ignored
     -  \ref kOfxStatErrFatal,
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message
 */
#define kOfxActionBeginInstanceEdit "OfxActionBeginInstanceEdit"

/** @brief

 This is called when the *last* user interface on an instance closed. It
 is there so that effects can destroy private user interface structures
 when necassary. Note that some hosts can have multiple editors open on
 the same effect instance simulateously, this will only be called when
 the last of those editors are closed.

 @param  handle handle to the plug-in instance, cast to an \ref OfxImageEffectHandle
 @param  inArgs is redundant and is set to NULL
 @param  outArgs is redundant and is set to NULL

 \pre
     -  \ref kOfxActionBeginInstanceEdit has been called on the instance handle,

 \post
     -  no user interface is open on the instance

 @returns
     -  \ref kOfxStatOK, the action was trapped and all was well
     -  \ref kOfxStatReplyDefault, the action was ignored
     -  \ref kOfxStatErrFatal,
     -  \ref kOfxStatFailed, something went wrong, but no error code appropriate,
     the plugin should to post a message
 */
#define kOfxActionEndInstanceEdit "OfxActionEndInstanceEdit"

/*@}*/

/** @brief Returns the 'nth' plug-in implemented inside a binary
 *
 * Returns a pointer to the 'nth' plug-in implemented in the binary. A function of this type
 * must be implemented in and exported from each plug-in binary.
 */
OfxExport OfxPlugin *OfxGetPlugin(int nth);

/** @brief Defines the number of plug-ins implemented inside a binary
 *
 * A host calls this to determine how many plug-ins there are inside
 * a binary it has loaded. A function of this type
 * must be implemented in and exported from each plug-in binary.
 */
OfxExport int OfxGetNumberOfPlugins(void);

/** @brief First thing host should call
*
* This host call, added in 2020, is not specified in earlier implementation of the API.
* Therefore host must check if the plugin implemented it and not assume symbol exists.
* The order of calls is then:  1) OfxSetHost, 2) OfxGetNumberOfPlugins, 3) OfxGetPlugin
* The host pointer is only assumed valid until OfxGetPlugin where it might get reset.
* Plug-in can return kOfxStatFailed to indicate it has nothing to do here, it's not for this Host and it should be skipped silently.
*/

OfxExport  OfxStatus OfxSetHost(const OfxHost *host);

/**
   \defgroup PropertiesAll Ofx Properties

These strings are used to identify properties within OFX, they are broken up by the host suite or API they relate to.
*/
/*@{*/

/**
   \defgroup PropertiesGeneral General Properties

These properties are general properties and  apply to may objects across OFX
*/
/*@{*/

/** @brief Property on the host descriptor, saying what API version of the API is being implemented

    - Type - int X N
    - Property Set - host descriptor.

This is a version string that will specify which version of the API is being implemented by a host. It
can have multiple values. For example "1.0", "1.2.4" etc.....

If this is not present, it is safe to assume that the version of the API is "1.0".
*/
#define kOfxPropAPIVersion "OfxPropAPIVersion"

/** @brief General property used to get/set the time of something.

    - Type - double X 1
    - Default - 0, if a setable property
    - Property Set - commonly used as an argument to actions, input and output.
*/
#define kOfxPropTime "OfxPropTime"

/** @brief Indicates if a host is actively editing the effect with some GUI.

    - Type - int X 1
    - Property Set - effect instance (read only)
    - Valid Values - 0 or 1

If false the effect currently has no interface, however this may be because the effect is loaded in a background render host, or it may be loaded on an interactive host that has not yet opened an editor for the effect.

The output of an effect should only ever depend on the state of its parameters, not on the interactive flag. The interactive flag is more a courtesy flag to let a plugin know that it has an interace. If a plugin want's to have its behaviour dependant on the interactive flag, it can always make a secret parameter which shadows the state if the flag.
*/
#define kOfxPropIsInteractive "OfxPropIsInteractive"

/** @brief The file path to the plugin.

    - Type - C string X 1
    - Property Set - effect descriptor (read only)

This is a string that indicates the file path where the plug-in was found by the host. The path is in the native
path format for the host OS (eg:  UNIX directory separators are forward slashes, Windows ones are backslashes).

The path is to the bundle location, see \ref InstallationLocation.
eg:  '/usr/OFX/Plugins/AcmePlugins/AcmeFantasticPlugin.ofx.bundle'
*/
#define kOfxPluginPropFilePath "OfxPluginPropFilePath"

/** @brief  A private data pointer that the plug-in can store its own data behind.

    - Type - pointer X 1
    - Property Set - plugin instance (read/write),
    - Default - NULL

This data pointer is unique to each plug-in instance, so two instances of the same plug-in do not share the same data pointer. Use it to hang any needed private data structures.
*/
#define kOfxPropInstanceData "OfxPropInstanceData"

/** @brief General property, used to identify the kind of an object behind a handle

    - Type - ASCII C string X 1
    - Property Set - any object handle (read only)
    - Valid Values - currently this can be...
       - ::kOfxTypeImageEffectHost
       - ::kOfxTypeImageEffect
       - ::kOfxTypeImageEffectInstance
       - ::kOfxTypeParameter
       - ::kOfxTypeParameterInstance
       - ::kOfxTypeClip
       - ::kOfxTypeImage
*/
#define kOfxPropType "OfxPropType"

/** @brief Unique name of an object.

    - Type - ASCII C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject (read only)

This property is used to label objects uniquely amoung objects of that type. It is typically set when a plugin creates a new object with a function that takes a name.
*/
#define kOfxPropName "OfxPropName"

/** @brief Identifies a specific version of a host or plugin.

    - Type - int X N
    - Property Set - host descriptor (read only), plugin descriptor (read/write)
    - Default - "0"
    - Valid Values - positive integers

This is a multi dimensional integer property that represents the version of a host (host descriptor), or plugin (plugin descriptor). These represent a version number of the form '1.2.3.4', with each dimension adding another 'dot' on the right.

A version is considered to be more recent than another if its ordered set of values is lexicographically greater than another, reading left to right. (ie: 1.2.4 is smaller than 1.2.6). Also, if the number of dimensions is different, then the values of the missing dimensions are considered to be zero (so 1.2.4 is greater than 1.2).
*/
#define kOfxPropVersion "OfxPropVersion"

/** @brief Unique user readable version string of a plugin or host.

    - Type - string X 1
    - Property Set - host descriptor (read only), plugin descriptor (read/write)
    - Default - none, the host needs to set this
    - Valid Values - ASCII string

This is purely for user feedback, a plugin or host should use ::kOfxPropVersion if they need
to check for specific versions.
*/
#define kOfxPropVersionLabel "OfxPropVersionLabel"

/** @brief Description of the plug-in to a user.

    - Type - string X 1
    - Property Set - plugin descriptor (read/write) and instance (read only)
    - Default - ""
    - Valid Values - UTF8 string

This is a string giving a potentially verbose description of the effect.
*/
#define kOfxPropPluginDescription "OfxPropPluginDescription"

/** @brief User visible name of an object.

    - Type - UTF8 C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject. Typically readable and writable in most cases.
    - Default - the ::kOfxPropName the object was created with.

The label is what a user sees on any interface in place of the object's name.

Note that resetting this will also reset ::kOfxPropShortLabel and ::kOfxPropLongLabel.
*/
#define kOfxPropLabel "OfxPropLabel"

/** @brief If set this tells the host to use an icon instead of a label for some object in the interface.

    - Type - string X 2
    - Property Set - various descriptors in the API
    - Default - ""
    - Valid Values - ASCII string

The value is a path is defined relative to the Resource folder that points to an SVG or PNG file containing the icon.

The first dimension, if set, will the name of and SVG file, the second a PNG file.
*/
#define kOfxPropIcon "OfxPropIcon"

/** @brief Short user visible name of an object.

    - Type - UTF8 C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject. Typically readable and writable in most cases.
    - Default - initially ::kOfxPropName, but will be reset if ::kOfxPropLabel is changed.

This is a shorter version of the label, typically 13 character glyphs or less. Hosts should use this if they have limitted display space for their object labels.
*/
#define kOfxPropShortLabel "OfxPropShortLabel"

/** @brief Long user visible name of an object.

    - Type - UTF8 C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject. Typically readable and writable in most cases.
    - Default - initially ::kOfxPropName, but will be reset if ::kOfxPropLabel is changed.

This is a longer version of the label, typically 32 character glyphs or so. Hosts should use this if they have mucg display space for their object labels.
*/
#define kOfxPropLongLabel "OfxPropLongLabel"

/** @brief Indicates why a plug-in changed.

    - Type - ASCII C string X 1
    - Property Set - inArgs parameter on the ::kOfxActionInstanceChanged action.
    - Valid Values - this can be...
       - ::kOfxChangeUserEdited - the user directly edited the instance somehow and caused a change to something, this includes undo/redos and resets
       - ::kOfxChangePluginEdited - the plug-in itself has changed the value of the object in some action
       - ::kOfxChangeTime - the time has changed and this has affected the value of the object because it varies over time

Argument property for the ::kOfxActionInstanceChanged action.
*/
#define kOfxPropChangeReason "OfxPropChangeReason"

/** @brief A pointer to an effect instance.

    - Type - pointer X 1
    - Property Set - on an interact instance (read only)

This property is used to link an object to the effect. For example if the plug-in supplies an openGL overlay for an image effect,
the interact instance will have one of these so that the plug-in can connect back to the effect the GUI links to.
*/
#define kOfxPropEffectInstance "OfxPropEffectInstance"

/** @brief A pointer to an operating system specific application handle.

    - Type - pointer X 1
    - Property Set - host descriptor.

Some plug-in vendor want raw OS specific handles back from the host so they can do interesting things with host OS APIs. Typically this is to control windowing properly on Microsoft Windows. This property returns the appropriate 'root' window handle on the current operating system. So on Windows this would be the hWnd of the application main window.
*/
#define kOfxPropHostOSHandle "OfxPropHostOSHandle"

/*@}*/

/*@}*/

/** @brief String used as a value to ::kOfxPropChangeReason to indicate a user has changed something */
#define kOfxChangeUserEdited "OfxChangeUserEdited"

/** @brief String used as a value to ::kOfxPropChangeReason to indicate the plug-in itself has changed something */
#define kOfxChangePluginEdited "OfxChangePluginEdited"

/** @brief String used as a value to ::kOfxPropChangeReason to a time varying object has changed due to a time change */
#define kOfxChangeTime "OfxChangeTime"

/** @brief How time is specified within the OFX API */
typedef double OfxTime;

/** @brief Defines one dimensional integer bounds */
typedef struct OfxRangeI {
  int min, max;
} OfxRangeI;

/** @brief Defines one dimensional double bounds */
typedef struct OfxRangeD {
  double min, max;
} OfxRangeD;

/** @brief Defines two dimensional integer point */
typedef struct OfxPointI {
  int x, y;
} OfxPointI;

/** @brief Defines two dimensional double point */
typedef struct OfxPointD {
  double x, y;
} OfxPointD;

/** @brief Used to flag infinite rects. Set minimums to this to indicate infinite

This is effectively INT_MAX.
 */
#define kOfxFlagInfiniteMax INT_MAX

/** @brief Used to flag infinite rects. Set minimums to this to indicate infinite.

This is effectively INT_MIN
 */
#define kOfxFlagInfiniteMin INT_MIN

/** @brief Defines two dimensional integer region

Regions are x1 <= x < x2

Infinite regions are flagged by setting
- x1 = \ref kOfxFlagInfiniteMin
- y1 = \ref kOfxFlagInfiniteMin
- x2 = \ref kOfxFlagInfiniteMax
- y2 = \ref kOfxFlagInfiniteMax

 */
typedef struct OfxRectI {
  int x1, y1, x2, y2;
} OfxRectI;

/** @brief Defines two dimensional double region

Regions are x1 <= x < x2

Infinite regions are flagged by setting
- x1 = \ref kOfxFlagInfiniteMin
- y1 = \ref kOfxFlagInfiniteMin
- x2 = \ref kOfxFlagInfiniteMax
- y2 = \ref kOfxFlagInfiniteMax

 */
typedef struct OfxRectD {
  double x1, y1, x2, y2;
} OfxRectD;

/** @brief String used to label unset bitdepths */
#define kOfxBitDepthNone "OfxBitDepthNone"

/** @brief String used to label unsigned 8 bit integer samples */
#define kOfxBitDepthByte "OfxBitDepthByte"

/** @brief String used to label unsigned 16 bit integer samples */
#define kOfxBitDepthShort "OfxBitDepthShort"

/** @brief String used to label half-float (16 bit floating point) samples
 *  \version Added in Version 1.4. Was in ofxOpenGLRender.h before.
 */
#define kOfxBitDepthHalf "OfxBitDepthHalf"

/** @brief String used to label signed 32 bit floating point samples */
#define kOfxBitDepthFloat "OfxBitDepthFloat"

/**
   \defgroup StatusCodes Status Codes

These strings are used to identify error states within ofx, they are returned
by various host suite functions, as well as plug-in functions. The valid return codes
for each function are documented with that function.
*/
/*@{*/

/**
   \defgroup StatusCodesGeneral General Status Codes

General status codes start at 1 and continue until 999

*/
/*@{*/

/** @brief Status code indicating all was fine */
#define kOfxStatOK 0

/** @brief Status error code for a failed operation. */
#define kOfxStatFailed  ((int)1)

/** @brief Status error code for a fatal error

  Only returned in the case where the plug-in or host cannot continue to function and needs to be restarted.
 */
#define kOfxStatErrFatal ((int)2)

/** @brief Status error code for an operation on or request for an unknown object */
#define kOfxStatErrUnknown ((int)3)

/** @brief Status error code returned by plug-ins when they are missing host functionality, either an API or some optional functionality (eg: custom params).

    Plug-Ins returning this should post an appropriate error message stating what they are missing.
 */
#define kOfxStatErrMissingHostFeature ((int) 4)

/** @brief Status error code for an unsupported feature/operation */
#define kOfxStatErrUnsupported ((int) 5)

/** @brief Status error code for an operation attempting to create something that exists */
#define kOfxStatErrExists  ((int) 6)

/** @brief Status error code for an incorrect format */
#define kOfxStatErrFormat ((int) 7)

/** @brief Status error code indicating that something failed due to memory shortage */
#define kOfxStatErrMemory  ((int) 8)

/** @brief Status error code for an operation on a bad handle */
#define kOfxStatErrBadHandle ((int) 9)

/** @brief Status error code indicating that a given index was invalid or unavailable */
#define kOfxStatErrBadIndex ((int)10)

/** @brief Status error code indicating that something failed due an illegal value */
#define kOfxStatErrValue ((int) 11)

/** @brief OfxStatus returned indicating a 'yes' */
#define kOfxStatReplyYes ((int) 12)

/** @brief OfxStatus returned indicating a 'no' */
#define kOfxStatReplyNo ((int) 13)

/** @brief OfxStatus returned indicating that a default action should be performed */
#define kOfxStatReplyDefault ((int) 14)

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

/** @mainpage OFX : Open Plug-Ins For Special Effects

This page represents the automatically extracted HTML documentation of the source headers for the OFX Image Effect API.
The documentation was extracted by doxygen (http://www.doxygen.org).
A more complete reference manual is https://openfx.readthedocs.io .

*/

#endif
