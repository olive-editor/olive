

/** @brief This file contains code that skins the ofx effect suite */

#include "ofxsSupportPrivate.h"
#include <algorithm> // for find
#include <cstring> // for strlen
#ifdef DEBUG
#include <iostream>
#endif
#include <stdexcept>
#include "ofxGPURender.h"
#include "ofxsCore.h"

#if defined __APPLE__ || defined __linux__ || defined __FreeBSD__
# if __GNUC__ >= 4
#  define EXPORT __attribute__((visibility("default")))
#  define LOCAL  __attribute__((visibility("hidden")))
# else
#  define EXPORT
#  define LOCAL
# endif
#elif defined _WIN32
#  define EXPORT OfxExport
#  define LOCAL
#else
#  error Not building on your operating system quite yet
#endif

// string utility functions
static bool ends_with(std::string const & value, std::string const & ending)
{
  if (ending.size() > value.size()) return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static bool starts_with(std::string const & value, std::string const & beginning)
{
  if (beginning.size() > value.size()) return false;
  return std::equal(beginning.begin(), beginning.end(), value.begin());
}

/** @brief The core 'OFX Support' namespace, used by plugin implementations. All code for these are defined in the common support libraries. */
namespace OFX {

  // globals to keep consistent data structures around.
  OFX::PluginFactoryArray plugIDs;
  //Put it all into a map, so we know when to delete what!
  struct OfxPlugInfo
  {
    OfxPlugInfo() = default;
    OfxPlugInfo(OFX::PluginFactory* f, std::unique_ptr<OfxPlugin> p):_factory(f), _plug(std::move(p)){}
    OFX::PluginFactory* _factory = nullptr;
    std::unique_ptr<OfxPlugin> _plug;
  };
  typedef std::map<std::string, OfxPlugInfo> OfxPlugInfoMap;
  OfxPlugInfoMap plugInfoMap;

  typedef std::vector<OfxPlugin*> OfxPluginArray;
  OfxPluginArray ofxPlugs;

  /** @brief the global host description */
  ImageEffectHostDescription gHostDescription;
  bool gHostDescriptionHasInit = false;

  bool ImageEffectHostDescription::supportsPixelComponent(const PixelComponentEnum component) const
  {
    return std::find(_supportedComponents.begin(), _supportedComponents.end(), component) != _supportedComponents.end();
  }
  bool ImageEffectHostDescription::supportsBitDepth( const BitDepthEnum bitDepth) const
  {
    return std::find(_supportedPixelDepths.begin(), _supportedPixelDepths.end(), bitDepth) != _supportedPixelDepths.end();
  }
  bool ImageEffectHostDescription::supportsContext(const ContextEnum context) const
  {
    return std::find(_supportedContexts.begin(), _supportedContexts.end(), context) != _supportedContexts.end();
  }
	
  /** @return default pixel depth supported by host application. */
  BitDepthEnum ImageEffectHostDescription::getDefaultPixelDepth() const
  {
    if(!_supportedPixelDepths.empty()) {
      return _supportedPixelDepths[0];
    } else {
      OFX::Log::warning(true, "The host doesn't define supported pixel depth. (size: %d)", (int)_supportedPixelDepths.size());
      return eBitDepthFloat;
    }
  }
	
  /** @return default pixel component supported by host application. */
  PixelComponentEnum ImageEffectHostDescription::getDefaultPixelComponent() const
  {
    if(! _supportedComponents.empty()) {
      return _supportedComponents[0];
    } else {
      OFX::Log::warning(true, "The host doesn't define supported pixel component. (size: %d)", _supportedComponents.size());
      return ePixelComponentRGBA;
    }
  }

  ImageEffectHostDescription* getImageEffectHostDescription()
  {
    if(gHostDescriptionHasInit)
      return &gHostDescription;
    return NULL;
  }

  namespace Private {        
    // Suite and host pointers
    OfxHost               *gHost = 0;
    OfxImageEffectSuiteV1 *gEffectSuite = 0;
    OfxPropertySuiteV1    *gPropSuite = 0;
    OfxInteractSuiteV1    *gInteractSuite = 0;
    OfxParameterSuiteV1   *gParamSuite = 0;
    OfxMemorySuiteV1      *gMemorySuite = 0;
    OfxMultiThreadSuiteV1 *gThreadSuite = 0;
    OfxMessageSuiteV1     *gMessageSuite = 0;
    OfxMessageSuiteV2     *gMessageSuiteV2 = 0;
    OfxProgressSuiteV1    *gProgressSuiteV1 = 0;
    OfxProgressSuiteV2    *gProgressSuiteV2 = 0;
    OfxTimeLineSuiteV1    *gTimeLineSuite = 0;
    OfxParametricParameterSuiteV1 *gParametricParameterSuite = 0;
#ifdef OFX_SUPPORTS_OPENGLRENDER
    OfxImageEffectOpenGLRenderSuiteV1 *gOpenGLRenderSuite = 0;
#endif

    // @brief the set of descriptors, one per context used by kOfxActionDescribeInContext,
    //'eContextNone' is the one used by the kOfxActionDescribe
    EffectDescriptorMap gEffectDescriptors;
  };

  /** @brief map a std::string to a context */
  ContextEnum mapToContextEnum(const std::string &s)
  {
    if(s == kOfxImageEffectContextGenerator) return eContextGenerator;
    if(s == kOfxImageEffectContextFilter) return eContextFilter;
    if(s == kOfxImageEffectContextTransition) return eContextTransition;
    if(s == kOfxImageEffectContextPaint) return eContextPaint;
    if(s == kOfxImageEffectContextGeneral) return eContextGeneral;
    if(s == kOfxImageEffectContextRetimer) return eContextRetimer;
    OFX::Log::error(true, "Unknown image effect context '%s'", s.c_str());
    throw std::invalid_argument(s);
  }

  const char* mapContextEnumToStr(ContextEnum context)
  {
    switch (context) {
      case eContextGenerator:
        return kOfxImageEffectContextGenerator;
      case eContextFilter:
        return kOfxImageEffectContextFilter;
      case eContextTransition:
        return kOfxImageEffectContextTransition;
      case eContextPaint:
        return kOfxImageEffectContextPaint;
      case eContextGeneral:
        return kOfxImageEffectContextGeneral;
      case eContextRetimer:
        return kOfxImageEffectContextRetimer;
      default:
        OFX::Log::error(true, "Unknown context enum '%d'", (int)context);
        throw std::invalid_argument("unknown ContextEnum");
    }
  }

  const char* mapMessageTypeEnumToStr(OFX::Message::MessageTypeEnum type)
  {
    if(type == OFX::Message::eMessageFatal)
      return kOfxMessageFatal;
    else if(type == OFX::Message::eMessageError)
      return kOfxMessageError;
    else if(type == OFX::Message::eMessageMessage)
      return kOfxMessageMessage;
    else if(type == OFX::Message::eMessageWarning)
      return kOfxMessageWarning;
    else if(type == OFX::Message::eMessageLog)
      return kOfxMessageLog;
    else if(type == OFX::Message::eMessageQuestion)
      return kOfxMessageQuestion;
    OFX::Log::error(true, "Unknown message type enum '%d'", type);
    return 0;
  }

  OFX::Message::MessageReplyEnum mapToMessageReplyEnum(OfxStatus stat)
  {
    if(stat == kOfxStatOK)
      return OFX::Message::eMessageReplyOK;
    else if(stat == kOfxStatReplyYes)
      return OFX::Message::eMessageReplyYes;
    else if(stat == kOfxStatReplyNo)
      return OFX::Message::eMessageReplyNo;
    else if(stat == kOfxStatFailed)
      return OFX::Message::eMessageReplyFailed;
    OFX::Log::error(true, "Unknown message reply status enum '%d'", stat);
    return OFX::Message::eMessageReplyFailed;
  }

  /** @brief map a std::string to a context */
  InstanceChangeReason mapToInstanceChangedReason(const std::string &s)
  {
    if(s == kOfxChangePluginEdited) return eChangePluginEdit;
    if(s == kOfxChangeUserEdited) return eChangeUserEdit;
    if(s == kOfxChangeTime) return eChangeTime;
    OFX::Log::error(true, "Unknown instance changed reason '%s'", s.c_str());
    throw std::invalid_argument(s);
  }

  /** @brief turns a bit depth string into and enum */
  BitDepthEnum mapStrToBitDepthEnum(const std::string &str)
  {
    if(str == kOfxBitDepthByte) {
      return eBitDepthUByte;
    }
    else if(str == kOfxBitDepthShort) {
      return eBitDepthUShort;
    }
    else if(str == kOfxBitDepthHalf) {
      return eBitDepthHalf;
    }
    else if(str == kOfxBitDepthFloat) {
      return eBitDepthFloat;
    }
    else if(str == kOfxBitDepthNone) {
      return eBitDepthNone;
    }
    else {
      return eBitDepthCustom;
    }
  }

  /** @brief turns a bit depth string into and enum */
  const char* mapBitDepthEnumToStr(BitDepthEnum bitDepth)
  {
    switch (bitDepth) {
      case eBitDepthUByte:
        return kOfxBitDepthByte;
      case eBitDepthUShort:
        return kOfxBitDepthShort;
      case eBitDepthHalf:
        return kOfxBitDepthHalf;
      case eBitDepthFloat:
        return kOfxBitDepthFloat;
      case eBitDepthNone:
        return kOfxBitDepthNone;
      case eBitDepthCustom:
        return "OfxBitDepthCustom";
      default:
        OFX::Log::error(true, "Unknown bit depth enum '%d'", (int)bitDepth);
        throw std::invalid_argument("unknown BitDepthEnum");
    }
  }

  /** @brief turns a pixel component string into and enum */
  PixelComponentEnum mapStrToPixelComponentEnum(const std::string &str)
  {
    if(str == kOfxImageComponentRGBA) {
      return ePixelComponentRGBA;
    }
    else if(str == kOfxImageComponentRGB) {
      return ePixelComponentRGB;
    }
    else if(str == kOfxImageComponentAlpha) {
      return ePixelComponentAlpha;
    }
    else if(str == kOfxImageComponentNone) {
      return ePixelComponentNone;
    }
    else {
      return ePixelComponentCustom;
    }
  }

  /** @brief turns a pixel component string into and enum */
  const char* mapPixelComponentEnumToStr(PixelComponentEnum pixelComponent)
  {
    switch (pixelComponent) {
      case ePixelComponentRGBA:
        return kOfxImageComponentRGBA;
      case ePixelComponentRGB:
        return kOfxImageComponentRGB;
      case ePixelComponentAlpha:
        return kOfxImageComponentAlpha;
      case ePixelComponentCustom:
        return "OfxImageComponentCustom";
      default:
        OFX::Log::error(true, "Unknown pixel component enum '%d'", (int)pixelComponent);
        throw std::invalid_argument("unknown PixelComponentEnum");
    }
  }

  /** @brief turns a premultiplication string into and enum */
  static PreMultiplicationEnum mapStrToPreMultiplicationEnum(const std::string &str)
  {
    if(str == kOfxImageOpaque) {
      return eImageOpaque;
    }
    else if(str == kOfxImagePreMultiplied) {
      return eImagePreMultiplied;
    }
    else if(str == kOfxImageUnPreMultiplied) {
      return eImageUnPreMultiplied;
    }
    else {
      throw std::invalid_argument("");
    }
  }

  /** @brief turns a field string into and enum */
  FieldEnum mapStrToFieldEnum(const std::string &str)
  {
    if(str == kOfxImageFieldNone) {
      return eFieldNone;
    }
    else if(str == kOfxImageFieldBoth) {
      return eFieldBoth;
    }
    else if(str == kOfxImageFieldLower) {
      return eFieldLower;
    }
    else if(str == kOfxImageFieldUpper) {
      return eFieldUpper;
    }
    else {
      throw std::invalid_argument("");
    }
  }


  ////////////////////////////////////////////////////////////////////////////////
  // clip descriptor

  /** @brief hidden constructor */
  ClipDescriptor::ClipDescriptor(const std::string &name, OfxPropertySetHandle props)
    : _clipName(name)
    , _clipProps(props)
  {
    OFX::Validation::validateClipDescriptorProperties(props);
  }

  /** @brief set the label properties */
  void ClipDescriptor::setLabel(const std::string &label)
  {
    _clipProps.propSetString(kOfxPropLabel, label);
  }

  /** @brief set the label properties */
  void ClipDescriptor::setLabels(const std::string &label, const std::string &shortLabel, const std::string &longLabel)
  {
    setLabel(label);
    _clipProps.propSetString(kOfxPropShortLabel, shortLabel, false);
    _clipProps.propSetString(kOfxPropLongLabel, longLabel, false);
  }

  /** @brief set how fielded images are extracted from the clip defaults to eFieldExtractDoubled */
  void ClipDescriptor::setFieldExtraction(FieldExtractionEnum v)
  {
    switch(v) 
    {
    case eFieldExtractBoth :
      _clipProps.propSetString(kOfxImageClipPropFieldExtraction, kOfxImageFieldBoth);
      break;

    case eFieldExtractSingle :
      _clipProps.propSetString(kOfxImageClipPropFieldExtraction, kOfxImageFieldSingle);
      break;

    case eFieldExtractDoubled :
      _clipProps.propSetString(kOfxImageClipPropFieldExtraction, kOfxImageFieldDoubled);
      break;
    }
  }

  /** @brief set which components are supported, defaults to none set, this must be called at least once! */
  void ClipDescriptor::addSupportedComponent(PixelComponentEnum v)
  {
    int n = _clipProps.propGetDimension(kOfxImageEffectPropSupportedComponents);
    switch(v) 
    {
    case ePixelComponentNone :
      _clipProps.propSetString(kOfxImageEffectPropSupportedComponents, kOfxImageComponentNone, n);
      break;

    case ePixelComponentRGBA :
      _clipProps.propSetString(kOfxImageEffectPropSupportedComponents, kOfxImageComponentRGBA, n);
      break;

    case ePixelComponentRGB :
      _clipProps.propSetString(kOfxImageEffectPropSupportedComponents, kOfxImageComponentRGB, n);
      break;

    case ePixelComponentAlpha :
      _clipProps.propSetString(kOfxImageEffectPropSupportedComponents, kOfxImageComponentAlpha, n);
      break;
    case ePixelComponentCustom :
      break;
    }
  }

  /** @brief set which components are supported, defaults to none set, this must be called at least once! */
  void ClipDescriptor::addSupportedComponent(const std::string &comp)
  {
    int n = _clipProps.propGetDimension(kOfxImageEffectPropSupportedComponents);
    _clipProps.propSetString(kOfxImageEffectPropSupportedComponents, comp, n);

  }

  /** @brief say whether we are going to do random temporal access on this clip, defaults to false */
  void ClipDescriptor::setTemporalClipAccess(bool v)
  {
    _clipProps.propSetInt(kOfxImageEffectPropTemporalClipAccess, int(v));
  }

  /** @brief say whether if the clip is optional, defaults to false */
  void ClipDescriptor::setOptional(bool v)
  {
    _clipProps.propSetInt(kOfxImageClipPropOptional, int(v));
  }

  /** @brief say whether this clip supports tiling, defaults to true */
  void ClipDescriptor::setSupportsTiles(bool v)
  {
    _clipProps.propSetInt(kOfxImageEffectPropSupportsTiles, int(v));
  }

  /** @brief say whether this clip is a 'mask', so the host can know to replace with a roto or similar, defaults to false */
  void ClipDescriptor::setIsMask(bool v)
  {
    _clipProps.propSetInt(kOfxImageClipPropIsMask, int(v));
  }

  ////////////////////////////////////////////////////////////////////////////////
  // image effect descriptor

  /** @brief effect descriptor ctor */
  ImageEffectDescriptor::ImageEffectDescriptor(OfxImageEffectHandle handle)
    : _effectHandle(handle)
  {
    // fetch the property set handle of the effect
    OfxPropertySetHandle props;
    OfxStatus stat = OFX::Private::gEffectSuite->getPropertySet(handle, &props);
    throwSuiteStatusException(stat);
    _effectProps.propSetHandle(props);

    OFX::Validation::validatePluginDescriptorProperties(props);

    // fetch the param set handle and set it in our ParamSetDescriptor base
    OfxParamSetHandle paramSetHandle;
    stat = OFX::Private::gEffectSuite->getParamSet(handle, &paramSetHandle);
    throwSuiteStatusException(stat);
    setParamSetHandle(paramSetHandle);
  }


  /** @brief dtor */
  ImageEffectDescriptor::~ImageEffectDescriptor()
  {
    // delete any clip descriptors we may have constructed
    std::map<std::string, ClipDescriptor *>::iterator iter;
    for(iter = _definedClips.begin(); iter != _definedClips.end(); ++iter) {
      if(iter->second) {
        delete iter->second;
        iter->second = NULL;
      }
    }
  }

  /** @brief, set the label properties in a plugin */
  void ImageEffectDescriptor::setLabel(const std::string &label)
  {
    _effectProps.propSetString(kOfxPropLabel, label);
  }

  /** @brief, set the label properties in a plugin */
  void ImageEffectDescriptor::setLabels(const std::string &label, const std::string &shortLabel, const std::string &longLabel)
  {
    setLabel(label);
    _effectProps.propSetString(kOfxPropShortLabel, shortLabel, false);
    _effectProps.propSetString(kOfxPropLongLabel, longLabel, false);
  }


  /** @brief, set the version properties in a plugin */
  void ImageEffectDescriptor::setVersion(int major, int minor, int micro, int build, const std::string &versionLabel)
  {
    _effectProps.propSetInt(kOfxPropVersion, major, 0, false); // introduced in OFX 1.2
    if (minor || micro || build) {
      _effectProps.propSetInt(kOfxPropVersion, minor, 1, false); // introduced in OFX 1.2
      if (micro || build) {
        _effectProps.propSetInt(kOfxPropVersion, micro, 2, false); // introduced in OFX 1.2
        if (build) {
          _effectProps.propSetInt(kOfxPropVersion, build, 3, false); // introduced in OFX 1.2
        }
      }
    }
    if (!versionLabel.empty()) {
      _effectProps.propSetString(kOfxPropVersionLabel, versionLabel, false);
    }
  }

  /** @brief Set the plugin grouping */
  void ImageEffectDescriptor::setPluginGrouping(const std::string &group)
  {
    _effectProps.propSetString(kOfxImageEffectPluginPropGrouping, group);
  }

  /** @brief Set the plugin description, defaults to "" */
  void ImageEffectDescriptor::setPluginDescription(const std::string &description)
  {
    _effectProps.propSetString(kOfxPropPluginDescription, description, false); // introduced in OFX 1.2
  }

  /** @brief Add a context to those supported */
  void ImageEffectDescriptor::addSupportedContext(ContextEnum v)
  {
    int n = _effectProps.propGetDimension(kOfxImageEffectPropSupportedContexts);
    switch (v) 
    {
    case eContextNone :
      break;
    case eContextGenerator :
      _effectProps.propSetString(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGenerator, n);
      break;
    case eContextFilter :
      _effectProps.propSetString(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextFilter, n);
      break;
    case eContextTransition :
      _effectProps.propSetString(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextTransition, n);
      break;
    case eContextPaint :
      _effectProps.propSetString(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextPaint, n);
      break;
    case eContextGeneral :
      _effectProps.propSetString(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGeneral, n);
      break;
    case eContextRetimer :
      _effectProps.propSetString(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextRetimer, n);
      break;
    }
  }

  void ImageEffectDescriptor::setOverlayInteractDescriptor(EffectOverlayDescriptor* desc)
  {
    _overlayDescriptor.reset(desc);
    if(OFX::gHostDescription.supportsOverlays && desc->getMainEntry())
      _effectProps.propSetPointer(kOfxImageEffectPluginPropOverlayInteractV1, (void*)desc->getMainEntry());
  }

  /** @brief Add a pixel depth to those supported */
  void ImageEffectDescriptor::addSupportedBitDepth(BitDepthEnum v)
  {
    int n = _effectProps.propGetDimension(kOfxImageEffectPropSupportedPixelDepths);
    switch(v) 
    {
    case eBitDepthNone :
      _effectProps.propSetString(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthNone  , n);
      break;
    case eBitDepthUByte :
      _effectProps.propSetString(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthByte  , n);
      break;
    case eBitDepthUShort :
      _effectProps.propSetString(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthShort  , n);
      break;
    case eBitDepthHalf :
      _effectProps.propSetString(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthHalf  , n);
      break;
    case eBitDepthFloat :
      _effectProps.propSetString(kOfxImageEffectPropSupportedPixelDepths, kOfxBitDepthFloat  , n);
      break;
    case eBitDepthCustom :
      break;
    }
  }

#ifdef OFX_SUPPORTS_OPENGLRENDER
  /** @brief Add a pixel depth to those supported */
  void ImageEffectDescriptor::addSupportedOpenGLBitDepth(BitDepthEnum v)
  {
    int n = _effectProps.propGetDimension(kOfxOpenGLPropPixelDepth);
    switch(v) 
    {
    case eBitDepthNone :
      _effectProps.propSetString(kOfxOpenGLPropPixelDepth, kOfxBitDepthNone  , n);
      break;
    case eBitDepthUByte :
      _effectProps.propSetString(kOfxOpenGLPropPixelDepth, kOfxBitDepthByte  , n);
      break;
    case eBitDepthUShort :
      _effectProps.propSetString(kOfxOpenGLPropPixelDepth, kOfxBitDepthShort  , n);
      break;
    case eBitDepthHalf :
      _effectProps.propSetString(kOfxOpenGLPropPixelDepth, kOfxBitDepthHalf  , n);
      break;
    case eBitDepthFloat :
      _effectProps.propSetString(kOfxOpenGLPropPixelDepth, kOfxBitDepthFloat  , n);
      break;
    default:
      break;
    }
  }
#endif

  /** @brief Is the plugin single instance only ? */
  void ImageEffectDescriptor::setSingleInstance(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPluginPropSingleInstance, int(v));
  }

  /** @brief Does the plugin expect the host to perform per frame SMP threading */
  void ImageEffectDescriptor::setHostFrameThreading(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPluginPropHostFrameThreading, int(v));
  }

  /** @brief Does the plugin support multi resolution images */
  void ImageEffectDescriptor::setSupportsMultiResolution(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPropSupportsMultiResolution, int(v));
  }

  /** @brief Does the plugin support image tiling */
  void ImageEffectDescriptor::setSupportsTiles(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPropSupportsTiles, int(v));
  }

  /** @brief Does the plugin perform temporal clip access */
  void ImageEffectDescriptor::setTemporalClipAccess(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPropTemporalClipAccess, int(v));
  }

  /** @brief Does the plugin want to have render called twice per frame in all circumanstances for fielded images ? */
  void ImageEffectDescriptor::setRenderTwiceAlways(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPluginPropFieldRenderTwiceAlways, int(v));
  }

  /** @brief Does the plugin support inputs and output clips of differing depths */
  void ImageEffectDescriptor::setSupportsMultipleClipDepths(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPropSupportsMultipleClipDepths, int(v));
  }

  /** @brief Does the plugin support inputs and output clips of pixel aspect ratios */
  void ImageEffectDescriptor::setSupportsMultipleClipPARs(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPropSupportsMultipleClipPARs, int(v));
  }

  /** @brief What kind of thread safety does the plugin have */
  void ImageEffectDescriptor::setRenderThreadSafety(RenderSafetyEnum v)
  {
    switch(v) 
    {
    case eRenderUnsafe :
      _effectProps.propSetString(kOfxImageEffectPluginRenderThreadSafety, kOfxImageEffectRenderUnsafe);
      break;
    case eRenderInstanceSafe :
      _effectProps.propSetString(kOfxImageEffectPluginRenderThreadSafety, kOfxImageEffectRenderInstanceSafe);
      break;
    case eRenderFullySafe :
      _effectProps.propSetString(kOfxImageEffectPluginRenderThreadSafety, kOfxImageEffectRenderFullySafe);
      break;
    }
  }

  /** @brief Does the plugin support OpenCL Buffers Render */
  void ImageEffectDescriptor::setSupportsOpenCLBuffersRender(bool v)
  {
    try {

      _effectProps.propSetString(kOfxImageEffectPropOpenCLRenderSupported, (v ? "true" : "false"));
    } catch(OFX::Exception::PropertyUnknownToHost) {
      OFX::Log::warning(true, "Host does not have kOfxImageEffectPropOpenCLRenderSupported property");
    }
  }

  /** @brief Does the plugin support OpenCL Images Render */
  void ImageEffectDescriptor::setSupportsOpenCLImagesRender(bool v)
  {
    try {

      _effectProps.propSetString(kOfxImageEffectPropOpenCLSupported, (v ? "true" : "false"));
    } catch(OFX::Exception::PropertyUnknownToHost) {
      OFX::Log::warning(true, "Host does not have kOfxImageEffectPropOpenCLSupported property");
    }
  }

  /** @brief Does the plugin support CUDA Render */
  void ImageEffectDescriptor::setSupportsCudaRender(bool v)
  {
    try {

      _effectProps.propSetString(kOfxImageEffectPropCudaRenderSupported, (v ? "true" : "false"));
    } catch(OFX::Exception::PropertyUnknownToHost) {
      OFX::Log::warning(true, "Host does not have kOfxImageEffectPropCudaRenderSupported property");
    }
  }

  /** @brief Does the plugin support CUDA Stream */
  void ImageEffectDescriptor::setSupportsCudaStream(bool v)
  {
    try {
      _effectProps.propSetString(kOfxImageEffectPropCudaStreamSupported, (v ? "true" : "false"));
    } catch(OFX::Exception::PropertyUnknownToHost) {
      OFX::Log::warning(true, "Host does not have kOfxImageEffectPropCudaStreamSupported property");
    }
  }

  /** @brief Does the plugin support Metal Render */
  void ImageEffectDescriptor::setSupportsMetalRender(bool v)
  {
    try {
      _effectProps.propSetString(kOfxImageEffectPropMetalRenderSupported, (v ? "true" : "false"));
    } catch(OFX::Exception::PropertyUnknownToHost) {
      OFX::Log::warning(true, "Host does not have kOfxImageEffectPropMetalRenderSupported property");
    }
  }

