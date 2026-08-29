

#ifdef _WINDOWS
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <stdio.h>
#include <math.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "../include/ofxsProcessing.H"

#include "multibundle1.h"

static const OfxPointD kBoxSize = {20, 20};
class GammaInteract : public OFX::OverlayInteract 
{
protected :
  enum StateEnum {
    eInActive,
    ePoised,
    ePicked
  };
  OfxPointD _position;
  StateEnum _state;
public :
  GammaInteract(OfxInteractHandle handle, OFX::ImageEffect* /*effect*/) : OFX::OverlayInteract(handle), _state(eInActive)
  {
    _position.x = 0;
    _position.y = 0;
  }
  virtual bool draw(const OFX::DrawArgs &args);
  virtual bool penMotion(const OFX::PenArgs &args);
  virtual bool penDown(const OFX::PenArgs &args);
  virtual bool penUp(const OFX::PenArgs &args);
};

template <class T> 
inline T Absolute(T a) 
{ 
  return (a < 0) ? -a : a;
}

template <class T> 
inline T Clamp(T v, int min, int max)
{
  if(v < T(min)) 
    return T(min);
  if(v > T(max)) 
    return T(max);
  return v;
}

class ImageScalerBase : public OFX::ImageProcessor 
{
protected :
  OFX::Image *_srcImg;
  OFX::Image *_maskImg;
  double _rScale, _gScale, _bScale, _aScale;
  bool   _doMasking;

public :
  ImageScalerBase(OFX::ImageEffect &instance): OFX::ImageProcessor(instance), _srcImg(0), _maskImg(0),
    _rScale(1), _gScale(1), _bScale(1), _aScale(1), _doMasking(false)
  {        
  }
  void setSrcImg(OFX::Image *v) {_srcImg = v;}
  void setMaskImg(OFX::Image *v) {_maskImg = v;}
  void doMasking(bool v) {_doMasking = v;}
  void setScales(float r, float g, float b, float a)
  {
    _rScale = r;
    _gScale = g;
    _bScale = b;
    _aScale = a;
  }
};

template <class PIX, int nComponents, int max>
class ImageScaler : public ImageScalerBase 
{
public :
  ImageScaler(OFX::ImageEffect &instance): ImageScalerBase(instance)
  {}
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    float scales[4];
    scales[0] = nComponents == 1 ? (float)_aScale : (float)_rScale;
    scales[1] = (float)_gScale;
    scales[2] = (float)_bScale;
    scales[3] = (float)_aScale;
    float maskScale = 1.0f;
    for(int y = procWindow.y1; y < procWindow.y2; y++) 
    {
      if(_effect.abort()) 
        break;
      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);
      for(int x = procWindow.x1; x < procWindow.x2; x++) 
      {
        PIX *srcPix = (PIX *)  (_srcImg ? _srcImg->getPixelAddress(x, y) : 0);
        if(_doMasking) 
        {
          if(!_maskImg)
            maskScale = 1.0f;
          else
          {
            PIX *maskPix = (PIX *)  (_maskImg ? _maskImg->getPixelAddress(x, y) : 0);
            maskScale = maskPix != 0 ? float(*maskPix)/float(max) : 0.0f;
          }
        }
        if(srcPix) 
        {
          for(int c = 0; c < nComponents; c++) 
          {
            float v = (float)(pow((double)srcPix[c], (double)scales[c])) * maskScale + (1.0f - maskScale) * srcPix[c];
            if(max == 1)
              dstPix[c] = PIX(v);
            else
              dstPix[c] = PIX(Clamp(v, 0, max));
          }
        }
        else 
        {
          for(int c = 0; c < nComponents; c++)
            dstPix[c] = 0;
        }
        dstPix += nComponents;
      }
    }
  }
};


class GammaPlugin : public OFX::ImageEffect 
{
protected :
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
  GammaPlugin(OfxImageEffectHandle handle): ImageEffect(handle), dstClip_(0), srcClip_(0), scale_(0)
    , rScale_(0), gScale_(0), bScale_(0), aScale_(0), componentScalesEnabled_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    scale_   = fetchDoubleParam("scale");
    rScale_  = fetchDoubleParam("scaleR");
    gScale_  = fetchDoubleParam("scaleG");
    bScale_  = fetchDoubleParam("scaleB");
    aScale_  = fetchDoubleParam("scaleA");
    componentScalesEnabled_ = fetchBooleanParam("scaleComponents");
    setEnabledness();
  }
  void setEnabledness();
  virtual void render(const OFX::RenderArguments &args);
  virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime);
  virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName);
  virtual void changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName);
  virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);
  virtual void getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois);
  void setupAndProcess(ImageScalerBase &, const OFX::RenderArguments &args);
};

