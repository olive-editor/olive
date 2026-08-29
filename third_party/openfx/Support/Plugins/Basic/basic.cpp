

#ifdef _WINDOWS
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <stdio.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

#include "../include/ofxsProcessing.H"

////////////////////////////////////////////////////////////////////////////////
// a dumb interact that just draw's a square you can drag
static const OfxPointD kBoxSize = {20, 20};

class BasicInteract : public OFX::OverlayInteract {
protected :
  enum StateEnum {
    eInActive,
    ePoised,
    ePicked
  };

  OfxPointD _position;
  StateEnum _state;

public :
  BasicInteract(OfxInteractHandle handle, OFX::ImageEffect* /*effect*/)
    : OFX::OverlayInteract(handle)
    , _state(eInActive)
  {
    _position.x = 0;
    _position.y = 0;
  }

  // overridden functions from OFX::Interact to do things
  virtual bool draw(const OFX::DrawArgs &args);
  virtual bool penMotion(const OFX::PenArgs &args);
  virtual bool penDown(const OFX::PenArgs &args);
  virtual bool penUp(const OFX::PenArgs &args);
};

////////////////////////////////////////////////////////////////////////////////
// rendering routines
template <class T> inline T
Minimum(T a, T b) {    return (a < b) ? a : b;}

template <class T> inline T
Absolute(T a) { return (a < 0) ? -a : a;}

template <class T> inline T 
Clamp(T v, int min, int max)
{
  if(v < T(min)) return T(min);
  if(v > T(max)) return T(max);
  return v;
}

// Base class for the RGBA and the Alpha processor
class ImageScalerBase : public OFX::ImageProcessor {
protected :
  OFX::Image *_srcImg;
  OFX::Image *_maskImg;
  double _rScale, _gScale, _bScale, _aScale;
  bool   _doMasking;

public :
  /** @brief no arg ctor */
  ImageScalerBase(OFX::ImageEffect &instance)
    : OFX::ImageProcessor(instance)
    , _srcImg(0)
    , _maskImg(0)
    , _rScale(1)
    , _gScale(1)
    , _bScale(1)
    , _aScale(1)
    , _doMasking(false)
  {        
  }

  /** @brief set the src image */
  void setSrcImg(OFX::Image *v) {_srcImg = v;}

  /** @brief set the optional mask image */
  void setMaskImg(OFX::Image *v) {_maskImg = v;}

  // Are we masking. We can't derive this from the mask image being set as NULL is a valid value for an input image
  void doMasking(bool v) {_doMasking = v;}


  /** @brief set the scale */
  void setScales(float r, float g, float b, float a)
  {
    _rScale = r;
    _gScale = g;
    _bScale = b;
    _aScale = a;
  }

};

// template to do the RGBA processing
template <class PIX, int nComponents, int max>
class ImageScaler : public ImageScalerBase {
public :
  // ctor
  ImageScaler(OFX::ImageEffect &instance) 
    : ImageScalerBase(instance)
  {}

  // and do some processing
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    float scales[4];
    scales[0] = nComponents == 1 ? (float)_aScale : (float)_rScale;
    scales[1] = (float)_gScale;
    scales[2] = (float)_bScale;
    scales[3] = (float)_aScale;

    float maskScale = 1.0f;

    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(_effect.abort()) break;

      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {

        PIX *srcPix = (PIX *)  (_srcImg ? _srcImg->getPixelAddress(x, y) : 0);

        // are we doing masking
        if(_doMasking) {
          // we do, get the pixel from the mask
          if(!_maskImg)
            maskScale = 1.0f;
          else
          {
            PIX *maskPix = (PIX *)  (_maskImg ? _maskImg->getPixelAddress(x, y) : 0);
            // figure the scale factor from that pixel
            maskScale = maskPix != 0 ? float(*maskPix)/float(max) : 0.0f;
          }
        }

        // do we have a source image to scale up
        if(srcPix) {
          for(int c = 0; c < nComponents; c++) {
            float v;

            // scale the component up by the scale factor, modulated by the maskScale
            if(maskScale != 1.0f) 
              v = srcPix[c] * (1.0f + (scales[c] - 1.0f) * maskScale);
            else
              v = srcPix[c] * scales[c];

            if(max == 1)  // implies floating point and so no clamping
              dstPix[c] = PIX(v);
            else  // integer based and we need to clamp
              dstPix[c] = PIX(Clamp(v, 0, max));
          }
        }
        else {
          // no src pixel here, be black and transparent
          for(int c = 0; c < nComponents; c++) {
            dstPix[c] = 0;
          }
        }
        // increment the dst pixel
        dstPix += nComponents;
      }
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class BasicPlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;
  OFX::Clip *srcClip_;
  OFX::Clip *maskClip_;