#ifdef OFX_SUPPORTS_OPENGLRENDER
  /** @brief Does the plugin support OpenGL accelerated rendering (but is also capable of CPU rendering) ? */
  void ImageEffectDescriptor::setSupportsOpenGLRender(bool v) {
    if (gHostDescription.supportsOpenGLRender) {
      _effectProps.propSetString(kOfxImageEffectPropOpenGLRenderSupported, (v ? "true" : "false"));
    }
  }

  /** @brief Does the plugin require OpenGL accelerated rendering ? */
  void ImageEffectDescriptor::setNeedsOpenGLRender(bool v) {
    if (gHostDescription.supportsOpenGLRender) {
      _effectProps.propSetString(kOfxImageEffectPropOpenGLRenderSupported, (v ? "needed" : "false"));
    }
  }

  void ImageEffectDescriptor::addOpenGLBitDepth(BitDepthEnum v) {
    int n = _effectProps.propGetDimension(kOfxImageEffectPropSupportedPixelDepths);
    std::string value = mapBitDepthEnumToStr(v);
    if (!value.empty()) {
      _effectProps.propSetString(kOfxOpenGLPropPixelDepth, value, n);
    }
  }
#endif

  /** @brief If the slave param changes the clip preferences need to be re-evaluated */
  void ImageEffectDescriptor::addClipPreferencesSlaveParam(ParamDescriptor &p)
  {
    int n = _effectProps.propGetDimension(kOfxImageEffectPropClipPreferencesSlaveParam);
    _effectProps.propSetString(kOfxImageEffectPropClipPreferencesSlaveParam, p.getName(), n);
  }

  /** @brief Create a clip, only callable from describe in context */
  ClipDescriptor *ImageEffectDescriptor::defineClip(const std::string &name)
  {
    // do we have the clip already
    std::map<std::string, ClipDescriptor *>::const_iterator search;
    search = _definedClips.find(name);
    if(search != _definedClips.end())
      return search->second;

    // no, so make it
    OfxPropertySetHandle propSet;
    OfxStatus stat = OFX::Private::gEffectSuite->clipDefine(_effectHandle, name.c_str(), &propSet);
    (void)stat;

    ClipDescriptor *clip = new ClipDescriptor(name, propSet);

    _definedClips[name] = clip;
    _clipComponentsPropNames[name] = std::string("OfxImageClipPropComponents_") + name;
    _clipDepthPropNames[name] = std::string("OfxImageClipPropDepth_") + name;
    _clipPARPropNames[name] = std::string("OfxImageClipPropPAR_") + name;
    _clipROIPropNames[name] = std::string("OfxImageClipPropRoI_") + name;
    _clipFrameRangePropNames[name] = std::string("OfxImageClipPropFrameRange_") + name;
    return clip;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // wraps up an image  
  ImageBase::ImageBase(OfxPropertySetHandle props)
    : _imageProps(props)
  {
    OFX::Validation::validateImageBaseProperties(props);

    // and fetch all the properties
    _rowBytes         = _imageProps.propGetInt(kOfxImagePropRowBytes, /*throwOnFailure*/false); // not required for OpenCL Images
    _pixelAspectRatio = _imageProps.propGetDouble(kOfxImagePropPixelAspectRatio);;
      
    std::string str  = _imageProps.propGetString(kOfxImageEffectPropComponents);
    _pixelComponents = mapStrToPixelComponentEnum(str);

    switch (_pixelComponents) {
      case ePixelComponentAlpha:
        _pixelComponentCount = 1;
        break;
      case ePixelComponentNone:
        _pixelComponentCount = 0;
        break;
      case ePixelComponentRGB:
        _pixelComponentCount = 3;
        break;
      case ePixelComponentRGBA:
        _pixelComponentCount = 4;
        break;
      case ePixelComponentCustom:
      default:
        _pixelComponentCount = 0;
        break;
    }

    str = _imageProps.propGetString(kOfxImageEffectPropPixelDepth);
    _pixelDepth = mapStrToBitDepthEnum(str);

    // compute bytes per pixel
    _pixelBytes = _pixelComponentCount;

    switch(_pixelDepth) 
    {
    case eBitDepthNone   : _pixelBytes *= 0; break;
    case eBitDepthUByte  : _pixelBytes *= 1; break;
    case eBitDepthUShort : _pixelBytes *= 2; break;
    case eBitDepthHalf   : _pixelBytes *= 2; break;
    case eBitDepthFloat  : _pixelBytes *= 4; break;
    case eBitDepthCustom : _pixelBytes *= 0; break;
    }

    str = _imageProps.propGetString(kOfxImageEffectPropPreMultiplication);
    _preMultiplication =  mapStrToPreMultiplicationEnum(str);

    _regionOfDefinition.x1 = _imageProps.propGetInt(kOfxImagePropRegionOfDefinition, 0);
    _regionOfDefinition.y1 = _imageProps.propGetInt(kOfxImagePropRegionOfDefinition, 1);
    _regionOfDefinition.x2 = _imageProps.propGetInt(kOfxImagePropRegionOfDefinition, 2);
    _regionOfDefinition.y2 = _imageProps.propGetInt(kOfxImagePropRegionOfDefinition, 3);

    _bounds.x1 = _imageProps.propGetInt(kOfxImagePropBounds, 0);
    _bounds.y1 = _imageProps.propGetInt(kOfxImagePropBounds, 1);
    _bounds.x2 = _imageProps.propGetInt(kOfxImagePropBounds, 2);
    _bounds.y2 = _imageProps.propGetInt(kOfxImagePropBounds, 3);

    str = _imageProps.propGetString(kOfxImagePropField);
    if(str == kOfxImageFieldNone) {
      _field = eFieldNone;
    }
    else if(str == kOfxImageFieldBoth) {
      _field = eFieldBoth;
    }
    else if(str == kOfxImageFieldLower) {
      _field = eFieldLower;
    }
    else if(str == kOfxImageFieldUpper) {
      _field = eFieldLower;
    }
    else {
      OFX::Log::error(true, "Unknown field state '%s' reported on an image", str.c_str());
      _field = eFieldNone;
    }

    _uniqueID = _imageProps.propGetString(kOfxImagePropUniqueIdentifier);

    _renderScale.x = _imageProps.propGetDouble(kOfxImageEffectPropRenderScale, 0);
    _renderScale.y = _imageProps.propGetDouble(kOfxImageEffectPropRenderScale, 1);
  }

  ImageBase::~ImageBase()
  {
  }

  ////////////////////////////////////////////////////////////////////////////////
  // wraps up an image  
  Image::Image(OfxPropertySetHandle props)
    : ImageBase(props)
  {
    OFX::Validation::validateImageProperties(props);

    // and fetch all the properties
    _OpenCLImage = nullptr;
    _OpenCLImage = _imageProps.propGetPointer(kOfxImageEffectPropOpenCLImage, /*throwOnFailure*/false);
    // should throw if it is not an image
    _pixelData = _imageProps.propGetPointer(kOfxImagePropData, /*throwOnFailure*/!_OpenCLImage);
  }

  Image::~Image()
  {
    OFX::Private::gEffectSuite->clipReleaseImage(_imageProps.propSetHandle());
  }

#ifdef OFX_SUPPORTS_OPENGLRENDER
  ////////////////////////////////////////////////////////////////////////////////
  // wraps up an OpenGL texture
  Texture::Texture(OfxPropertySetHandle props)
    : ImageBase(props)
  {
    OFX::Validation::validateTextureProperties(props);

    // should throw if it is not a texture
    _index = _imageProps.propGetInt(kOfxImageEffectPropOpenGLTextureIndex);
    _target = _imageProps.propGetInt(kOfxImageEffectPropOpenGLTextureTarget);
  }

  Texture::~Texture()
  {
    OfxStatus stat = OFX::Private::gOpenGLRenderSuite->clipFreeTexture(_imageProps.propSetHandle());
    if (stat != kOfxStatOK) {
      throwSuiteStatusException(stat);
    }
  }
#endif

  /** @brief return a pixel pointer

  No attempt made to be uber efficient here.
  */
  void *Image::getPixelAddress(int x, int y)
  {
    // are we in the image bounds
    if(x < _bounds.x1 || x >= _bounds.x2 || y < _bounds.y1 || y >= _bounds.y2 || _pixelBytes == 0)
      return 0;

    char *pix = ((char *) _pixelData) + (size_t)(y - _bounds.y1) * _rowBytes;
    pix += (x - _bounds.x1) * _pixelBytes;
    return (void *) pix;   
  }

  const void *Image::getPixelAddress(int x, int y) const
  {
    // are we in the image bounds
    if(x < _bounds.x1 || x >= _bounds.x2 || y < _bounds.y1 || y >= _bounds.y2 || _pixelBytes == 0)
      return 0;

    const char *pix = ((const char *) _pixelData) + (size_t)(y - _bounds.y1) * _rowBytes;
    pix += (x - _bounds.x1) * _pixelBytes;
    return (const void *) pix;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // clip instance

  /** @brief hidden constructor */
  Clip::Clip(ImageEffect *effect, const std::string &name, OfxImageClipHandle handle, OfxPropertySetHandle props)
    : _clipName(name)
    , _clipProps(props)
    , _clipHandle(handle)
    , _effect(effect)
  {
    OFX::Validation::validateClipInstanceProperties(_clipProps);
  }

  /** @brief fetch the label */
  void Clip::getLabel(std::string &label) const
  {
    label      = _clipProps.propGetString(kOfxPropLabel);
  }

  /** @brief fetch the labels */
  void Clip::getLabels(std::string &label, std::string &shortLabel, std::string &longLabel) const
  {
    getLabel(label);
    shortLabel = _clipProps.propGetString(kOfxPropShortLabel, false);
    longLabel  = _clipProps.propGetString(kOfxPropLongLabel, false);
  }

  /** @brief get the pixel depth */
  BitDepthEnum Clip::getPixelDepth(void) const
  {
    std::string str = _clipProps.propGetString(kOfxImageEffectPropPixelDepth);
    BitDepthEnum e;
    try {
      e = mapStrToBitDepthEnum(str);
      if(e == eBitDepthNone && isConnected()) {
        OFX::Log::error(true, "Clip %s is connected and has no pixel depth.", _clipName.c_str());
      }
    }
    // gone wrong ?
    catch(std::invalid_argument&) {
      OFX::Log::error(true, "Unknown pixel depth property '%s' reported on clip '%s'", str.c_str(), _clipName.c_str());
      e = eBitDepthNone;
    }
    return e;
  }

  /** @brief get the components in the image */
  PixelComponentEnum Clip::getPixelComponents(void) const
  {
    std::string str = _clipProps.propGetString(kOfxImageEffectPropComponents);
    PixelComponentEnum e;
    try {
      e = mapStrToPixelComponentEnum(str);
      if(e == ePixelComponentNone && isConnected()) {
        OFX::Log::error(true, "Clip %s is connected and has no pixel component type!", _clipName.c_str());
      }
    }
    // gone wrong ?
    catch(std::invalid_argument&) {
      OFX::Log::error(true, "Unknown  pixel component type '%s' reported on clip '%s'", str.c_str(), _clipName.c_str());
      e = ePixelComponentNone;
    }
    return e;
  }

  /** @brief get the number of components in the image */
  int Clip::getPixelComponentCount(void) const
  {
    std::string str = _clipProps.propGetString(kOfxImageEffectPropComponents);
    PixelComponentEnum e;
    try {
      e = mapStrToPixelComponentEnum(str);
      if(e == ePixelComponentNone && isConnected()) {
        OFX::Log::error(true, "Clip %s is connected and has no pixel component type!", _clipName.c_str());
      }
    }
    // gone wrong ?
    catch(std::invalid_argument&) {
      OFX::Log::error(true, "Unknown  pixel component type '%s' reported on clip '%s'", str.c_str(), _clipName.c_str());
      e = ePixelComponentNone;
    }

    switch (e) {
      case ePixelComponentAlpha:
        return 1;
      case ePixelComponentNone:
        return 0;
      case ePixelComponentRGB:
        return 3;
      case ePixelComponentRGBA:
        return 4;
      case ePixelComponentCustom:
      default:
        return 0;
    }
  }

  /** @brief what is the actual pixel depth of the clip */
  BitDepthEnum Clip::getUnmappedPixelDepth(void) const
  {
    std::string str = _clipProps.propGetString(kOfxImageClipPropUnmappedPixelDepth);
    BitDepthEnum e;
    try {
      e = mapStrToBitDepthEnum(str);
      if(e == eBitDepthNone && !isConnected()) {
        OFX::Log::error(true, "Clip %s is connected and has no unmapped pixel depth.", _clipName.c_str());
      }
    }
    // gone wrong ?
    catch(std::invalid_argument&) {
      OFX::Log::error(true, "Unknown unmapped pixel depth property '%s' reported on clip '%s'", str.c_str(), _clipName.c_str());
      e = eBitDepthNone;
    }
    return e;
  }

  /** @brief what is the component type of the clip */
  PixelComponentEnum Clip::getUnmappedPixelComponents(void) const
  {
    std::string str = _clipProps.propGetString(kOfxImageClipPropUnmappedComponents);
    PixelComponentEnum e;
    try {
      e = mapStrToPixelComponentEnum(str);
      if(e == ePixelComponentNone && !isConnected()) {
        OFX::Log::error(true, "Clip %s is connected and has no unmapped pixel component type!", _clipName.c_str());
      }
    }
    // gone wrong ?
    catch(std::invalid_argument&) {
      OFX::Log::error(true, "Unknown unmapped pixel component type '%s' reported on clip '%s'", str.c_str(), _clipName.c_str());
      e = ePixelComponentNone;
    }
    return e;
  }

  /** @brief get the components in the image */
  PreMultiplicationEnum Clip::getPreMultiplication(void) const
  {
    std::string str = _clipProps.propGetString(kOfxImageEffectPropPreMultiplication);
    PreMultiplicationEnum e;
    try {
      e = mapStrToPreMultiplicationEnum(str);
    }
    // gone wrong ?
    catch(std::invalid_argument&) {
      OFX::Log::error(true, "Unknown premultiplication type '%s' reported on clip %s!", str.c_str(), _clipName.c_str());
      e = eImageOpaque;
    }
    return e;
  }

  /** @brief which spatial field comes first temporally */
  FieldEnum Clip::getFieldOrder(void) const
  {
    std::string str = _clipProps.propGetString(kOfxImageClipPropFieldOrder);
    FieldEnum e;
    try {
      e = mapStrToFieldEnum(str);
      OFX::Log::error(e != eFieldNone && e != eFieldLower && e != eFieldUpper, 
        "Field order '%s' reported on a clip %s is invalid, it must be none, lower or upper.", str.c_str(), _clipName.c_str());
    }
    // gone wrong ?
    catch(std::invalid_argument&) {
      OFX::Log::error(true, "Unknown field order '%s' reported on a clip %s.", str.c_str(), _clipName.c_str());
      e = eFieldNone;
    }
    return e;
  }

  /** @brief is the clip connected */
  bool Clip::isConnected(void) const
  {
    return _clipProps.propGetInt(kOfxImageClipPropConnected) != 0;
  }

  /** @brief can the clip be continuously sampled */
  bool Clip::hasContinuousSamples(void) const
  {
    return _clipProps.propGetInt(kOfxImageClipPropContinuousSamples) != 0;
  }

  /** @brief get the scale factor that has been applied to this clip */
  double Clip::getPixelAspectRatio(void) const
  {
    try {
      return _clipProps.propGetDouble(kOfxImagePropPixelAspectRatio);
    } catch(...) {
      return 1.0;  // This error could happen in Eyeon Fusion.
    }
  }

  /** @brief get the frame rate, in frames per second on this clip, after any clip preferences have been applied */
  double Clip::getFrameRate(void) const
  {
    return _clipProps.propGetDouble(kOfxImageEffectPropFrameRate);
  }

  /** @brief return the range of frames over which this clip has images, after any clip preferences have been applied */
  OfxRangeD Clip::getFrameRange(void) const
  {
    OfxRangeD v;
    v.min = _clipProps.propGetDouble(kOfxImageEffectPropFrameRange, 0);
    v.max = _clipProps.propGetDouble(kOfxImageEffectPropFrameRange, 1);
    return v;
  }

  /** @brief get the frame rate, in frames per second on this clip, before any clip preferences have been applied */
  double Clip::getUnmappedFrameRate(void) const
  {
    return _clipProps.propGetDouble(kOfxImageEffectPropUnmappedFrameRate);
  }

  /** @brief return the range of frames over which this clip has images, before any clip preferences have been applied */
  OfxRangeD Clip::getUnmappedFrameRange(void) const
  {
    OfxRangeD v;
    v.min = _clipProps.propGetDouble(kOfxImageEffectPropUnmappedFrameRange, 0);
    v.max = _clipProps.propGetDouble(kOfxImageEffectPropUnmappedFrameRange, 1);
    return v;
  }

  /** @brief get the RoD for this clip in the cannonical coordinate system */
  OfxRectD Clip::getRegionOfDefinition(double t)
  {
    OfxRectD bounds;
    OfxStatus stat = OFX::Private::gEffectSuite->clipGetRegionOfDefinition(_clipHandle, t, &bounds);
    if(stat == kOfxStatFailed) {
      bounds.x1 = bounds.x2 = bounds.y1 = bounds.y2 = 0;
    }
    throwSuiteStatusException(stat);
    return bounds;
  }

  /** @brief fetch an image */
  Image *Clip::fetchImage(double t)
  {
    OfxPropertySetHandle imageHandle;
    OfxStatus stat = OFX::Private::gEffectSuite->clipGetImage(_clipHandle, t, NULL, &imageHandle);
    if(stat == kOfxStatFailed) {
      return NULL; // not an error, fetched images out of range/region, assume black and transparent
    }
    else
      throwSuiteStatusException(stat);

    return new Image(imageHandle);
  }

  /** @brief fetch an image, with a specific region in cannonical coordinates */
  Image *Clip::fetchImage(double t, const OfxRectD &bounds)
  {
    OfxPropertySetHandle imageHandle;
    OfxStatus stat = OFX::Private::gEffectSuite->clipGetImage(_clipHandle, t, &bounds, &imageHandle);
    if(stat == kOfxStatFailed) {
      return NULL; // not an error, fetched images out of range/region, assume black and transparent
    }
    else
      throwSuiteStatusException(stat);

    return new Image(imageHandle);
  }

#ifdef OFX_SUPPORTS_OPENGLRENDER
  Texture *Clip::loadTexture(double t, BitDepthEnum format, const OfxRectD *region)
  {
    if (!gHostDescription.supportsOpenGLRender) {
      throwHostMissingSuiteException("loadTexture");
    }
    OfxPropertySetHandle hTex;
    OfxStatus stat = Private::gOpenGLRenderSuite->clipLoadTexture(_clipHandle, t, format == eBitDepthNone ? NULL : mapBitDepthEnumToStr(format), region, &hTex);
    if (stat != kOfxStatOK) {
      throwSuiteStatusException(stat);
    }
    return new Texture(hTex);
  }
#endif
  ////////////////////////////////////////////////////////////////////////////////
  /// image effect 

  /** @brief ctor */
  ImageEffect::ImageEffect(OfxImageEffectHandle handle)
    : _effectHandle(handle)
    , _effectProps(0)
    , _context(eContextNone)
    , _progressStartSuccess(false)
  {
    // get the property handle
    _effectProps = OFX::Private::fetchEffectProps(handle);

    // Set this as the instance data pointer on the effect handle
    _effectProps.propSetPointer(kOfxPropInstanceData, this);

    // validate the plugin instance
    OFX::Validation::validatePluginInstanceProperties(_effectProps);

    // fetch the context
    std::string ctxt = _effectProps.propGetString(kOfxImageEffectPropContext);
    _context = mapToContextEnum(ctxt);

    // the param set daddy-oh
    OfxParamSetHandle paramSet;
    OfxStatus stat = OFX::Private::gEffectSuite->getParamSet(handle, &paramSet);
    throwSuiteStatusException(stat);
    setParamSetHandle(paramSet);      

  }

  /** @brief dtor */
  ImageEffect::~ImageEffect()
  {
    // clobber the instance data property on the effect handle
    _effectProps.propSetPointer(kOfxPropInstanceData, 0);

    // delete any clip instances we may have constructed
    std::map<std::string, Clip *>::iterator iter;
    for(iter = _fetchedClips.begin(); iter != _fetchedClips.end(); ++iter) {
      if(iter->second) {
        delete iter->second;
        iter->second = NULL;
      }
    }
  }

  /** @brief the context this effect was instantiate in */
  ContextEnum ImageEffect::getContext(void) const
  {
    return _context;
  }

  /** @brief size of the project */
  OfxPointD ImageEffect::getProjectSize(void) const
  {
    OfxPointD v;    
    v.x = _effectProps.propGetDouble(kOfxImageEffectPropProjectSize, 0);
    v.y = _effectProps.propGetDouble(kOfxImageEffectPropProjectSize, 1);
    return v;
  }

  /** @brief origin of the project */
  OfxPointD ImageEffect::getProjectOffset(void) const
  {
    OfxPointD v;    
    v.x = _effectProps.propGetDouble(kOfxImageEffectPropProjectOffset, 0);
    v.y = _effectProps.propGetDouble(kOfxImageEffectPropProjectOffset, 1);
    return v;
  }

  /** @brief extent of the project */
  OfxPointD ImageEffect::getProjectExtent(void) const
  {
    OfxPointD v;    
    v.x = _effectProps.propGetDouble(kOfxImageEffectPropProjectExtent, 0);
    v.y = _effectProps.propGetDouble(kOfxImageEffectPropProjectExtent, 1);
    return v;
  }

  /** @brief pixel aspect ratio of the project */
  double ImageEffect::getProjectPixelAspectRatio(void) const
  {
    return _effectProps.propGetDouble(kOfxImageEffectPropProjectPixelAspectRatio, 0);
  }

  /** @brief how long does the effect last */
  double ImageEffect::getEffectDuration(void) const
  {
    return _effectProps.propGetDouble(kOfxImageEffectInstancePropEffectDuration, 0);
  }

  /** @brief the frame rate of the project */
  double ImageEffect::getFrameRate(void) const
  {
    return _effectProps.propGetDouble(kOfxImageEffectPropFrameRate, 0);
  }

  /** @brief is the instance currently being interacted with */
  bool ImageEffect::isInteractive(void) const
  {
    return _effectProps.propGetInt(kOfxPropIsInteractive) != 0;
  }

  /** @brief set the instance to be sequentially renderred, this should have been part of clip preferences! */
  void ImageEffect::setSequentialRender(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectInstancePropSequentialRender, int(v));
  }

  /** @brief Have we informed the host we want to be seqentially renderred ? */
  bool ImageEffect::getSequentialRender(void) const
  {
    return _effectProps.propGetInt(kOfxImageEffectInstancePropSequentialRender) != 0;
  }

  /** @brief Does the plugin support image tiling ? Can only be called from changedParam or changedClip. */
  void ImageEffect::setSupportsTiles(bool v)
  {
    _effectProps.propSetInt(kOfxImageEffectPropSupportsTiles, int(v), false); // read/write from OFX 1.4
  }

  /** @brief Have we informed the host we support image tiling ? */
  bool ImageEffect::getSupportsTiles(void) const
  {
    return _effectProps.propGetInt(kOfxImageEffectPropSupportsTiles) != 0;
  }


#ifdef OFX_SUPPORTS_OPENGLRENDER
  /** @brief Does the plugin support OpenGL accelerated rendering (but is also capable of CPU rendering) ? Can only be called from changedParam or changedClip. */
  void ImageEffect::setSupportsOpenGLRender(bool v) {
    if (gHostDescription.supportsOpenGLRender) {
      _effectProps.propSetString(kOfxImageEffectPropOpenGLRenderSupported, (v ? "true" : "false"), false); // read/write from OFX 1.4
    }
  }

  /** @brief Does the plugin require OpenGL accelerated rendering ? Can only be called from changedParam or changedClip. */
  void ImageEffect::setNeedsOpenGLRender(bool v) {
    if (gHostDescription.supportsOpenGLRender) {
      _effectProps.propSetString(kOfxImageEffectPropOpenGLRenderSupported, (v ? "needed" : "false"), false); // read/write from OFX 1.4
    }
  }
#endif

  /** @brief notify host that the internal data structures need syncing back to parameters for persistance and so on.  This is reset by the host after calling SyncPrivateData. */
  void ImageEffect::setParamSetNeedsSyncing()
  {
    _effectProps.propSetInt(kOfxPropParamSetNeedsSyncing, 1, false); // introduced in OFX 1.2
  }

  OFX::Message::MessageReplyEnum ImageEffect::sendMessage(OFX::Message::MessageTypeEnum type, const std::string& id, const std::string& msg)
  {   
    if(!OFX::Private::gMessageSuite){ throwHostMissingSuiteException("message"); }
    if(!OFX::Private::gMessageSuite->message){ throwHostMissingSuiteException("message"); }
    OfxStatus stat = OFX::Private::gMessageSuite->message(_effectHandle, mapMessageTypeEnumToStr(type), id.c_str(), msg.c_str());
    return mapToMessageReplyEnum(stat);
  }

  OFX::Message::MessageReplyEnum ImageEffect::setPersistentMessage(OFX::Message::MessageTypeEnum type, const std::string& id, const std::string& msg)
  {   
    if(!OFX::Private::gMessageSuiteV2){ throwHostMissingSuiteException("setPersistentMessage"); }
    if(!OFX::Private::gMessageSuiteV2->setPersistentMessage){ throwHostMissingSuiteException("setPersistentMessage"); }
    OfxStatus stat = OFX::Private::gMessageSuiteV2->setPersistentMessage(_effectHandle, mapMessageTypeEnumToStr(type), id.c_str(), msg.c_str());
    return mapToMessageReplyEnum(stat);
  }

  OFX::Message::MessageReplyEnum ImageEffect::clearPersistentMessage()
  {   
    if(!OFX::Private::gMessageSuiteV2){ throwHostMissingSuiteException("clearPersistentMessage"); }
    if(!OFX::Private::gMessageSuiteV2->clearPersistentMessage){ throwHostMissingSuiteException("clearPersistentMessage"); }
    OfxStatus stat = OFX::Private::gMessageSuiteV2->clearPersistentMessage(_effectHandle);
    return mapToMessageReplyEnum(stat);
  }

  /** @brief Fetch the named clip from this instance */
  Clip *ImageEffect::fetchClip(const std::string &name)
  {
    // do we have the clip already
    std::map<std::string, Clip *>::const_iterator search;
    search = _fetchedClips.find(name);
    if(search != _fetchedClips.end())
      return search->second;

    // fetch the property set handle of the effect
    OfxImageClipHandle clipHandle = 0;
    OfxPropertySetHandle propHandle = 0;
    OfxStatus stat = OFX::Private::gEffectSuite->clipGetHandle(_effectHandle, name.c_str(), &clipHandle, &propHandle);
    throwSuiteStatusException(stat);

    // and make one
    Clip *newClip = new Clip(this, name, clipHandle, propHandle);

    // add it in
    _fetchedClips[name] = newClip;

    // return it
    return newClip;
  }

  /** @brief does the host want us to abort rendering? */
  bool ImageEffect::abort(void) const
  {
    return OFX::Private::gEffectSuite->abort(_effectHandle) != 0;
  }

  /** @brief adds a new interact to the set of interacts open on this effect */
  void ImageEffect::addOverlayInteract(OverlayInteract *interact)
  {
    // do we have it already ?
    std::list<OverlayInteract *>::iterator i;
    i = std::find(_overlayInteracts.begin(), _overlayInteracts.end(), interact);

    // we don't, put it in there
    if(i == _overlayInteracts.end()) {
      // we have a new one to add in here
      _overlayInteracts.push_back(interact);
    }
  }

  /** @brief removes an interact to the set of interacts open on this effect */
  void ImageEffect::removeOverlayInteract(OverlayInteract *interact)
  {
    // find it
    std::list<OverlayInteract *>::iterator i;
    i = std::find(_overlayInteracts.begin(), _overlayInteracts.end(), interact);

    // and remove it
    if(i != _overlayInteracts.end()) {
      _overlayInteracts.erase(i);
    }
  }

  /** @brief force all overlays on this interact to be redrawn */
  void ImageEffect::redrawOverlays(void)
  {
    // find it
    std::list<OverlayInteract *>::iterator i;
    for(i = _overlayInteracts.begin(); i != _overlayInteracts.end(); ++i) {
      (*i)->requestRedraw();
    }
  }

#ifdef OFX_SUPPORTS_OPENGLRENDER
    bool ImageEffect::flushOpenGLResources(void)
    {
      if (!gHostDescription.supportsOpenGLRender) {
        return false;
      }
      return Private::gOpenGLRenderSuite->flushResources() == kOfxStatOK;
    }
#endif

  ////////////////////////////////////////////////////////////////////////////////
  // below are the default members for the base image effect


  /** @brief client is identity function, returns the clip and time for the identity function 
  */
  bool ImageEffect::isIdentity(const IsIdentityArguments &/*args*/, Clip * &/*identityClip*/, double &/*identityTime*/)
  {
    return false; // by default, we are not an identity operation
  }

  /** @brief The get RoD action */
  bool ImageEffect::getRegionOfDefinition(const RegionOfDefinitionArguments &/*args*/, OfxRectD &/*rod*/)
  {
    return false; // by default, we are not setting the RoD
  }

  /** @brief the get RoI action */
  void ImageEffect::getRegionsOfInterest(const RegionsOfInterestArguments &/*args*/, RegionOfInterestSetter &/*rois*/)
  {
    // fa niente
  }

  /** @brief the get frames needed action */
  void ImageEffect::getFramesNeeded(const FramesNeededArguments &/*args*/, FramesNeededSetter &/*frames*/)
  {
    // fa niente
  }

  /** @brief client begin sequence render function */
  void ImageEffect::beginSequenceRender(const BeginSequenceRenderArguments &/*args*/)
  {
    // fa niente
  }

  /** @brief client end sequence render function, this is one of the few that must be set */
  void ImageEffect::endSequenceRender(const EndSequenceRenderArguments &/*args*/)
  {
    // fa niente
  }

  /** @brief The purge caches action, a request for an instance to free up as much memory as possible in low memory situations */
  void ImageEffect::purgeCaches(void)
  {
    // fa niente
  }

  /** @brief The sync private data action, called when the effect needs to sync any private data to persistant parameters */
  void ImageEffect::syncPrivateData(void)
  {
    // fa niente
  }

  /** @brief get the clip preferences */
  void ImageEffect::getClipPreferences(ClipPreferencesSetter &/*clipPreferences*/)
  {
    // fa niente
  }

  /** @brief the effect is about to be actively edited by a user, called when the first user interface is opened on an instance */
  void ImageEffect::beginEdit(void)
  {
    // fa niente
  }

  /** @brief the effect is no longer being edited by a user, called when the last user interface is closed on an instance */
  void ImageEffect::endEdit(void)
  {
    // fa niente
  }

  /** @brief the effect is about to have some values changed */
  void ImageEffect::beginChanged(InstanceChangeReason /*reason*/)
  {
  }

  /** @brief called when a param has just had its value changed */
  void ImageEffect::changedParam(const InstanceChangedArgs &/*args*/, const std::string &/*paramName*/)
  {
  }

  /** @brief called when a clip has just been changed in some way (a rewire maybe) */
  void ImageEffect::changedClip(const InstanceChangedArgs &/*args*/, const std::string &/*clipName*/)
  {
  }

  /** @brief the effect has just had some values changed */
  void ImageEffect::endChanged(InstanceChangeReason /*reason*/)
  {
  }

  /** @brief get the time domain */
  bool ImageEffect::getTimeDomain(OfxRangeD &/*range*/)
  {
    // by default, do the default
    return false;
  }

#ifdef OFX_SUPPORTS_OPENGLRENDER
  /** @brief OpenGL context attached */
  void ImageEffect::contextAttached(void)
  {
    // fa niente
  }

  /** @brief OpenGL context detached */
  void ImageEffect::contextDetached(void)
  {
    // fa niente
  }
#endif

  /** @brief called when a custom param needs to be interpolated */
  std::string ImageEffect::interpolateCustomParam(const InterpolateCustomArgs &args, const std::string &/*paramName*/)
  {
      return args.value1;
  }

  /// Start doing progress. 
  void ImageEffect::progressStart(const std::string &message, const std::string &messageid)
  {
    if(OFX::Private::gProgressSuiteV2) {
      OfxStatus stat = OFX::Private::gProgressSuiteV2->progressStart((void *) _effectHandle, message.c_str(), messageid.c_str());
      _progressStartSuccess = ( stat == kOfxStatOK );
    } else if(OFX::Private::gProgressSuiteV1) {
      OfxStatus stat = OFX::Private::gProgressSuiteV1->progressStart((void *) _effectHandle, message.c_str());
      _progressStartSuccess = ( stat == kOfxStatOK );
    }

  }

  /// finish yer progress
  void ImageEffect::progressEnd()
  {
    if(_progressStartSuccess) {
      if(OFX::Private::gProgressSuiteV2) {
        OFX::Private::gProgressSuiteV2->progressEnd((void *) _effectHandle);
      } else if(OFX::Private::gProgressSuiteV1) {
        OFX::Private::gProgressSuiteV1->progressEnd((void *) _effectHandle);
      }
    }
  }

  /// set the progress to some level of completion, returns
  /// false if you should abandon processing, true to continue
  bool ImageEffect::progressUpdate(double t)
  {
    if(_progressStartSuccess) {
      if(OFX::Private::gProgressSuiteV2) {
        OfxStatus stat = OFX::Private::gProgressSuiteV2->progressUpdate((void *) _effectHandle, t);
        if(stat == kOfxStatReplyNo)
          return false;
      } else if(OFX::Private::gProgressSuiteV1) {
        OfxStatus stat = OFX::Private::gProgressSuiteV1->progressUpdate((void *) _effectHandle, t);
        if(stat == kOfxStatReplyNo)
          return false;
      }
    }
    return true;
  }

  /// get the current time on the timeline. This is not necessarily the same
  /// time as being passed to an action (eg render)
  double ImageEffect::timeLineGetTime()
  {
    if(OFX::Private::gTimeLineSuite) {
      double time;
      if(OFX::Private::gTimeLineSuite->getTime((void *) _effectHandle, &time) == kOfxStatOK)
        return time;
    }
    return 0;
  }

  /// set the timeline to a specific time
  void ImageEffect::timeLineGotoTime(double t)
  {
    if(OFX::Private::gTimeLineSuite) {
      OFX::Private::gTimeLineSuite->gotoTime((void *) _effectHandle, t);
    }
  }

  /// get the first and last times available on the effect's timeline
  void ImageEffect:: timeLineGetBounds(double &t1, double &t2)
  {
    if(OFX::Private::gTimeLineSuite) {
      OFX::Private::gTimeLineSuite->getTimeBounds((void *) _effectHandle, &t1, &t2);
      return;
    }
    t1 = t2 = 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Class used to set the clip preferences of the effect. */ 

  const std::string& ClipPreferencesSetter::extractValueForName(const StringStringMap& m, const std::string& name)
  {
    StringStringMap::const_iterator it = m.find(name);
    if(it==m.end())
      throw(Exception::PropertyUnknownToHost(name.c_str()));
    return it->second;
  }

  /** @brief, force the host to set a clip's mapped component type to be \em comps.  */
  void ClipPreferencesSetter::setClipComponents(Clip &clip, PixelComponentEnum comps)
  {
    doneSomething_ = true;
    const std::string& propName = extractValueForName(clipComponentPropNames_, clip.name());

    switch(comps) 
    {
    case ePixelComponentNone :
      outArgs_.propSetString(propName.c_str(), kOfxImageComponentNone);
      break;
    case ePixelComponentRGBA : 
      outArgs_.propSetString(propName.c_str(), kOfxImageComponentRGBA); 
      break;
    case ePixelComponentRGB :
      outArgs_.propSetString(propName.c_str(), kOfxImageComponentRGB);
      break;
    case ePixelComponentAlpha : 
      outArgs_.propSetString(propName.c_str(), kOfxImageComponentAlpha); 
      break;
    case ePixelComponentCustom :
      break;
    }
  }

  /** @brief, force the host to set a clip's mapped bit depth be \em bitDepth */
  void ClipPreferencesSetter::setClipBitDepth(Clip &clip, BitDepthEnum bitDepth)
  {
    doneSomething_ = true;
    const std::string& propName = extractValueForName(clipDepthPropNames_, clip.name());

    switch(bitDepth) 
    {
    case eBitDepthNone :
      outArgs_.propSetString(propName.c_str(), kOfxBitDepthNone);
      break;
    case eBitDepthUByte : 
      outArgs_.propSetString(propName.c_str(), kOfxBitDepthByte); 
      break;
    case eBitDepthUShort : 
      outArgs_.propSetString(propName.c_str(), kOfxBitDepthShort); 
      break;
    case eBitDepthHalf :
      outArgs_.propSetString(propName.c_str(), kOfxBitDepthHalf);
      break;
    case eBitDepthFloat :
      outArgs_.propSetString(propName.c_str(), kOfxBitDepthFloat); 
      break;
    case eBitDepthCustom :
      break;
    }
  }

  /** @brief, force the host to set a clip's mapped Pixel Aspect Ratio to be \em PAR */
  void ClipPreferencesSetter::setPixelAspectRatio(Clip &clip, double PAR)
  {
    doneSomething_ = true;
    const std::string& propName = extractValueForName(clipPARPropNames_, clip.name());
    outArgs_.propSetDouble(propName.c_str(), PAR);
  }

  /** @brief Allows an effect to change the output frame rate */
  void ClipPreferencesSetter::setOutputFrameRate(double v)
  {
    doneSomething_ = true;
    outArgs_.propSetDouble(kOfxImageEffectPropFrameRate, v);
  }

  /** @brief Set the premultiplication state of the output clip. */
  void ClipPreferencesSetter::setOutputPremultiplication(PreMultiplicationEnum v)
  {
    doneSomething_ = true;
    switch(v) 
    {
    case eImageOpaque : 
      outArgs_.propSetString(kOfxImageEffectPropPreMultiplication, kOfxImageOpaque); 
      break;
    case eImagePreMultiplied: 
      outArgs_.propSetString(kOfxImageEffectPropPreMultiplication, kOfxImagePreMultiplied); 
      break;
    case eImageUnPreMultiplied:  
      outArgs_.propSetString(kOfxImageEffectPropPreMultiplication, kOfxImageUnPreMultiplied); 
      break;
    }
  }

  /** @brief Set whether the effect can be continously sampled. */
  void ClipPreferencesSetter::setOutputHasContinousSamples(bool v)
  {
    doneSomething_ = true;
    outArgs_.propSetInt(kOfxImageClipPropContinuousSamples, int(v));
  }

  /** @brief Sets whether the effect will produce different images in all frames, even if the no params or input images are varying (eg: a noise generator). */
  void ClipPreferencesSetter::setOutputFrameVarying(bool v)
  {
    doneSomething_ = true;
    outArgs_.propSetInt(kOfxImageEffectFrameVarying, int(v));
  }


  void  ClipPreferencesSetter::setOutputFielding(FieldEnum v)
  {
    doneSomething_ = true;
    switch(v) 
    {
    case eFieldNone : outArgs_.propSetString(kOfxImageClipPropFieldOrder, kOfxImageFieldNone, 0, false); break;
    case eFieldLower : outArgs_.propSetString(kOfxImageClipPropFieldOrder, kOfxImageFieldLower, 0, false); break;
    case eFieldUpper : outArgs_.propSetString(kOfxImageClipPropFieldOrder, kOfxImageFieldUpper, 0, false); break;
    case eFieldBoth : outArgs_.propSetString(kOfxImageClipPropFieldOrder, kOfxImageFieldBoth, 0, false); break;
    case eFieldSingle : outArgs_.propSetString(kOfxImageClipPropFieldOrder, kOfxImageFieldSingle, 0, false); break;
    case eFieldDoubled : outArgs_.propSetString(kOfxImageClipPropFieldOrder, kOfxImageFieldDoubled, 0, false); break;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /** @brief Class that skins image memory allocation */

  /** @brief ctor */
  ImageMemory::ImageMemory(size_t nBytes, ImageEffect *associatedEffect)
    : _handle(0)
  {
    OfxImageEffectHandle effectHandle = 0;
    if(associatedEffect != 0) {
      effectHandle = associatedEffect->_effectHandle;
    }

    OfxStatus stat = OFX::Private::gEffectSuite->imageMemoryAlloc(effectHandle, nBytes, &_handle);
    if(stat == kOfxStatErrMemory)
      throw std::bad_alloc();
    throwSuiteStatusException(stat);
  }

  /** @brief dtor */
  ImageMemory::~ImageMemory()
  {
    OfxStatus stat = OFX::Private::gEffectSuite->imageMemoryFree(_handle);
    // ignore status code for exception purposes
    (void)stat;
  }

  /** @brief lock the memory and return a pointer to it */
  void *ImageMemory::lock(void)
  {
    void *ptr;
    OfxStatus stat = OFX::Private::gEffectSuite->imageMemoryLock(_handle, &ptr);
    if(stat == kOfxStatErrMemory)
      throw std::bad_alloc();
    throwSuiteStatusException(stat);
    return ptr;
  }

  /** @brief unlock the memory */
  void ImageMemory::unlock(void)
  {
    OfxStatus stat = OFX::Private::gEffectSuite->imageMemoryUnlock(_handle);
    (void)stat;
  }



  /** @brief OFX::Private namespace, for things private to the support library code here generally calls image effect class members */
  namespace Private {

    /** @brief Creates the global host description and sets its properties */
    static
    void
      fetchHostDescription(OfxHost *host)
    {
      OFX::Log::error(OFX::gHostDescriptionHasInit, "Tried to create host description when we already have one.");
      if(!OFX::gHostDescriptionHasInit) {
        OFX::gHostDescriptionHasInit = true;
        // wrap the property handle up with a property set
        PropertySet hostProps(host->host);

        // and get some properties
        gHostDescription.APIVersionMajor            = hostProps.propGetInt(kOfxPropAPIVersion, 0, false); // OFX 1.2
        if (gHostDescription.APIVersionMajor == 0) {
          // assume OFX 1.0
          gHostDescription.APIVersionMajor = 1;
        }
        gHostDescription.APIVersionMinor            = hostProps.propGetInt(kOfxPropAPIVersion, 1, false); // OFX 1.2
        gHostDescription.hostName                   = hostProps.propGetString(kOfxPropName);
        gHostDescription.hostLabel                  = hostProps.propGetString(kOfxPropLabel);
        gHostDescription.versionMajor               = hostProps.propGetInt(kOfxPropVersion, 0, false); // OFX 1.2
        gHostDescription.versionMinor               = hostProps.propGetInt(kOfxPropVersion, 1, false); // OFX 1.2
        gHostDescription.versionMicro               = hostProps.propGetInt(kOfxPropVersion, 2, false); // OFX 1.2
        gHostDescription.versionLabel               = hostProps.propGetString(kOfxPropVersionLabel, false); // OFX 1.2
        gHostDescription.hostIsBackground           = hostProps.propGetInt(kOfxImageEffectHostPropIsBackground) != 0;
        gHostDescription.supportsOverlays           = hostProps.propGetInt(kOfxImageEffectPropSupportsOverlays) != 0;
        gHostDescription.supportsMultiResolution    = hostProps.propGetInt(kOfxImageEffectPropSupportsMultiResolution) != 0;
        gHostDescription.supportsTiles              = hostProps.propGetInt(kOfxImageEffectPropSupportsTiles) != 0;
        gHostDescription.temporalClipAccess         = hostProps.propGetInt(kOfxImageEffectPropTemporalClipAccess) != 0;
        gHostDescription.supportsMultipleClipDepths = hostProps.propGetInt(kOfxImageEffectPropSupportsMultipleClipDepths) != 0;
        gHostDescription.supportsMultipleClipPARs   = hostProps.propGetInt(kOfxImageEffectPropSupportsMultipleClipPARs) != 0;
        gHostDescription.supportsSetableFrameRate   = hostProps.propGetInt(kOfxImageEffectPropSetableFrameRate) != 0;
        gHostDescription.supportsSetableFielding    = hostProps.propGetInt(kOfxImageEffectPropSetableFielding) != 0;
        gHostDescription.sequentialRender           = hostProps.propGetInt(kOfxImageEffectInstancePropSequentialRender, false); // appeared in OFX 1.2
        gHostDescription.supportsStringAnimation    = hostProps.propGetInt(kOfxParamHostPropSupportsStringAnimation) != 0;
        gHostDescription.supportsCustomInteract     = hostProps.propGetInt(kOfxParamHostPropSupportsCustomInteract) != 0;
        gHostDescription.supportsChoiceAnimation    = hostProps.propGetInt(kOfxParamHostPropSupportsChoiceAnimation) != 0;
        // As of 1.5
        gHostDescription.supportsStrChoice          = hostProps.propGetInt(kOfxParamHostPropSupportsStrChoice, false) != 0;
        // As of 1.5
        gHostDescription.supportsStrChoiceAnimation = hostProps.propGetInt(kOfxParamHostPropSupportsStrChoiceAnimation, false) != 0;
        gHostDescription.supportsBooleanAnimation   = hostProps.propGetInt(kOfxParamHostPropSupportsBooleanAnimation) != 0;
        gHostDescription.supportsCustomAnimation    = hostProps.propGetInt(kOfxParamHostPropSupportsCustomAnimation) != 0;
        gHostDescription.osHandle                   = hostProps.propGetPointer(kOfxPropHostOSHandle, false);
        gHostDescription.supportsParametricParameter = gParametricParameterSuite != 0;
        gHostDescription.supportsParametricAnimation = hostProps.propGetInt(kOfxParamHostPropSupportsParametricAnimation, false) != 0;
        gHostDescription.supportsOpenCLRender        = hostProps.propGetString(kOfxImageEffectPropOpenCLRenderSupported, 0, false) == "true";
        gHostDescription.supportsCudaRender          = hostProps.propGetString(kOfxImageEffectPropCudaRenderSupported, 0, false) == "true";
        gHostDescription.supportsCudaStream          = hostProps.propGetString(kOfxImageEffectPropCudaStreamSupported, 0, false) == "true";
        gHostDescription.supportsMetalRender         = hostProps.propGetString(kOfxImageEffectPropMetalRenderSupported, 0, false) == "true";
        gHostDescription.supportsRenderQualityDraft = hostProps.propGetInt(kOfxImageEffectPropRenderQualityDraft, false) != 0; // appeared in OFX 1.4
        {
            std::string originStr = hostProps.propGetString(kOfxImageEffectHostPropNativeOrigin, false); // appeared in OFX 1.4
          if (originStr.empty()) {
            // from http://openeffects.org/standard_changes/host-origin-hints :
            // "All this hint does is tell plugin that the host world is different
            // than OFX. Historically the first two hosts that exhibited this issue
            // could be Fusion (upper left is 0,0 natively) and Toxic (Center is 0,0)."
            if (gHostDescription.hostName == "com.eyeonline.Fusion" ||
                ends_with(gHostDescription.hostName, "Fusion")) {
              // if host is Fusion, set to TopLeft
              gHostDescription.nativeOrigin = eNativeOriginTopLeft;
            } else if (starts_with(gHostDescription.hostName, "Autodesk Toxik") ||
                       ends_with(gHostDescription.hostName, "Toxik")) {
              // if host is Toxic, set to Center
              gHostDescription.nativeOrigin = eNativeOriginCenter;
            } else {
              gHostDescription.nativeOrigin = eNativeOriginBottomLeft;
            }
          } else if (originStr == kOfxHostNativeOriginBottomLeft) {
            gHostDescription.nativeOrigin = eNativeOriginBottomLeft;
          } else if (originStr == kOfxHostNativeOriginTopLeft) {
            gHostDescription.nativeOrigin = eNativeOriginTopLeft;
          } else if (originStr == kOfxHostNativeOriginCenter) {
            gHostDescription.nativeOrigin = eNativeOriginCenter;
          }
        }
#ifdef OFX_SUPPORTS_OPENGLRENDER
        gHostDescription.supportsOpenGLRender = gOpenGLRenderSuite != 0 && hostProps.propGetString(kOfxImageEffectPropOpenGLRenderSupported, 0, false) == "true";
#endif
        gHostDescription.maxParameters              = hostProps.propGetInt(kOfxParamHostPropMaxParameters);
        gHostDescription.maxPages                   = hostProps.propGetInt(kOfxParamHostPropMaxPages);
        gHostDescription.pageRowCount               = hostProps.propGetInt(kOfxParamHostPropPageRowColumnCount, 0);
        gHostDescription.pageColumnCount            = hostProps.propGetInt(kOfxParamHostPropPageRowColumnCount, 1);

        int numComponents = hostProps.propGetDimension(kOfxImageEffectPropSupportedComponents);
        for(int i=0; i<numComponents; ++i)
          gHostDescription._supportedComponents.push_back(mapStrToPixelComponentEnum(hostProps.propGetString(kOfxImageEffectPropSupportedComponents, i)));

        int numContexts = hostProps.propGetDimension(kOfxImageEffectPropSupportedContexts);
        for(int i=0; i<numContexts; ++i)
          gHostDescription._supportedContexts.push_back(mapToContextEnum(hostProps.propGetString(kOfxImageEffectPropSupportedContexts, i)));

        int numPixelDepths = hostProps.propGetDimension(kOfxImageEffectPropSupportedPixelDepths);
        for(int i=0; i<numPixelDepths; ++i)
          gHostDescription._supportedPixelDepths.push_back(mapStrToBitDepthEnum(hostProps.propGetString(kOfxImageEffectPropSupportedPixelDepths, i)));
      }
        
    }

    /** @brief fetch the effect property set from the ImageEffectHandle */
    OFX::PropertySet
      fetchEffectProps(OfxImageEffectHandle handle)
    {
      // get the property handle
      OfxPropertySetHandle propHandle;
      OfxStatus stat = OFX::Private::gEffectSuite->getPropertySet(handle, &propHandle);
      throwSuiteStatusException(stat);
      return OFX::PropertySet(propHandle);
    }

    /** @brief Keeps count of how many times load/unload have been called */
    int gLoadCount = 0;

    /** @brief Library side load action, this fetches all the suite pointers */
    void loadAction(void)
    {
      gLoadCount++;  

      //OfxStatus status = kOfxStatOK;

      // fetch the suites
      OFX::Log::error(gHost == 0, "Host pointer has not been set.");
      if(!gHost) throw OFX::Exception::Suite(kOfxStatErrBadHandle);

      if(gLoadCount == 1) {
        gEffectSuite    = (OfxImageEffectSuiteV1 *) fetchSuite(kOfxImageEffectSuite, 1);
        gPropSuite      = (OfxPropertySuiteV1 *)    fetchSuite(kOfxPropertySuite, 1);
        gParamSuite     = (OfxParameterSuiteV1 *)   fetchSuite(kOfxParameterSuite, 1);
        gMemorySuite    = (OfxMemorySuiteV1 *)      fetchSuite(kOfxMemorySuite, 1);
        gThreadSuite    = (OfxMultiThreadSuiteV1 *) fetchSuite(kOfxMultiThreadSuite, 1);
        gMessageSuite   = (OfxMessageSuiteV1 *)     fetchSuite(kOfxMessageSuite, 1);
        gMessageSuiteV2 = (OfxMessageSuiteV2 *)     fetchSuite(kOfxMessageSuite, 2, true);
        gProgressSuiteV1 = (OfxProgressSuiteV1 *)     fetchSuite(kOfxProgressSuite, 1, true);
        gProgressSuiteV2 = (OfxProgressSuiteV2 *)     fetchSuite(kOfxProgressSuite, 2, true);
        gTimeLineSuite   = (OfxTimeLineSuiteV1 *)     fetchSuite(kOfxTimeLineSuite, 1, true);
        gParametricParameterSuite = (OfxParametricParameterSuiteV1*) fetchSuite(kOfxParametricParameterSuite, 1, true);
#ifdef OFX_SUPPORTS_OPENGLRENDER
        gOpenGLRenderSuite = (OfxImageEffectOpenGLRenderSuiteV1*) fetchSuite(kOfxOpenGLRenderSuite, 1, true);
#endif

        // OK check and fetch host information
        fetchHostDescription(gHost);

        /// and set some dendent flags
        OFX::gHostDescription.supportsMessageSuiteV2 = gMessageSuiteV2 != NULL;
        OFX::gHostDescription.supportsProgressSuite = (gProgressSuiteV1 != NULL || gProgressSuiteV2 != NULL);
        OFX::gHostDescription.supportsTimeLineSuite = gTimeLineSuite != NULL;

        // fetch the interact suite if the host supports interaction
        if(OFX::gHostDescription.supportsOverlays || OFX::gHostDescription.supportsCustomInteract)
          gInteractSuite  = (OfxInteractSuiteV1 *)    fetchSuite(kOfxInteractSuite, 1);
      }

      // initialise the validation code
      OFX::Validation::initialise();

      // validate the host
      OFX::Validation::validateHostProperties(gHost);

    }

    /** @brief Library side unload action, this fetches all the suite pointers */
    static
    void unloadAction(const char* id)
    {
      gLoadCount--;
      if (gLoadCount<0) {
        OFX::Log::warning(true, "OFX Plugin '%s' is already unloaded.", id);
        return;
      }

      if(gLoadCount==0)
      {
        // force these to null
        gEffectSuite = 0;
        gPropSuite = 0;
        gParamSuite = 0;
        gMemorySuite = 0;
        gThreadSuite = 0;
        gMessageSuite = 0;
        gMessageSuiteV2 = 0;
        gInteractSuite = 0;
        gParametricParameterSuite = 0;
      }

      {
        EffectDescriptorMap::iterator it = gEffectDescriptors.find(id);
        EffectContextMap& toBeDeleted = it->second;
        for(EffectContextMap::iterator it2 = toBeDeleted.begin(); it2 != toBeDeleted.end(); ++it2)
        {
          OFX::ImageEffectDescriptor* desc = it2->second;
          delete desc;
        }
        toBeDeleted.clear();
      }
      { 
        OFX::OfxPlugInfoMap::iterator it = OFX::plugInfoMap.find(id);
        OFX::OfxPluginArray::iterator it2 = std::find(ofxPlugs.begin(), ofxPlugs.end(), it->second._plug.get());
        if (it2 != ofxPlugs.end()) {
          (*it2) = nullptr;
        }
        OFX::plugInfoMap.erase(it);
      }
    }


    /** @brief fetches our pointer out of the props on the handle */
    ImageEffect *retrieveImageEffectPointer(OfxImageEffectHandle handle) 
    {
      ImageEffect *instance;

      // get the prop set on the handle
      OfxPropertySetHandle propHandle;
      OfxStatus stat = OFX::Private::gEffectSuite->getPropertySet(handle, &propHandle);
      throwSuiteStatusException(stat);

      // make our wrapper object
      PropertySet props(propHandle);

      // fetch the instance data out of the properties
      instance = (ImageEffect *) props.propGetPointer(kOfxPropInstanceData);

      OFX::Log::error(instance == 0, "Instance data handle in effect instance properties is NULL!");

      // need to throw something here

      // and dance to the music
      return instance;
    }

    /** @brief Checks the handles passed into the plugin's main entry point */
    static
    void
      checkMainHandles(const std::string &action,  const void *handle, 
      OfxPropertySetHandle inArgsHandle,  OfxPropertySetHandle outArgsHandle,
      bool handleCanBeNull, bool inArgsCanBeNull, bool outArgsCanBeNull)
    {
      if(handleCanBeNull)
        OFX::Log::warning(handle != 0, "Handle passed to '%s' is not null.", action.c_str());
      else
        OFX::Log::error(handle == 0, "'Handle passed to '%s' is null.", action.c_str());

      if(inArgsCanBeNull)
        OFX::Log::warning(inArgsHandle != 0, "'inArgs' Handle passed to '%s' is not null.", action.c_str());
      else
        OFX::Log::error(inArgsHandle == 0, "'inArgs' handle passed to '%s' is null.", action.c_str());

      if(outArgsCanBeNull)
        OFX::Log::warning(outArgsHandle != 0, "'outArgs' Handle passed to '%s' is not null.", action.c_str());
      else
        OFX::Log::error(outArgsHandle == 0, "'outArgs' handle passed to '%s' is null.", action.c_str());

      // validate the property sets on the arguments
      OFX::Validation::validateActionArgumentsProperties(action, inArgsHandle, outArgsHandle);

      // throw exceptions if null when not meant to be null
      if(!handleCanBeNull && !handle)         throwSuiteStatusException(kOfxStatErrBadHandle);
      if(!inArgsCanBeNull && !inArgsHandle)   throwSuiteStatusException(kOfxStatErrBadHandle);
      if(!outArgsCanBeNull && !outArgsHandle) throwSuiteStatusException(kOfxStatErrBadHandle);
    }


    /** @brief Fetches the arguments used in a render action 'inargs' property set into a POD struct */
    static void
      getRenderActionArguments(RenderArguments &args,  OFX::PropertySet inArgs)
    {
      args.time = inArgs.propGetDouble(kOfxPropTime);

      args.renderScale.x = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 0);
      args.renderScale.y = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 1);

      args.renderWindow.x1 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 0);
      args.renderWindow.y1 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 1);
      args.renderWindow.x2 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 2);
      args.renderWindow.y2 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 3);

      args.isEnabledOpenCLRender = inArgs.propGetInt(kOfxImageEffectPropOpenCLEnabled, false) != 0;
      args.isEnabledCudaRender   = inArgs.propGetInt(kOfxImageEffectPropCudaEnabled, false) != 0;
      args.isEnabledMetalRender  = inArgs.propGetInt(kOfxImageEffectPropMetalEnabled, false) != 0;
      args.pOpenCLCmdQ           = inArgs.propGetPointer(kOfxImageEffectPropOpenCLCommandQueue, false);
      args.pCudaStream           = inArgs.propGetPointer(kOfxImageEffectPropCudaStream, false);
      args.pMetalCmdQ            = inArgs.propGetPointer(kOfxImageEffectPropMetalCommandQueue, false);