void GammaPlugin::setupAndProcess(ImageScalerBase &processor, const OFX::RenderArguments &args)
{
  std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
  OFX::BitDepthEnum dstBitDepth       = dst->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dst->getPixelComponents();
  std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));
  if(src.get()) 
  {
    OFX::BitDepthEnum    srcBitDepth      = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();
    if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
      throw int(1);
  }
  std::unique_ptr<OFX::Image> mask;
  if(getContext() != OFX::eContextFilter) 
  {
    mask.reset(maskClip_->fetchImage(args.time));
    processor.doMasking(true);
    processor.setMaskImg(mask.get());
  }
  double r, g, b, a = aScale_->getValueAtTime(args.time);
  r = g = b = scale_->getValueAtTime(args.time);
  if(componentScalesEnabled_->getValueAtTime(args.time)) 
  {
    r += rScale_->getValueAtTime(args.time);  
    g += gScale_->getValueAtTime(args.time);
    b += bScale_->getValueAtTime(args.time);
  }
  processor.setDstImg(dst.get());
  processor.setSrcImg(src.get());
  processor.setRenderWindow(args.renderWindow);
  processor.setScales((float)r, (float)g, (float)b, (float)a);
  processor.process();
}

bool GammaPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
  rod = srcClip_->getRegionOfDefinition(args.time);
  return true;
}

void GammaPlugin::getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois)
{
  rois.setRegionOfInterest(*srcClip_, args.regionOfInterest);
  if(getContext() != OFX::eContextFilter)
    rois.setRegionOfInterest(*maskClip_, args.regionOfInterest);
}