  OFX::DoubleParam  *scale_;
  OFX::DoubleParam  *rScale_;
  OFX::DoubleParam  *gScale_;
  OFX::DoubleParam  *bScale_;
  OFX::DoubleParam  *aScale_;
  OFX::BooleanParam *componentScalesEnabled_;

public :
  /** @brief ctor */
  BasicPlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , scale_(0)
    , rScale_(0)
    , gScale_(0)
    , bScale_(0)
    , aScale_(0)
    , componentScalesEnabled_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    // name of mask clip depends on the context
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    scale_   = fetchDoubleParam("scale");
    rScale_  = fetchDoubleParam("scaleR");
    gScale_  = fetchDoubleParam("scaleG");
    bScale_  = fetchDoubleParam("scaleB");
    aScale_  = fetchDoubleParam("scaleA");
    componentScalesEnabled_ = fetchBooleanParam("scaleComponents");

    // set the enabledness of our RGBA sliders
    setEnabledness();
  }

  /* sets the enabledness of the component scale params depending on the type of input image and the state of the scaleComponents param */
  void setEnabledness(void);

  /* Override the render */
  virtual void render(const OFX::RenderArguments &args);

  /* override is identity */
  virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime);

  /* override changedParam */
  virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName);

  /* override changed clip */
  virtual void changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName);

  // override the rod call
  virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);

  // override the roi call
  virtual void getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois);

  /* set up and run a processor */
  void
    setupAndProcess(ImageScalerBase &, const OFX::RenderArguments &args);
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from


/* set up and run a processor */
void
BasicPlugin::setupAndProcess(ImageScalerBase &processor, const OFX::RenderArguments &args)
{
  // get a dst image
  std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
  OFX::BitDepthEnum dstBitDepth       = dst->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dst->getPixelComponents();

  // fetch main input image
  std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

  // make sure bit depths are sane
  if(src.get()) {
    OFX::BitDepthEnum    srcBitDepth      = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    // see if they have the same depths and bytes and all
    if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
      throw int(1); // HACK!! need to throw an sensible exception here!
  }

  std::unique_ptr<OFX::Image> mask;

  // do we do masking
  if(getContext() != OFX::eContextFilter) {
    mask.reset(maskClip_->fetchImage(args.time));
    // say we are masking
    processor.doMasking(true);

    // Set it in the processor 
    processor.setMaskImg(mask.get());
  }

  // get the scale parameter values...
  double r, g, b, a = aScale_->getValueAtTime(args.time);
  r = g = b = scale_->getValueAtTime(args.time);

  // see if the individual component scales are enabled
  if(componentScalesEnabled_->getValueAtTime(args.time)) {
    r *= rScale_->getValueAtTime(args.time);
    g *= gScale_->getValueAtTime(args.time);
    b *= bScale_->getValueAtTime(args.time);
  }

  // set the images
  processor.setDstImg(dst.get());
  processor.setSrcImg(src.get());


  // set the render window
  processor.setRenderWindow(args.renderWindow);

  // set the scales
  processor.setScales((float)r, (float)g, (float)b, (float)a);

  // Call the base class process member, this will call the derived templated process code
  processor.process();
}

// override the rod call
bool
BasicPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
  // our RoD is the same as the 'Source' clip's, we are not interested in the mask
  rod = srcClip_->getRegionOfDefinition(args.time);

  // say we set it
  return true;
}

// override the roi call
void 
BasicPlugin::getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois)
{
  // we don't actually need to do this as this is the default, but do it for examples sake
  rois.setRegionOfInterest(*srcClip_, args.regionOfInterest);

  // set it on the mask only if we are in an interesting context
  if(getContext() != OFX::eContextFilter)
    rois.setRegionOfInterest(*maskClip_, args.regionOfInterest);
}