#ifdef OFX_SUPPORTS_OPENGLRENDER
      // Don't throw an exception if the following inArgs are not present.
      // OpenGL rendering appeared in OFX 1.3
      args.openGLEnabled = inArgs.propGetInt(kOfxImageEffectPropOpenGLEnabled, false) != 0;
#endif

      // Don't throw an exception if the following inArgs are not present:
      // They appeared in OFX 1.2.
      args.sequentialRenderStatus = inArgs.propGetInt(kOfxImageEffectPropSequentialRenderStatus, false) != 0;
      args.interactiveRenderStatus = inArgs.propGetInt(kOfxImageEffectPropInteractiveRenderStatus, false) != 0;

      // kOfxImageEffectPropRenderQualityDraft appeared in OFX 1.4
      args.renderQualityDraft = inArgs.propGetInt(kOfxImageEffectPropRenderQualityDraft, false) != 0;

      args.fieldToRender = eFieldNone;
      std::string str = inArgs.propGetString(kOfxImageEffectPropFieldToRender);
      try {
        args.fieldToRender = mapStrToFieldEnum(str);
      }
      catch (std::invalid_argument&) {
        // dud field?
        OFX::Log::error(true, "Unknown field to render '%s'", str.c_str());

        // HACK need to throw something to cause a failure
      }
    }

    /** @brief Library side render action, fetches relevant properties and calls the client code */
    static
    void
      renderAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);
      RenderArguments args;

      // get the arguments 
      getRenderActionArguments(args, inArgs);

      // and call the plugin client render code
      effectInstance->render(args);
    }

    /** @brief Library side render begin sequence render action, fetches relevant properties and calls the client code */
    static
    void
      beginSequenceRenderAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

      BeginSequenceRenderArguments args;

      args.frameRange.min = inArgs.propGetDouble(kOfxImageEffectPropFrameRange, 0);
      args.frameRange.max = inArgs.propGetDouble(kOfxImageEffectPropFrameRange, 1);

      args.frameStep      = inArgs.propGetDouble(kOfxImageEffectPropFrameStep, 0);

      args.renderScale.x = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 0);
      args.renderScale.y = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 1);

      args.isEnabledOpenCLRender = inArgs.propGetInt(kOfxImageEffectPropOpenCLEnabled, false) != 0;
      args.isEnabledCudaRender   = inArgs.propGetInt(kOfxImageEffectPropCudaEnabled, false) != 0;
      args.isEnabledMetalRender  = inArgs.propGetInt(kOfxImageEffectPropMetalEnabled, false) != 0;
      args.pOpenCLCmdQ           = inArgs.propGetPointer(kOfxImageEffectPropOpenCLCommandQueue, false);
      args.pCudaStream           = inArgs.propGetPointer(kOfxImageEffectPropCudaStream, false);
      args.pMetalCmdQ            = inArgs.propGetPointer(kOfxImageEffectPropMetalCommandQueue, false);