void GammaPlugin::render(const OFX::RenderArguments &args)
{
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();
  if(dstComponents == OFX::ePixelComponentRGBA) 
  {
    switch(dstBitDepth) 
    {
    case OFX::eBitDepthUByte :
      {
        ImageScaler<unsigned char, 4, 255> fred(*this);
        setupAndProcess(fred, args);
        break;
      }
    case OFX::eBitDepthUShort :
      {
        ImageScaler<unsigned short, 4, 65535> fred(*this);
        setupAndProcess(fred, args);
        break;
      }
    case OFX::eBitDepthFloat :
      {
        ImageScaler<float, 4, 1> fred(*this);
        setupAndProcess(fred, args);
        break;
      }
    default :
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
  else 
  {
    switch(dstBitDepth) 
    {
    case OFX::eBitDepthUByte :
      {
        ImageScaler<unsigned char, 1, 255> fred(*this);
        setupAndProcess(fred, args);
        break;
      }
    case OFX::eBitDepthUShort :
      {
        ImageScaler<unsigned short, 1, 65535> fred(*this);
        setupAndProcess(fred, args);
        break;
      }
    case OFX::eBitDepthFloat :
      {
        ImageScaler<float, 1, 1> fred(*this);
        setupAndProcess(fred, args);
        break;
      }
    default :
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } 
}

bool GammaPlugin:: isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  double scale = scale_->getValueAtTime(args.time);
  double rScale = 1, gScale = 1, bScale = 1, aScale = 1;
  if(componentScalesEnabled_->getValueAtTime(args.time)) {
    rScale = rScale_->getValueAtTime(args.time);
    gScale = gScale_->getValueAtTime(args.time);
    bScale = bScale_->getValueAtTime(args.time);
    aScale = aScale_->getValueAtTime(args.time);
  }
  rScale += scale; 
  gScale += scale; 
  bScale += scale;
  if(rScale == 1 && gScale == 1 && bScale == 1 && aScale == 1) {
    identityClip = srcClip_;
    identityTime = args.time;
    return true;
  }
  return false;
}

void GammaPlugin::setEnabledness(void)
{
  bool v = componentScalesEnabled_->getValue() && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA;
  rScale_->setEnabled(v);
  gScale_->setEnabled(v);
  bScale_->setEnabled(v);
  aScale_->setEnabled(v);
}

void GammaPlugin::changedParam(const OFX::InstanceChangedArgs &/*args*/, const std::string &paramName)
{
  if(paramName == "scaleComponents")  
    setEnabledness(); 
}

void GammaPlugin::changedClip(const OFX::InstanceChangedArgs &/*args*/, const std::string &clipName)
{
  if(clipName == kOfxImageEffectSimpleSourceClipName)  
    setEnabledness();
}

bool GammaInteract::draw(const OFX::DrawArgs &args)
{
  OfxRGBColourF col;
  switch(_state) {
case eInActive : 
  col.r = col.g = col.b = 0.0f; 
  break;
case ePoised   : 
  col.r = col.g = col.b = 0.5f; 
  break;
case ePicked   : 
  col.r = col.g = col.b = 1.0f; 
  break;
  }

  float dx = (float)(kBoxSize.x * args.pixelScale.x);
  float dy = (float)(kBoxSize.y * args.pixelScale.y);
  glPushMatrix();
  glColor3f(col.r, col.g, col.b);
  glTranslated(_position.x, _position.y, 0);
  glBegin(GL_POLYGON);
  glVertex2f(-dx, -dy);
  glVertex2f(-dx,  dy);
  glVertex2f( dx,  dy);
  glVertex2f( dx, -dy);
  glEnd();
  glPopMatrix();
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

bool GammaInteract::penMotion(const OFX::PenArgs &args)
{
  float dx = (float)(kBoxSize.x * args.pixelScale.x);
  float dy = (float)(kBoxSize.y * args.pixelScale.y);
  OfxPointD penPos = args.penPosition;
  switch(_state) 
  {
  case eInActive : 
  case ePoised   : 
    {
      StateEnum newState;
      penPos.x -= _position.x;
      penPos.y -= _position.y;
      if(Absolute(penPos.x) < dx && Absolute(penPos.y) < dy) 
      {
        newState = ePoised;
      }
      else 
      {
        newState = eInActive;
      }

      if(_state != newState) 
      {
        _state = newState;
        _effect->redrawOverlays();
      }
    }
    break;
  case ePicked   : 
    {
      _position = penPos;
      _effect->redrawOverlays();
    }
    break;
  }
  return _state != eInActive;
}

bool GammaInteract::penDown(const OFX::PenArgs &args)
{
  penMotion(args);
  if(_state == ePoised) 
  {
    _state = ePicked;
    _position = args.penPosition;
    _effect->redrawOverlays();
  }
  return _state == ePicked;
}

bool GammaInteract::penUp(const OFX::PenArgs &args)
{
  if(_state == ePicked) 
  {
    _state = ePoised;
    penMotion(args);
    _effect->redrawOverlays();
    return true;
  }
  return false;
}

using namespace OFX;

class GammaOverlayDescriptor : public DefaultEffectOverlayDescriptor<GammaOverlayDescriptor, GammaInteract> {};
    
void GammaExamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  desc.setLabels("Gamma", "Gamma", "Gamma");
  desc.setPluginGrouping("OFX Example (Support)");
  desc.addSupportedContext(eContextFilter);
  desc.addSupportedContext(eContextGeneral);
  desc.addSupportedContext(eContextPaint);
  desc.addSupportedBitDepth(eBitDepthUByte);
  desc.addSupportedBitDepth(eBitDepthUShort);
  desc.addSupportedBitDepth(eBitDepthFloat);
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(true);
  desc.setTemporalClipAccess(false);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);
  desc.setOverlayInteractDescriptor( new GammaOverlayDescriptor );
}

static
DoubleParamDescriptor *defineScaleParam(OFX::ImageEffectDescriptor &desc,
                                        const std::string &name, const std::string &label, const std::string &hint,
                                        GroupParamDescriptor *parent, double def = 1.0)
{
  DoubleParamDescriptor *param = desc.defineDoubleParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  param->setHint(hint);
  param->setDefault(def);
  param->setRange(0, 10);
  param->setIncrement(0.1);
  param->setDisplayRange(0, 10);
  param->setDoubleType(eDoubleTypeScale);
  if(parent) 
    param->setParent(*parent);
  return param;
}

void GammaExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context)
{
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);

  if(context == eContextGeneral || context == eContextPaint) 
  {
    ClipDescriptor *maskClip = context == eContextGeneral ? desc.defineClip("Mask") : desc.defineClip("Brush");
    maskClip->addSupportedComponent(ePixelComponentAlpha);
    maskClip->setTemporalClipAccess(false);
    if(context == eContextGeneral)
      maskClip->setOptional(true);
    maskClip->setSupportsTiles(true); 
    maskClip->setIsMask(true);
  }

  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

  PageParamDescriptor *page = desc.definePageParam("Controls");

  GroupParamDescriptor *componentScalesGroup = desc.defineGroupParam("componentScales");
  componentScalesGroup->setHint("Scales on the individual component");
  componentScalesGroup->setLabels("Components", "Components", "Components");            

  DoubleParamDescriptor *param = defineScaleParam(desc, "scale", "scale", "Scales all component in the image", 0, 1.0);
  page->addChild(*param);

  BooleanParamDescriptor *boolP = desc.defineBooleanParam("scaleComponents");
  boolP->setDefault(true);
  boolP->setHint("Enables gamma correction on individual components");
  boolP->setLabels("Gamma Components", "Gamma Components", "Gamma Components");
  boolP->setParent(*componentScalesGroup);
  page->addChild(*boolP);

  param = defineScaleParam(desc, "scaleR", "red", "Gamma corrects the red component of the image", componentScalesGroup, 0.0);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleG", "green", "Gamma corrects the green component of the image", componentScalesGroup, 0.0);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleB", "blue", "Gamma corrects the blue component of the image", componentScalesGroup, 0.0);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleA", "alpha", "Gamma corrects the alpha component of the image", componentScalesGroup, 0.0);
  page->addChild(*param);

}

ImageEffect* GammaExamplePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/)
{
  return new GammaPlugin(handle);
}