// the overridden render function
void
BasicPlugin::render(const OFX::RenderArguments &args)
{
  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) {
    switch(dstBitDepth) {
case OFX::eBitDepthUByte : {      
  ImageScaler<unsigned char, 4, 255> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  ImageScaler<unsigned short, 4, 65535> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  ImageScaler<float, 4, 1> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;
default :
  OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
  else {
    switch(dstBitDepth) {
case OFX::eBitDepthUByte : {
  ImageScaler<unsigned char, 1, 255> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  ImageScaler<unsigned short, 1, 65535> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  ImageScaler<float, 1, 1> fred(*this);
  setupAndProcess(fred, args);
                           }                          
                           break;
default :
  OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } 
}

// overridden is identity
bool
BasicPlugin:: isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  // get the scale parameters
  double scale = scale_->getValueAtTime(args.time);
  double rScale = 1, gScale = 1, bScale = 1, aScale = 1;
  if(componentScalesEnabled_->getValueAtTime(args.time)) {
    rScale = rScale_->getValueAtTime(args.time);
    gScale = gScale_->getValueAtTime(args.time);
    bScale = bScale_->getValueAtTime(args.time);
    aScale = aScale_->getValueAtTime(args.time);
  }
  rScale *= scale; gScale *= scale; bScale *= scale;

  // do we do any scaling ?
  if(rScale == 1 && gScale == 1 && bScale == 1 && aScale == 1) {
    identityClip = srcClip_;
    identityTime = args.time;
    return true;
  }

  // nope, idenity we is
  return false;
}

// set the enabledness of the individual component scales
void
BasicPlugin::setEnabledness(void)
{
  // the componet enabledness depends on the clip being RGBA and the param being true
  bool v = componentScalesEnabled_->getValue() && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA;

  // enable them
  rScale_->setEnabled(v);
  gScale_->setEnabled(v);
  bScale_->setEnabled(v);
  aScale_->setEnabled(v);
}

// we have changed a param
void
BasicPlugin::changedParam(const OFX::InstanceChangedArgs &/*args*/, const std::string &paramName)
{
  if(paramName == "scaleComponents")  setEnabledness(); 
}

// we have changed a param
void
BasicPlugin::changedClip(const OFX::InstanceChangedArgs &/*args*/, const std::string &clipName)
{
  if(clipName == kOfxImageEffectSimpleSourceClipName)  setEnabledness();
}

////////////////////////////////////////////////////////////////////////////////
// stuff for the interact 

// draw the interact
bool BasicInteract::draw(const OFX::DrawArgs &args)
{
  OfxRGBColourF col;
  switch(_state) 
  {
  case eInActive : col.r = col.g = col.b = 0.0f; break;
  case ePoised   : col.r = col.g = col.b = 0.5f; break;
  case ePicked   : col.r = col.g = col.b = 1.0f; break;
  }

  // make the box a constant size on screen by scaling by the pixel scale
  float dx = (float)(kBoxSize.x * args.pixelScale.x);
  float dy = (float)(kBoxSize.y * args.pixelScale.y);

  // Draw a cross hair, the current coordinate system aligns with the image plane.
  glPushMatrix();

  // draw the bo
  glColor3f(col.r, col.g, col.b);
  glTranslated(_position.x, _position.y, 0);
  glBegin(GL_POLYGON);
  glVertex2f(-dx, -dy);
  glVertex2f(-dx,  dy);
  glVertex2f( dx,  dy);
  glVertex2f( dx, -dy);
  glEnd();
  glPopMatrix();

  glPushMatrix();
  // draw a complementary outline
  glColor3f(1.0f - col.r, 1.0f - col.g, 1.0f - col.b);
  glTranslated(_position.x, _position.y, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(-dx, -dy);
  glVertex2f(-dx,  dy);
  glVertex2f( dx,  dy);
  glVertex2f( dx, -dy);
  glEnd();
  glPopMatrix();

  return true;
}

// overridden functions from OFX::Interact to do things
bool 
BasicInteract::penMotion(const OFX::PenArgs &args)
{
  // figure the size of the box in cannonical coords
  float dx = (float)(kBoxSize.x * args.pixelScale.x);
  float dy = (float)(kBoxSize.y * args.pixelScale.y);

  // pen position is in cannonical coords
  OfxPointD penPos = args.penPosition;

  switch(_state) {
case eInActive : 
case ePoised   : 
  {
    // are we in the box, become 'poised'
    StateEnum newState;
    penPos.x -= _position.x;
    penPos.y -= _position.y;
    if(Absolute(penPos.x) < dx &&
      Absolute(penPos.y) < dy) {
        newState = ePoised;
    }
    else {
      newState = eInActive;
    }

    if(_state != newState) {
      // we have a new state
      _state = newState;

      // and force an overlay redraw
      _effect->redrawOverlays();
    }
  }
  break;

case ePicked   : 
  {
    // move our position
    _position = penPos;

    // and force an overlay redraw
    _effect->redrawOverlays();
  }
  break;
  }

  // we have trapped it only if the mouse ain't over it or we are actively dragging
  return _state != eInActive;
}

bool 
BasicInteract::penDown(const OFX::PenArgs &args)
{
  // this will refigure the state
  penMotion(args);

  // if poised means we were over it when the pen went down, so pick it
  if(_state == ePoised) {
    // we are now picked
    _state = ePicked;

    // move our position
    _position = args.penPosition;

    // and request a redraw just incase
    _effect->redrawOverlays();
  }

  return _state == ePicked;
}

bool 
BasicInteract::penUp(const OFX::PenArgs &args)
{
  if(_state == ePicked) {
    // reset to poised for a moment
    _state = ePoised;

    // this will refigure the state
    penMotion(args);

    // and redraw for good measure
    _effect->redrawOverlays();

    // we did trap it
    return true;
  }

  // we didn't trap it
  return false;
}


using namespace OFX;

mDeclarePluginFactory(BasicExamplePluginFactory, {}, {});

class BasicExampleOverlayDescriptor : public DefaultEffectOverlayDescriptor<BasicExampleOverlayDescriptor, BasicInteract> {};

void BasicExamplePluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
  // basic labels
  desc.setLabels("Gain", "Gain", "Gain");
  desc.setPluginGrouping("OFX Example (Support)");

  // add the supported contexts, only filter at the moment
  desc.addSupportedContext(eContextFilter);
  desc.addSupportedContext(eContextGeneral);
  desc.addSupportedContext(eContextPaint);

  // add supported pixel depths
  desc.addSupportedBitDepth(eBitDepthUByte);
  desc.addSupportedBitDepth(eBitDepthUShort);
  desc.addSupportedBitDepth(eBitDepthFloat);

  // set a few flags
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(true);
  desc.setTemporalClipAccess(false);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);

  desc.setOverlayInteractDescriptor( new BasicExampleOverlayDescriptor);
}

// make a double scale param
static
DoubleParamDescriptor *defineScaleParam(OFX::ImageEffectDescriptor &desc,
                                        const std::string &name, const std::string &label, const std::string &hint,
                                        GroupParamDescriptor *parent)
{
  DoubleParamDescriptor *param = desc.defineDoubleParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  param->setHint(hint);
  param->setDefault(1);
  param->setRange(0, 10);
  param->setIncrement(0.1);
  param->setDisplayRange(0, 10);
  param->setDoubleType(eDoubleTypeScale);
  if(parent) param->setParent(*parent);
  return param;
}


void BasicExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  // Source clip only in the filter context
  // create the mandated source clip
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);

  // if general or paint context, define the mask clip
  if(context == eContextGeneral || context == eContextPaint) {                
    // if paint context, it is a mandated input called 'brush'
    ClipDescriptor *maskClip = context == eContextGeneral ? desc.defineClip("Mask") : desc.defineClip("Brush");
    maskClip->addSupportedComponent(ePixelComponentAlpha);
    maskClip->setTemporalClipAccess(false);
    if(context == eContextGeneral)
      maskClip->setOptional(true);
    maskClip->setSupportsTiles(true); 
    maskClip->setIsMask(true); // we are a mask input
  }

  // create the mandated output clip
  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

  // make some pages and to things in 
  PageParamDescriptor *page = desc.definePageParam("Controls");

  // group param to group the scales
  GroupParamDescriptor *componentScalesGroup = desc.defineGroupParam("componentScales");
  componentScalesGroup->setHint("Scales on the individual component");
  componentScalesGroup->setLabels("Components", "Components", "Components");            

  // make overall scale params 
  DoubleParamDescriptor *param = defineScaleParam(desc, "scale", "scale", "Scales all component in the image", 0);
  page->addChild(*param);

  // add a boolean to enable the component scale
  BooleanParamDescriptor *boolP = desc.defineBooleanParam("scaleComponents");
  boolP->setDefault(true);
  boolP->setHint("Enables scales on individual components");
  boolP->setLabels("Scale Components", "Scale Components", "Scale Components");
  boolP->setParent(*componentScalesGroup);
  page->addChild(*boolP);

  // make the four component scale params 
  param = defineScaleParam(desc, "scaleR", "red", "Scales the red component of the image", componentScalesGroup);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleG", "green", "Scales the green component of the image", componentScalesGroup);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleB", "blue", "Scales the blue component of the image", componentScalesGroup);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleA", "alpha", "Scales the alpha component of the image", componentScalesGroup);
  page->addChild(*param);
}

ImageEffect *BasicExamplePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
  return new BasicPlugin(handle);
}

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static BasicExamplePluginFactory p("net.sf.openfx.basicPlugin", 1, 0);
      ids.push_back(&p);
    }  
  }
}