#ifdef OFX_SUPPORTS_OPENGLRENDER
      // Don't throw an exception if the following inArgs are not present.
      // OpenGL rendering appeared in OFX 1.3
      args.openGLEnabled = inArgs.propGetInt(kOfxImageEffectPropOpenGLEnabled, false) != 0;
#endif
      args.isInteractive = inArgs.propGetInt(kOfxPropIsInteractive) != 0;
      // Don't throw an exception if the following inArgs are not present:
      // They appeared in OFX 1.2
      args.sequentialRenderStatus = inArgs.propGetInt(kOfxImageEffectPropSequentialRenderStatus, false) != 0;
      args.interactiveRenderStatus = inArgs.propGetInt(kOfxImageEffectPropInteractiveRenderStatus, false) != 0;
        
      // and call the plugin client render code
      effectInstance->beginSequenceRender(args);
    }

    /** @brief Library side render begin sequence render action, fetches relevant properties and calls the client code */
    static
    void
      endSequenceRenderAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

      EndSequenceRenderArguments args;

      args.renderScale.x = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 0);
      args.renderScale.y = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 1);

      args.isEnabledOpenCLRender = inArgs.propGetInt(kOfxImageEffectPropOpenCLEnabled, false) != 0;
      args.isEnabledCudaRender   = inArgs.propGetInt(kOfxImageEffectPropCudaEnabled, false) != 0;
      args.isEnabledMetalRender  = inArgs.propGetInt(kOfxImageEffectPropMetalEnabled, false) != 0;
      args.pOpenCLCmdQ           = inArgs.propGetPointer(kOfxImageEffectPropOpenCLCommandQueue, false);
      args.pCudaStream           = inArgs.propGetPointer(kOfxImageEffectPropCudaStream, false);
      args.pMetalCmdQ            = inArgs.propGetPointer(kOfxImageEffectPropMetalCommandQueue, false);

