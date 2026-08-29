

#ifdef _WINDOWS
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <cassert>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "../include/ofxsProcessing.H"

#include "multibundle2.h"

static const OfxPointD kBoxSize = {20, 20};

class DotExampleInteract : public OFX::OverlayInteract 
{
protected :
  enum StateEnum {
    eInActive,
    ePoised,
    ePicked
  };
  StateEnum _state;
  OFX::Double2DParam* position_;
public :
  DotExampleInteract(OfxInteractHandle handle, OFX::ImageEffect* effect) : OFX::OverlayInteract(handle), _state(eInActive)
  {
    position_ = effect->fetchDouble2DParam("position");
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

class DotGeneratorBase : public OFX::ImageProcessor 
{  
public :
  DotGeneratorBase(OFX::ImageEffect &instance) : OFX::ImageProcessor(instance), _radius(0.0f), _positionx(0.0f), _positiony(0.0f)
  {
    _colour[0] = _colour[1] = _colour[2] = _colour[3] = 0;
  }
  void setRadius(float v) { _radius = v; }
  void setR(float v) { _colour[0] = v;}
  void setG(float v) { _colour[1] = v;}
  void setB(float v) { _colour[2] = v;}
  void setA(float v) { _colour[3] = v;}
  void setColour(double r, double g, double b, double a)
  {
    setR((float)r);
    setG((float)g);
    setB((float)b);
    setA((float)a);
  }
  void setPosition(float x, float y) 
  {
    _positionx = x; 
    _positiony = y; 
  }
protected:
  float _radius;
  float _colour[4];
  float _positionx;
  float _positiony;
};

template <class PIX, int nComponents, int max>
class DotGenerator : public DotGeneratorBase {
public :
  DotGenerator(OFX::ImageEffect &instance): DotGeneratorBase(instance){}
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    float radiusSq = _radius * _radius;
    for(int y = procWindow.y1; y < procWindow.y2; y++) 
    {
      if(_effect.abort()) 
        break;
      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);
      for(int x = procWindow.x1; x < procWindow.x2; x++) 
      {
        float radsq = (x - _positionx)*(x - _positionx) + (y - _positiony)*(y - _positiony);
        float val = radsq/radiusSq;
        for(int c = 0; c < nComponents; c++) 
        {
          dstPix[c] = val < 1.0f ? PIX((1.0f-val)*max) : PIX(0);
          dstPix[c] = (PIX)(dstPix[c] * _colour[c]);
        }
        dstPix += nComponents;
      }
    }
  }
};

class DotExamplePlugin : public OFX::ImageEffect 
{
protected:
  OFX::Clip *dstClip_;
  OFX::DoubleParam  *radius_;
  OFX::RGBAParam *colour_;
  OFX::Double2DParam* position_;
public:
  DotExamplePlugin(OfxImageEffectHandle handle): ImageEffect(handle), dstClip_(0), radius_(0) , colour_(0), position_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    radius_   = fetchDoubleParam("radius");
    colour_ = fetchRGBAParam("colour");
    position_ = fetchDouble2DParam("position");
  }
  virtual void render(const OFX::RenderArguments &args);
  void setupAndProcess(DotGeneratorBase &, const OFX::RenderArguments &args);
  bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);
  template<class ARGS>
  void getPositionInPixels(double& x, double& y, const ARGS& args);
  template<class ARGS>
  void getPositionInCanonical(double& x, double& y, const ARGS& args);
};

template<class ARGS>
void DotExamplePlugin::getPositionInCanonical(double& x, double& y, const ARGS& args)
{
  position_->getValueAtTime(args.time, x, y);
}

template<class ARGS>
void DotExamplePlugin::getPositionInPixels(double& x, double& y, const ARGS& args)
{
  double xpos, ypos;
  getPositionInCanonical(xpos, ypos, args);
  x = (xpos * args.renderScale.x) /getProjectPixelAspectRatio();
  y = ypos * args.renderScale.y;
}

void DotExamplePlugin::setupAndProcess(DotGeneratorBase &processor, const OFX::RenderArguments &args)
{
  std::unique_ptr<OFX::Image>  dst(dstClip_->fetchImage(args.time));
  //OFX::BitDepthEnum         dstBitDepth    = dst->getPixelDepth();
  //OFX::PixelComponentEnum   dstComponents  = dst->getPixelComponents();
  double rad = radius_->getValueAtTime(args.time);
  processor.setRadius((float)(rad));
  double r, g, b, a;
  colour_->getValueAtTime(args.time, r, g, b, a);
  processor.setColour(r,g,b,a);
  processor.setDstImg(dst.get());
  processor.setRenderWindow(args.renderWindow);
  double xpospixel, ypospixel;
  getPositionInPixels(xpospixel, ypospixel, args);
  float fieldScaler = (args.fieldToRender == OFX::eFieldLower || args.fieldToRender == OFX::eFieldUpper)? 0.5f: 1.0f;
  ypospixel *= fieldScaler;
  processor.setPosition((float)xpospixel, (float)ypospixel);
  processor.process();
}


bool DotExamplePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
  double r = radius_->getValueAtTime(args.time);
  double x, y;
  position_->getValueAtTime(args.time, x, y);
  rod.x1 = x - r;
  rod.y1 = y - r;
  rod.x2 = x + r;
  rod.y2 = y + r;
  return true;
}

