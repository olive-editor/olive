

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

#include "../include/ofxsProcessing.H"

////////////////////////////////////////////////////////////////////////////////
// a dumb interact that just draw's a square you can drag
static const OfxPointD kBoxSize = {20, 20};

class ChoiceParamsInteract : public OFX::OverlayInteract {
protected :
  enum StateEnum {
    eInActive,
    ePoised,
    ePicked
  };

  OfxPointD _position;
  StateEnum _state;

public :
  ChoiceParamsInteract(OfxInteractHandle handle, OFX::ImageEffect* /*effect*/)
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
class ChoiceParamsPlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;
  OFX::Clip *srcClip_;
  OFX::Clip *maskClip_;

  OFX::ChoiceParam  *red_choice_;
  OFX::ChoiceParam  *green_choice_;
  OFX::StrChoiceParam  *blue_choice_;

public :
  /** @brief ctor */
  ChoiceParamsPlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , red_choice_(0)
    , green_choice_(0)
    , blue_choice_()
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    // name of mask clip depends on the context
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    red_choice_   = fetchChoiceParam("red_choice");
    green_choice_  = fetchChoiceParam("green_choice");
    if (OFX::getImageEffectHostDescription()->supportsStrChoice)  {
      blue_choice_ = fetchStrChoiceParam("blue_choice");
    }
  }

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
ChoiceParamsPlugin::setupAndProcess(ImageScalerBase &processor, const OFX::RenderArguments &args)
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

  int ri = 0;
  int gi = 0;
  std::string bi;
  double r = 0, g = 0, b = 0, a = 0;

  red_choice_->getValueAtTime(args.time, ri);
  green_choice_->getValueAtTime(args.time, gi);

  if (ri == 0)
    r = 0;
  if (ri == 1)
    r = 0.5;
  if (ri == 2)
    r = 1.0;

  // Note that green options are out of order
  if (gi == 0)
    g = 0;
  if (gi == 2)
    g = 0.5;
  if (gi == 1)
    g = 1.0;

  if (OFX::getImageEffectHostDescription()->supportsStrChoice)  {
    blue_choice_->getValueAtTime(args.time, bi);
    if (bi == "blue_0.0")
      b = 0;
    if (bi == "blue_0.5")
      b = 0.5;
    if (bi == "blue_1.0")
      b = 1.0;
  } else {
    b = 1.0;
  }

  a = 1.0;                      // always

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
ChoiceParamsPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
  // our RoD is the same as the 'Source' clip's, we are not interested in the mask
  rod = srcClip_->getRegionOfDefinition(args.time);

  // say we set it
  return true;
}

// override the roi call
void 
ChoiceParamsPlugin::getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois)
{
  // we don't actually need to do this as this is the default, but do it for examples sake
  rois.setRegionOfInterest(*srcClip_, args.regionOfInterest);

  // set it on the mask only if we are in an interesting context
  if(getContext() != OFX::eContextFilter)
    rois.setRegionOfInterest(*maskClip_, args.regionOfInterest);
}

// the overridden render function
void
ChoiceParamsPlugin::render(const OFX::RenderArguments &args)
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
ChoiceParamsPlugin:: isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  return false;
}

// we have changed a param
void
ChoiceParamsPlugin::changedParam(const OFX::InstanceChangedArgs &/*args*/, const std::string &paramName)
{
}

// we have changed a param
void
ChoiceParamsPlugin::changedClip(const OFX::InstanceChangedArgs &/*args*/, const std::string &clipName)
{
}

////////////////////////////////////////////////////////////////////////////////
// stuff for the interact 

// draw the interact
bool ChoiceParamsInteract::draw(const OFX::DrawArgs &args)
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
ChoiceParamsInteract::penMotion(const OFX::PenArgs &args)
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
ChoiceParamsInteract::penDown(const OFX::PenArgs &args)
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
ChoiceParamsInteract::penUp(const OFX::PenArgs &args)
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

mDeclarePluginFactory(ChoiceParamsExamplePluginFactory, {}, {});

class ChoiceParamsExampleOverlayDescriptor : public DefaultEffectOverlayDescriptor<ChoiceParamsExampleOverlayDescriptor, ChoiceParamsInteract> {};

void ChoiceParamsExamplePluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
  // basic labels
  desc.setLabels("Choice Params (Support)", "ChoiceParams(spt)", "Choice Params (Support)");
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

  desc.setOverlayInteractDescriptor( new ChoiceParamsExampleOverlayDescriptor);
}

void ChoiceParamsExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
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

  auto *choice1 = desc.defineChoiceParam("red_choice");
  choice1->appendOption("red: none");
  choice1->appendOption("red: some");
  choice1->appendOption("red: lots");
  choice1->setDefault(0);
  page->addChild(*choice1);

  // Note: index 1 is "lots" (even though UI order is 2), index 2 is "some"
  // because options are appended in order.
  auto *choice2 = desc.defineChoiceParam("green_choice");
  choice2->appendOption("green: none", "", 0);
  choice2->appendOption("green: lots", "", 2);
  choice2->appendOption("green: some", "", 1);
  choice2->setDefault(0);
  page->addChild(*choice2);

  if (getImageEffectHostDescription()->supportsStrChoice)  {
    auto *choice3 = desc.defineStrChoiceParam("blue_choice");
    choice3->appendOption("blue_0.0", "blue: none", 0);
    choice3->appendOption("blue_0.5", "blue: some", 1);
    choice3->appendOption("blue_1.0", "blue: lots", 2);
    choice3->setDefault("blue_0.0");
    page->addChild(*choice3);
  }
}

ImageEffect *ChoiceParamsExamplePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
  return new ChoiceParamsPlugin(handle);
}

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static ChoiceParamsExamplePluginFactory p("org.openeffects.support.choiceParamsPlugin", 1, 0);
      ids.push_back(&p);
    }  
  }
}