#ifdef OFX_SUPPORTS_OPENGLRENDER
      // Don't throw an exception if the following inArgs are not present.
      // OpenGL rendering appeared in OFX 1.3
      args.openGLEnabled = inArgs.propGetInt(kOfxImageEffectPropOpenGLEnabled, false) != 0;
#endif
      args.isInteractive = inArgs.propGetInt(kOfxPropIsInteractive) != 0;
      // Don't throw an exception if the following inArgs are not present:
      // They appeared in OFX 1.2
      args.sequentialRenderStatus = inArgs.propGetInt(kOfxImageEffectPropSequentialRenderStatus, false) != 0;
      args.interactiveRenderStatus = inArgs.propGetInt(kOfxImageEffectPropInteractiveRenderStatus, false) != 0;

      // and call the plugin client render code
      effectInstance->endSequenceRender(args);
    }


    /** @brief Fetches the arguments used in a isIdentity action 'inargs' property set into a POD struct */
    static void
      getIsIdentityActionArguments(IsIdentityArguments &args,  OFX::PropertySet inArgs)
    {
      args.time = inArgs.propGetDouble(kOfxPropTime);

      args.renderScale.x = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 0);
      args.renderScale.y = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 1);

      args.renderWindow.x1 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 0);
      args.renderWindow.y1 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 1);
      args.renderWindow.x2 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 2);
      args.renderWindow.y2 = inArgs.propGetInt(kOfxImageEffectPropRenderWindow, 3);

      std::string str = inArgs.propGetString(kOfxImageEffectPropFieldToRender);
      try {
        args.fieldToRender = mapStrToFieldEnum(str);
      }
      catch (std::invalid_argument&) {
        // dud field?
        OFX::Log::error(true, "Unknown field to render '%s'", str.c_str());

        // HACK need to throw something to cause a failure
      }
    }

    /** @brief Library side render begin sequence render action, fetches relevant properties and calls the client code */
    static
    bool
      isIdentityAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs, OFX::PropertySet &outArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);
      IsIdentityArguments args;

      // get the arguments 
      getIsIdentityActionArguments(args, inArgs);

      // and call the plugin client isIdentity code
      Clip *identityClip = 0;
      double identityTime = args.time;
      bool v = effectInstance->isIdentity(args, identityClip, identityTime);

      if(v && identityClip) {
        outArgs.propSetString(kOfxPropName, identityClip->name());
        outArgs.propSetDouble(kOfxPropTime, identityTime);                
        return true;
      }
      return false;
    }

    /** @brief Library side get region of definition function */
    static
    bool
      regionOfDefinitionAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs, OFX::PropertySet &outArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);
      RegionOfDefinitionArguments args;

      args.renderScale.x = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 0);
      args.renderScale.y = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 1);

      args.time = inArgs.propGetDouble(kOfxPropTime);

      // and call the plugin client code
      OfxRectD rod;
      bool v = effectInstance->getRegionOfDefinition(args, rod);

      if(v) {
        outArgs.propSetDouble(kOfxImageEffectPropRegionOfDefinition, rod.x1, 0);
        outArgs.propSetDouble(kOfxImageEffectPropRegionOfDefinition, rod.y1, 1);
        outArgs.propSetDouble(kOfxImageEffectPropRegionOfDefinition, rod.x2, 2);
        outArgs.propSetDouble(kOfxImageEffectPropRegionOfDefinition, rod.y2, 3);
        return true;
      }
      return false;
    }

    /** @brief Library side get regions of interest function */
    static
    bool
      regionsOfInterestAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs, OFX::PropertySet &outArgs, const char* plugname)
    {
      /** @brief local class to set the roi of a clip */
      class LOCAL ActualROISetter : public OFX::RegionOfInterestSetter {
        OFX::PropertySet &outArgs_;
        bool doneSomething_;
        const std::map<std::string, std::string>& clipROIPropNames_;
      public :
        /** @brief ctor */
        ActualROISetter(OFX::PropertySet &args, const std::map<std::string, std::string>& clipROIPropNames) 
          : outArgs_(args)
          , doneSomething_(false) 
          , clipROIPropNames_(clipROIPropNames)
        { }

        /** @brief did we set something ? */
        bool didSomething(void) const {return doneSomething_;}

        /** @brief set the RoI of the clip */
        virtual void setRegionOfInterest(const Clip &clip, const OfxRectD &roi)
        {
          std::map<std::string, std::string>::const_iterator it = clipROIPropNames_.find(clip.name());
          if(it==clipROIPropNames_.end())
            throw(Exception::PropertyUnknownToHost(clip.name().c_str()));

          // construct the name of the property
          const std::string& propName = it->second;

          // and set it
          outArgs_.propSetDouble(propName.c_str(), roi.x1, 0);
          outArgs_.propSetDouble(propName.c_str(), roi.y1, 1);
          outArgs_.propSetDouble(propName.c_str(), roi.x2, 2);
          outArgs_.propSetDouble(propName.c_str(), roi.y2, 3);      

          // and record the face we have done something
          doneSomething_ = true;
        }
      }; // end of local class

      // fetch our effect pointer 
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);
      RegionsOfInterestArguments args;

      // fetch in arguments from the prop handle
      args.renderScale.x = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 0);
      args.renderScale.y = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 1);

      args.regionOfInterest.x1 = inArgs.propGetDouble(kOfxImageEffectPropRegionOfInterest, 0);
      args.regionOfInterest.y1 = inArgs.propGetDouble(kOfxImageEffectPropRegionOfInterest, 1);
      args.regionOfInterest.x2 = inArgs.propGetDouble(kOfxImageEffectPropRegionOfInterest, 2);
      args.regionOfInterest.y2 = inArgs.propGetDouble(kOfxImageEffectPropRegionOfInterest, 3);

      args.time = inArgs.propGetDouble(kOfxPropTime);
        
      // make a roi setter object
      ActualROISetter setRoIs(outArgs, gEffectDescriptors[plugname][effectInstance->getContext()]->getClipROIPropNames());

      // and call the plugin client code
      effectInstance->getRegionsOfInterest(args, setRoIs);

      // did we do anything ?
      if(setRoIs.didSomething()) 
        return true;
      return false;
    }

    /** @brief Library side frames needed action */
    static
    bool
      framesNeededAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs, OFX::PropertySet &outArgs, const char* plugname)
    {
      /** @brief local class to set the frames needed from a clip */
      class LOCAL ActualSetter : public OFX::FramesNeededSetter {
        OFX::PropertySet &outArgs_;                                  /**< @brief property set to set values in */
        std::map<std::string, std::vector<OfxRangeD> > frameRanges_;  /**< @brief map holding a bunch of frame ranges, one for each clip */
        const std::map<std::string, std::string>& _clipFrameRangePropNames;
      public :
        /** @brief ctor */
        ActualSetter(OFX::PropertySet &args, const std::map<std::string, std::string>& clipFrameRangePropNames) 
          : outArgs_(args), _clipFrameRangePropNames(clipFrameRangePropNames) 
        { }

        /** @brief set the RoI of the clip */
        virtual void setFramesNeeded(const Clip &clip, const OfxRangeD &range) 
        {
          // insert this into the vector which is in the map
          frameRanges_[clip.name()].push_back(range);
        }

        /** @brief write frameRanges_ back to the property set */
        bool setOutProperties(void) 
        {
          bool didSomething = false;

          std::map<std::string, std::vector<OfxRangeD> >::iterator i;

          for(i = frameRanges_.begin(); i != frameRanges_.end(); ++i) {
            if(i->first != kOfxImageEffectOutputClipName) {
              didSomething = true;

              // Make the property name we are setting
              const std::map<std::string, std::string>::const_iterator it = _clipFrameRangePropNames.find(i->first);
              if(it==_clipFrameRangePropNames.end())
                throw(Exception::PropertyUnknownToHost(i->first.c_str()));

              const std::string& propName = it->second;

              // fetch the list of frame ranges
              std::vector<OfxRangeD> &clipRange = i->second;
              std::vector<OfxRangeD>::iterator j;
              int n = 0;

              // and set 'em
              for(j = clipRange.begin(); j < clipRange.end(); ++j) {
                outArgs_.propSetDouble(propName.c_str(), j->min, n++);
                outArgs_.propSetDouble(propName.c_str(), j->max, n++);
              }
            }
          }

          return didSomething;
        }

      }; // end of local class

      // fetch our effect pointer 
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);
      FramesNeededArguments args;

      // fetch in arguments from the prop handle
      args.time = inArgs.propGetDouble(kOfxPropTime);

      // make a roi setter object
      ActualSetter setFrames(outArgs, gEffectDescriptors[plugname][effectInstance->getContext()]->getClipFrameRangePropNames());

      // and call the plugin client code
      effectInstance->getFramesNeeded(args, setFrames);

      // Write it back to the properties and see if we set anything
      if(setFrames.setOutProperties()) 
        return true;
      return false;
    }

    /** @brief Library side get regions of interest function */
    static
    bool
      getTimeDomainAction(OfxImageEffectHandle handle, OFX::PropertySet &outArgs)
    {
      // fetch our effect pointer 
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

      // we can only be a general context effect, so check that this is true
      OFX::Log::error(effectInstance->getContext() != eContextGeneral, "Calling kOfxImageEffectActionGetTimeDomain on an effect that is not a general context effect.");

      OfxRangeD timeDomain;

      // and call the plugin client code
      bool v = effectInstance->getTimeDomain(timeDomain);

      if(v) {
        outArgs.propSetDouble(kOfxImageEffectPropFrameRange, timeDomain.min, 0);
        outArgs.propSetDouble(kOfxImageEffectPropFrameRange, timeDomain.max, 1);
      }

      return v;
    }

    /** @brief Library side get regions of interest function */
    static
    bool
      clipPreferencesAction(OfxImageEffectHandle handle, OFX::PropertySet &outArgs, const char* plugname)
    {
      // fetch our effect pointer 
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

      // set up our clip preferences setter
      ImageEffectDescriptor* desc = gEffectDescriptors[plugname][effectInstance->getContext()];
      ClipPreferencesSetter prefs(outArgs, desc->getClipDepthPropNames(), desc->getClipComponentPropNames(), desc->getClipPARPropNames());

      // and call the plug-in client code
      effectInstance->getClipPreferences(prefs);

      // did we do anything ?
      if(prefs.didSomething()) 
        return true;
      return false;
    }

    /** @brief Library side begin instance changed action */
    static
    void
      beginInstanceChangedAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

      std::string reasonStr = inArgs.propGetString(kOfxPropChangeReason);
      InstanceChangeReason reason = mapToInstanceChangedReason(reasonStr);

      // and call the plugin client code
      effectInstance->beginChanged(reason);
    }

    /** @brief Library side instance changed action */
    static
    void
      instanceChangedAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

      InstanceChangedArgs args;

      // why did it change
      std::string reasonStr = inArgs.propGetString(kOfxPropChangeReason);
      args.reason = mapToInstanceChangedReason(reasonStr);
      args.time = inArgs.propGetDouble(kOfxPropTime);
      args.renderScale.x = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 0);
      args.renderScale.y = inArgs.propGetDouble(kOfxImageEffectPropRenderScale, 1);

      // what changed
      std::string changedType = inArgs.propGetString(kOfxPropType);
      std::string changedName = inArgs.propGetString(kOfxPropName);

      if(changedType == kOfxTypeParameter) {
        // and call the plugin client code
        effectInstance->changedParam(args, changedName);
      }
      else if(changedType == kOfxTypeClip) {
        // and call the plugin client code
        effectInstance->changedClip(args, changedName);
      }
      else {
        OFX::Log::error(true, "Instance Changed called with unknown type '%s' of object '%s'", changedType.c_str(), changedName.c_str());
      }
    }

    /** @brief Library side end instance changed action */
    static
    void
      endInstanceChangedAction(OfxImageEffectHandle handle, OFX::PropertySet inArgs)
    {
      ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

      std::string reasonStr = inArgs.propGetString(kOfxPropChangeReason);
      InstanceChangeReason reason = mapToInstanceChangedReason(reasonStr);

      // and call the plugin client code
      effectInstance->endChanged(reason);
    }


    /** @brief The main entry point for the plugin
    */
    OfxStatus mainEntryStr(const char    *actionRaw,
      const void    *handleRaw,
      OfxPropertySetHandle   inArgsRaw,
      OfxPropertySetHandle   outArgsRaw,
      const char* plugname)
    {
      OFX::Log::print("********************************************************************************");
      OFX::Log::print("START mainEntry (%s for %s)", actionRaw, plugname);
      OFX::Log::indent();
      OfxStatus stat = kOfxStatReplyDefault;
      try {

        OfxPlugInfoMap::iterator it = plugInfoMap.find(plugname);
        if(it==plugInfoMap.end())
          throw;

        OFX::PluginFactory* factory = it->second._factory;

        // Cast the raw handle to be an image effect handle, because that is what it is
        OfxImageEffectHandle handle = (OfxImageEffectHandle) handleRaw;

        // Turn the arguments into wrapper objects to make our lives easier
        OFX::PropertySet inArgs(inArgsRaw);
        OFX::PropertySet outArgs(outArgsRaw);

        // turn the action into a std::string
        std::string action(actionRaw);

        // figure the actions
        if (action == kOfxActionLoad) {
          // call the support load function, param-less
          OFX::Private::loadAction(); 

          // call the plugin side load action, param-less
          factory->load();

          // got here, must be good
          stat = kOfxStatOK;
        }

        // figure the actions
        else if (action == kOfxActionUnload) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, true, true, true);

          // call the plugin side unload action, param-less, should be called, eve if the stat above failed!
          factory->unload();

          // call the support unload function, param-less
          OFX::Private::unloadAction(plugname); 

          // got here, must be good
          stat = kOfxStatOK;
        }

        else if(action == kOfxActionDescribe) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // make the plugin descriptor
          ImageEffectDescriptor *desc = new ImageEffectDescriptor(handle);

          // validate the host
          OFX::Validation::validatePluginDescriptorProperties(fetchEffectProps(handle));

          //  and pass it to the plugin to do something with it

          factory->describe(*desc);

          // add it to our map
          gEffectDescriptors[plugname][eContextNone] = desc;

          // got here, must be good
          stat = kOfxStatOK;
        }
        else if(action == kOfxImageEffectActionDescribeInContext) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, true);

          // make the plugin descriptor and pass it to the plugin to do something with it
          ImageEffectDescriptor *desc = new ImageEffectDescriptor(handle);

          // figure the context and map it to an enum
          std::string contextStr = inArgs.propGetString(kOfxImageEffectPropContext);
          ContextEnum context = mapToContextEnum(contextStr);

          // validate the host
          OFX::Validation::validatePluginDescriptorProperties(fetchEffectProps(handle));

          // call plugin describe in context
          factory->describeInContext(*desc, context);

          // add it to our map
          gEffectDescriptors[plugname][context] = desc;

          // got here, must be good
          stat = kOfxStatOK;
        }
        else if(action == kOfxActionCreateInstance) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch the effect props to figure the context
          PropertySet effectProps = fetchEffectProps(handle);

          // get the context and turn it into an enum
          std::string str = effectProps.propGetString(kOfxImageEffectPropContext);
          ContextEnum context = mapToContextEnum(str);

          // make the image effect instance for this context
          ImageEffect *instance = factory->createInstance(handle, context);
          (void)instance;

          // validate the plugin handle's properties
          OFX::Validation::validatePluginInstanceProperties(fetchEffectProps(handle));

          // got here, must be good
          stat = kOfxStatOK;
        }
        else if(action == kOfxActionDestroyInstance) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch our pointer out of the props on the handle
          ImageEffect *instance = retrieveImageEffectPointer(handle);

          // kill it
          delete instance;

          // got here, must be good
          stat = kOfxStatOK;
        }
        else if(action == kOfxImageEffectActionRender) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, true);

          // call the render action skin
          renderAction(handle, inArgs);

          // got here, must be good
          stat = kOfxStatOK;
        }
        else if(action == kOfxImageEffectActionBeginSequenceRender) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, true);

          // call the begin render action skin
          beginSequenceRenderAction(handle, inArgs);
        }
        else if(action == kOfxImageEffectActionEndSequenceRender) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, true);

          // call the begin render action skin
          endSequenceRenderAction(handle, inArgs);
        }
        else if(action == kOfxImageEffectActionIsIdentity) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, false);

          // call the identity action, if it is, return OK
          if(isIdentityAction(handle, inArgs, outArgs))
            stat = kOfxStatOK;
        }
        else if(action == kOfxImageEffectActionGetRegionOfDefinition) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, false);

          // call the rod action, return OK if it does something
          if(regionOfDefinitionAction(handle, inArgs, outArgs))
            stat = kOfxStatOK;
        }
        else if(action == kOfxImageEffectActionGetRegionsOfInterest) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, false);

          // call the RoI action, return OK if it does something
          if(regionsOfInterestAction(handle, inArgs, outArgs, plugname))
            stat = kOfxStatOK;
        }
        else if(action == kOfxImageEffectActionGetFramesNeeded) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, false);

          // call the frames needed action, return OK if it does something
          if(framesNeededAction(handle, inArgs, outArgs, plugname))
            stat = kOfxStatOK;
        }
        else if(action == kOfxImageEffectActionGetClipPreferences) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, false);

          // call the frames needed action, return OK if it does something
          if(clipPreferencesAction(handle, outArgs, plugname))
            stat = kOfxStatOK;
        }
        else if(action == kOfxActionPurgeCaches) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch our pointer out of the props on the handle
          ImageEffect *instance = retrieveImageEffectPointer(handle);

          // purge 'em
          instance->purgeCaches();
        }
        else if(action == kOfxActionSyncPrivateData) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch our pointer out of the props on the handle
          ImageEffect *instance = retrieveImageEffectPointer(handle);

          // and sync it
          instance->syncPrivateData();
        }
        else if(action == kOfxImageEffectActionGetTimeDomain) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, false);

          // call the instance changed action
          if(getTimeDomainAction(handle, outArgs))
            stat = kOfxStatOK;
        }
        else if(action == kOfxActionBeginInstanceChanged) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, true);

          // call the instance changed action
          beginInstanceChangedAction(handle, inArgs);
        }
        else if(action == kOfxActionInstanceChanged) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, true);

          // call the instance changed action
          instanceChangedAction(handle, inArgs);
        }
        else if(action == kOfxActionEndInstanceChanged) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, false, true);

          // call the instance changed action
          endInstanceChangedAction(handle, inArgs);
        }
        else if(action == kOfxActionBeginInstanceEdit) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch our pointer out of the props on the handle
          ImageEffect *instance = retrieveImageEffectPointer(handle);

          // call the begin edit function
          instance->beginEdit();
        }
        else if(action == kOfxActionEndInstanceEdit) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch our pointer out of the props on the handle
          ImageEffect *instance = retrieveImageEffectPointer(handle);

          // call the end edit function
          instance->endEdit();
        }