void DotExamplePlugin::render(const OFX::RenderArguments &args)
{
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();
  if(dstComponents == OFX::ePixelComponentRGBA) 
  {
    switch(dstBitDepth) 
    {
    case OFX::eBitDepthUByte : 
      {
        DotGenerator<unsigned char, 4, 255> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
    case OFX::eBitDepthUShort : 
      {
        DotGenerator<unsigned short, 4, 65535> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
    case OFX::eBitDepthFloat : 
      {
        DotGenerator<float, 4, 1> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
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
        DotGenerator<unsigned char, 1, 255> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
    case OFX::eBitDepthUShort : 
      {
        DotGenerator<unsigned short, 1, 65535> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
    case OFX::eBitDepthFloat : 
      {
        DotGenerator<float, 1, 1> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
    default :
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } 
}


bool DotExampleInteract::draw(const OFX::DrawArgs &args)
{
  OfxRGBColourF col;
  col.r = 0.5f;
  col.g = 0.0f;

  switch(_state) 
  {
    case eInActive : 
      col.b = 0.0f; 
      break;
    case ePoised   : 
      col.b = 0.5f; 
      break;
    case ePicked   : 
      col.b = 1.0f; 
      break;
  }

  float dx = (float)(kBoxSize.x * args.pixelScale.x);
  float dy = (float)(kBoxSize.y * args.pixelScale.y);
  double xpos = 0., ypos = 0.;
  DotExamplePlugin* plug = dynamic_cast<DotExamplePlugin*>(_effect);
  assert(plug);
  if (plug) {
    plug->getPositionInCanonical(xpos, ypos, args);
  }
  glPushMatrix();
  glColor3f(col.r, col.g, col.b);
  glTranslated(xpos, ypos, 0);
  glBegin(GL_POLYGON);
  glVertex2f(-dx, -dy);
  glVertex2f(-dx,  dy);
  glVertex2f( dx,  dy);
  glVertex2f( dx, -dy);
  glEnd();
  glPopMatrix();
  glColor3f(1.0f - col.r, 1.0f - col.g, 1.0f - col.b);
  glTranslated(xpos, ypos, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(-dx, -dy);
  glVertex2f(-dx,  dy);
  glVertex2f( dx,  dy);
  glVertex2f( dx, -dy);
  glEnd();
  glPopMatrix();
  return true;
}

bool DotExampleInteract::penMotion(const OFX::PenArgs &args)
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
      double xpos = 0., ypos = 0.;
      DotExamplePlugin* plug = dynamic_cast<DotExamplePlugin*>(_effect);
      if (plug) {
        plug->getPositionInCanonical(xpos, ypos, args);
      }
      penPos.x -= xpos;
      penPos.y -= ypos;
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
      
    position_->setValueAtTime(args.time, args.penPosition.x, args.penPosition.y);
      _effect->redrawOverlays();
    }
    break;
  }
  return _state != eInActive;
}

bool DotExampleInteract::penDown(const OFX::PenArgs &args)
{
  penMotion(args);
  if(_state == ePoised) 
  {
    _state = ePicked;
    _effect->redrawOverlays();
  }
  return _state == ePicked;
}

bool DotExampleInteract::penUp(const OFX::PenArgs &args)
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

class DotExampleOverlayDescriptor : public DefaultEffectOverlayDescriptor<DotExampleOverlayDescriptor, DotExampleInteract> {};

void DotExamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  desc.setLabels("Dot Generator", "Dot Generator", "Dot Generator");
  desc.setPluginGrouping("OFX Example (Support)");
  desc.addSupportedContext(eContextGenerator);
  desc.addSupportedContext(eContextGeneral);
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
  desc.setRenderTwiceAlways(false);
  desc.setOverlayInteractDescriptor( new DotExampleOverlayDescriptor );
}

void DotExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
  // there has to be an input clip, even for generators
  ClipDescriptor* srcClip = desc.defineClip( kOfxImageEffectSimpleSourceClipName );
  srcClip->addSupportedComponent( OFX::ePixelComponentRGBA );
  srcClip->addSupportedComponent( OFX::ePixelComponentAlpha );
  srcClip->setSupportsTiles(true);
  srcClip->setOptional(true);

  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);
  dstClip->setFieldExtraction(eFieldExtractSingle);

  DoubleParamDescriptor *param = desc.defineDoubleParam("radius");
  param->setLabels("Radius", "Radius", "Radius");
  param->setScriptName("radius");
  param->setHint("The radius of the dot produced.");
  param->setDoubleType(eDoubleTypeX);
  param->setDefaultCoordinateSystem(eCoordinatesNormalised);
  param->setDefault(0.02);
  //param->setRange(0, 1);
  param->setIncrement(1);
  param->setDisplayRange(0, 1);
  param->setAnimates(true);

  RGBAParamDescriptor *param2 = desc.defineRGBAParam("colour");
  param2->setAnimates(true);
  param2->setLabels("Colour", "Colour", "Colour");
  param2->setHint("The colour of the dot produced.");
  param2->setScriptName("colour");
  param2->setDefault(1.0, 1.0, 1.0, 1.0);

  Double2DParamDescriptor* param3 = desc.defineDouble2DParam("position");
  param3->setLabels("Dot Position", "Dot Position", "Dot Position");
  param3->setDoubleType(eDoubleTypeXY);
  param3->setDefaultCoordinateSystem(eCoordinatesNormalised);
  param3->setAnimates(true);
  param3->setDimensionLabels("X", "Y");
  param3->setDefault(0.5, 0.5);

  PageParamDescriptor *page = desc.definePageParam("Controls");
  page->addChild(*param);
  page->addChild(*param2);
  page->addChild(*param3);
}

ImageEffect* DotExamplePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/)
{
  return new DotExamplePlugin(handle);
}