#ifdef OFX_SUPPORTS_OPENGLRENDER
        else if(action == kOfxActionOpenGLContextAttached) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch our pointer out of the props on the handle
          ImageEffect *instance = retrieveImageEffectPointer(handle);

          // call the context attached function
          instance->contextAttached();
        }
        else if(action == kOfxActionOpenGLContextDetached) {
          checkMainHandles(actionRaw, handleRaw, inArgsRaw, outArgsRaw, false, true, true);

          // fetch our pointer out of the props on the handle
          ImageEffect *instance = retrieveImageEffectPointer(handle);

          // call the context detached function
          instance->contextDetached();
        }
#endif
        else if(actionRaw) {
          OFX::Log::error(true, "Unknown action '%s'.", actionRaw);
        }
        else {
          OFX::Log::error(true, "Requested action was a null pointer.");
        }
      }

      // catch suite exceptions
      catch (const OFX::Exception::Suite &ex)
      {
        OFX::Log::error(true, "Caught OFX::Exception::Suite: %s", ex.what());
#      ifdef DEBUG
        std::cout << "Caught OFX::Exception::Suite: " << ex.what() << std::endl;
#      endif
        stat = ex.status();
      }

      // catch host inadequate exceptions
      catch (const OFX::Exception::HostInadequate &e)
      {
        OFX::Log::error(true, "Caught OFX::Exception::HostInadequate: %s", e.what());
#      ifdef DEBUG
        std::cout << "Caught OFX::Exception::HostInadequate: " << e.what() << std::endl;
#      endif
        stat = kOfxStatErrMissingHostFeature;
      }

      // catch exception due to a property being unknown to the host, implies something wrong with host if not caught further down
      catch (const OFX::Exception::PropertyUnknownToHost &e)
      {
        OFX::Log::error(true, "Caught OFX::Exception::PropertyUnknownToHost: %s", e.what());
#      ifdef DEBUG
        std::cout << "Caught OFX::Exception::PropertyUnknownToHost: " << e.what() << std::endl;
#      endif
        stat = kOfxStatErrMissingHostFeature;
      }

      // catch memory
      catch (std::bad_alloc&)
      {
        stat = kOfxStatErrMemory;
      }

      // catch a custom client exception, if defined
#ifdef OFX_CLIENT_EXCEPTION_TYPE
      catch (OFX_CLIENT_EXCEPTION_TYPE &ex)
      {
        stat = OFX_CLIENT_EXCEPTION_HANDLER(ex, plugname);
      }
#endif
      // Catch anything else, unknown
      catch (const std::exception &e)
      {
        OFX::Log::error(true, "Caught std::exception: %s", e.what());
#      ifdef DEBUG
        std::cout << "Caught std::exception: " << e.what() << std::endl;
#      endif
        stat = kOfxStatFailed;
      }
      catch (...)
      {
        OFX::Log::error(true, "Caught unknown exception");
#      ifdef DEBUG
        std::cout << "Caught Unknown exception" << std::endl;
#      endif
        stat = kOfxStatFailed;
      }

      OFX::Log::outdent();
      OFX::Log::print("STOP mainEntry (%s for %s, returning %d=%s)\n", actionRaw, plugname,
                      stat, mapStatusToString(stat));
      return stat;
    }      


    OfxStatus customParamInterpolationV1Entry(
      const void*            handleRaw,
      OfxPropertySetHandle   inArgsRaw,
      OfxPropertySetHandle   outArgsRaw)
    {
      OFX::Log::print("********************************************************************************");
      OFX::Log::print("START customParamInterpolationV1Entry");
      OFX::Log::indent();
      OfxStatus stat = kOfxStatReplyDefault;
      try {
        // Cast the raw handle to be an image effect handle, because that is what it is
        OfxImageEffectHandle handle = (OfxImageEffectHandle) handleRaw;

        // Turn the arguments into wrapper objects to make our lives easier
        OFX::PropertySet inArgs(inArgsRaw);
        OFX::PropertySet outArgs(outArgsRaw);

        ImageEffect *effectInstance = retrieveImageEffectPointer(handle);

        InterpolateCustomArgs interpArgs;

        interpArgs.time     = inArgs.propGetDouble(kOfxPropTime);
        interpArgs.value1   = inArgs.propGetString(kOfxParamPropCustomValue, 0);
        interpArgs.value2   = inArgs.propGetString(kOfxParamPropCustomValue, 1);
        interpArgs.keytime1 = inArgs.propGetDouble(kOfxParamPropInterpolationTime, 0);
        interpArgs.keytime2 = inArgs.propGetDouble(kOfxParamPropInterpolationTime, 1);
        interpArgs.amount   = inArgs.propGetDouble(kOfxParamPropInterpolationAmount);

        std::string paramName = inArgs.propGetString(kOfxPropName);

        // and call the plugin client code
        std::string output = effectInstance->interpolateCustomParam(interpArgs, paramName);

        outArgs.propSetString(kOfxParamPropCustomValue, output);
      }

      // catch suite exceptions
      catch (OFX::Exception::Suite &ex)
      {
#      ifdef DEBUG
        std::cout << "Caught OFX::Exception::Suite" << std::endl;
#      endif
        stat = ex.status();
      }

      // catch host inadequate exceptions
      catch (OFX::Exception::HostInadequate&)
      {
#      ifdef DEBUG
        std::cout << "Caught OFX::Exception::HostInadequate" << std::endl;
#      endif
        stat = kOfxStatErrMissingHostFeature;
      }

      // catch exception due to a property being unknown to the host, implies something wrong with host if not caught further down
      catch (OFX::Exception::PropertyUnknownToHost&)
      {
#      ifdef DEBUG
        std::cout << "Caught OFX::Exception::PropertyUnknownToHost" << std::endl;
#      endif
        stat = kOfxStatErrMissingHostFeature;
      }

      // catch memory
      catch (std::bad_alloc&)
      {
        stat = kOfxStatErrMemory;
      }

      // catch a custom client exception, if defined
#ifdef OFX_CLIENT_EXCEPTION_TYPE
      catch (OFX_CLIENT_EXCEPTION_TYPE &ex)
      {
        stat = OFX_CLIENT_EXCEPTION_HANDLER(ex, plugname);
      }
#endif
      // Catch anything else, unknown
      catch (...)
      {
#      ifdef DEBUG
        std::cout << "Caught Unknown exception" << std::endl;
#      endif
        stat = kOfxStatFailed;
      }

      OFX::Log::outdent();
      OFX::Log::print("STOP customParamInterpolationV1Entry\n");
      return stat;
    }

    /** @brief The plugin function that gets passed the host structure. */
    void setHost(OfxHost *host)
    {
      gHost = host;
    }

  }; // namespace Private

  /** @brief Fetch's a suite from the host and logs errors */
  const void * fetchSuite(const char *suiteName, int suiteVersion, bool optional)
  {
    const void *suite = Private::gHost->fetchSuite(Private::gHost->host, suiteName, suiteVersion);
    if(suite==0)
    {
      if(optional)
        OFX::Log::warning(suite == 0, "Could not fetch the optional suite '%s' version %d.", suiteName, suiteVersion);
      else
        OFX::Log::error(suite == 0, "Could not fetch the mandatory suite '%s' version %d.", suiteName, suiteVersion);
    }
    if(!optional && suite == 0) throw OFX::Exception::HostInadequate(suiteName);
    return suite;
  }

}; // namespace OFX

static
OFX::OfxPlugInfo generatePlugInfo(OFX::PluginFactory* factory, std::string& newID)
{
  newID = factory->getUID();
  std::unique_ptr<OfxPlugin> ofxPlugin(new OfxPlugin());
  ofxPlugin->pluginApi  = kOfxImageEffectPluginApi;
  ofxPlugin->apiVersion = 1;
  ofxPlugin->pluginIdentifier   = factory->getID().c_str();
  ofxPlugin->pluginVersionMajor = factory->getMajorVersion();
  ofxPlugin->pluginVersionMinor = factory->getMinorVersion();
  ofxPlugin->setHost    = OFX::Private::setHost;
  ofxPlugin->mainEntry  = factory->getMainEntry();
  return OFX::OfxPlugInfo(factory, std::move(ofxPlugin));
}

bool gHasInit = false;

static
void init()
{
  if(gHasInit)
    return;

  OFX::Plugin::getPluginIDs(OFX::plugIDs);
  if(OFX::ofxPlugs.empty())
    OFX::ofxPlugs.resize(OFX::plugIDs.size());

  int counter = 0;
  for (OFX::PluginFactoryArray::const_iterator it = OFX::plugIDs.begin(); it != OFX::plugIDs.end(); ++it, ++counter)
  {
    std::string newID;
    OFX::OfxPlugInfo info = generatePlugInfo(*it, newID);
    OFX::ofxPlugs[counter] = info._plug.get();
    OFX::plugInfoMap[newID] = std::move(info);
  }
  gHasInit = true;
}

/** @brief, mandated function returning the number of plugins, which is always 1 */
EXPORT int OfxGetNumberOfPlugins(void)
{
  init();
  return (int)OFX::plugIDs.size();
}

/** @brief, mandated function returning the nth plugin 

We call the plugin side defined OFX::Plugin::getPluginIDs function to find out what to set.
*/

EXPORT OfxPlugin* OfxGetPlugin(int nth)
{
  init();
  int numPlugs = (int)OFX::plugInfoMap.size();
  OFX::Log::error(nth >= numPlugs, "Host attempted to get plugin %d, when there is only %d plugin(s), so it should have asked for 0.", nth, numPlugs);
  if(OFX::ofxPlugs[nth] == nullptr)
  {
    std::string newID;
    OFX::OfxPlugInfo info = generatePlugInfo(OFX::plugIDs[nth], newID);
    OFX::ofxPlugs[nth] = info._plug.get();
    OFX::plugInfoMap[newID] = std::move(info);
  }
  return OFX::ofxPlugs[nth];
}
