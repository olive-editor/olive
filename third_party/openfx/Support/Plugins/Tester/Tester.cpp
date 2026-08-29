

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
#include "ofxsInteract.h"

#include "../include/ofxsProcessing.H"

static const OfxPointD kBoxSize = {5, 5};

template <class T> 
inline T Minimum(T a, T b) {    return (a < b) ? a : b;}

template <class T> 
inline T Absolute(T a) { return (a < 0) ? -a : a;}

class PositionInteract : public OFX::OverlayInteract 
{
protected :
  enum StateEnum {
    eInActive,
    ePoised,
    ePicked
  };

  StateEnum _state;
  OFX::Double2DParam* _position;
public :
  PositionInteract(OfxInteractHandle handle, OFX::ImageEffect* effect) 
    : OFX::OverlayInteract(handle)
    , _state(eInActive)
  {
    _position = effect->fetchDouble2DParam("widgetPos");
  }

  // overridden functions from OFX::Interact to do things
  virtual bool draw(const OFX::DrawArgs &args);
  virtual bool penMotion(const OFX::PenArgs &args);
  virtual bool penDown(const OFX::PenArgs &args);
  virtual bool penUp(const OFX::PenArgs &args);
  OfxPointD getCanonicalPosition(double time) const
  {
    OfxPointD retVal;
    _position->getValueAtTime(time, retVal.x, retVal.y);
    return retVal; 
  }
  void setCanonicalPosition(double x, double y, double time)
  {
    _position->setValueAtTime(time, x, y);
  }
};

bool PositionInteract::draw(const OFX::DrawArgs &args)
{
  OfxRGBColourF col;
  switch(_state) 
  {
  case eInActive : col.r = col.g = col.b = 0.0f; break;
  case ePoised   : col.r = col.g = col.b = 0.5f; break;
  case ePicked   : col.r = col.g = col.b = 1.0f; break;
  }

  // make the box a constant size on screen by scaling by the pixel scale
  float dx = (float)(kBoxSize.x / args.pixelScale.x);
  float dy = (float)(kBoxSize.y / args.pixelScale.y);

  // Draw a cross hair, the current coordinate system aligns with the image plane.
  glPushMatrix();

  // draw the bo
  OfxPointD pos = getCanonicalPosition(args.time);
  glColor3f(col.r, col.g, col.b);
  glTranslated(pos.x, pos.y, 0);
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
  glTranslated(pos.x, pos.y, 0);
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
bool PositionInteract::penMotion(const OFX::PenArgs &args)
{
  // figure the size of the box in cannonical coords
  float dx = (float)(kBoxSize.x / args.pixelScale.x);
  float dy = (float)(kBoxSize.y / args.pixelScale.y);

  OfxPointD pos = getCanonicalPosition(args.time);

  // pen position is in cannonical coords
  OfxPointD penPos = args.penPosition;

  switch(_state) 
  {
  case eInActive : 
  case ePoised   : 
    {
      // are we in the box, become 'poised'
      StateEnum newState;
      penPos.x -= pos.x;
      penPos.y -= pos.y;
      if(Absolute(penPos.x) < dx &&
        Absolute(penPos.y) < dy) {
          newState = ePoised;
      }
      else {
        newState = eInActive;
      }

      if(_state != newState) {
        _state = newState;
        _effect->redrawOverlays();
      }
    }
    break;

  case ePicked   : 
    {
      setCanonicalPosition(penPos.x, penPos.y, args.time);
      _effect->redrawOverlays();
    }
    break;
  }
  return _state != eInActive;
}

bool PositionInteract::penDown(const OFX::PenArgs &args)
{
  penMotion(args);
  if(_state == ePoised) {
    _state = ePicked;
    setCanonicalPosition(args.penPosition.x, args.penPosition.y, args.time);
    _effect->redrawOverlays();
  }

  return _state == ePicked;
}

bool PositionInteract::penUp(const OFX::PenArgs &args)
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

class GenericTestBase : public OFX::ImageProcessor {
protected :
  OFX::Image *_srcImg;
public :
  GenericTestBase(OFX::ImageEffect &instance): OFX::ImageProcessor(instance), _srcImg(0)
  {        
  }
  void setSrcImg(OFX::Image *v) {_srcImg = v;}
};

template <class PIX, int nComponents, int max>
class ImageGenericTester : public GenericTestBase 
{
public :
  ImageGenericTester(OFX::ImageEffect &instance) : GenericTestBase(instance){}
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    for(int y = procWindow.y1; y < procWindow.y2; y++) 
    {
      if(_effect.abort()) 
        break;
      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);
      for(int x = procWindow.x1; x < procWindow.x2; x++) 
      {
        PIX *srcPix = (PIX *)  (_srcImg ? _srcImg->getPixelAddress(x, y) : 0);
        if(srcPix) 
        {
          for(int c = 0; c < nComponents; c++)
            dstPix[c] = max - srcPix[c];
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



template<class TypeCarrier, int kComponents, int kMax>
class Analyser
{
public:
  Analyser(OFX::Clip* srcClip, OFX::DoubleParam* dbl)
  {
    OfxRangeD range = srcClip->getFrameRange();
    for(double d = range.min; d< range.max; ++d)
    {
      std::unique_ptr<OFX::Image> src(srcClip->fetchImage(d));
      dbl->setValueAtTime(d, d);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class GenericTestPlugin : public OFX::ImageEffect 
{
protected :
  OFX::Clip *dstClip_;
  OFX::Clip *srcClip_;

public :
  GenericTestPlugin(OfxImageEffectHandle handle) : ImageEffect(handle), dstClip_(0), srcClip_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
  }

  virtual void render(const OFX::RenderArguments &args);
  void setupAndProcess(GenericTestBase &, const OFX::RenderArguments &args);
  void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
  {
    if(paramName=="enableTest")
    {
      OFX::ChoiceParam* choice  = fetchChoiceParam("enableTest");
      OFX::DoubleParam* dbl = fetchDoubleParam("enableDbl");
      int value = 0;
      choice->getValueAtTime(args.time, value);
      dbl->setEnabled(value ==0 );
    }
    else if(paramName=="pbButton")
    {
      sendMessage(OFX::Message::eMessageMessage, "", "Push Button Pressed - TestPassed!");
    }
    else if(paramName=="widgetPos")
    {
      redrawOverlays();
    }
    else if(paramName == "analyseButton")
    {
      OFX::BitDepthEnum       dstBitDepth    = srcClip_->getPixelDepth();
      OFX::PixelComponentEnum dstComponents  = srcClip_->getPixelComponents();
      OFX::DoubleParam* dbl = fetchDoubleParam("analysisParam");

      if(dstComponents == OFX::ePixelComponentRGBA) 
      {
        switch(dstBitDepth) 
        {
        case OFX::eBitDepthUByte : 
          {
            Analyser<unsigned char, 4, 255> analyse(srcClip_, dbl);
            break;
          }
        case OFX::eBitDepthUShort :
          {
            Analyser<unsigned short, 4, 65535> analyse(srcClip_, dbl);
            break;
          }
        case OFX::eBitDepthFloat :
          {
            Analyser<float, 4, 1> analyse(srcClip_, dbl);
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
            Analyser<unsigned char, 1, 255> analyse(srcClip_, dbl);
            break;
          }
        case OFX::eBitDepthUShort :
          {
            Analyser<unsigned short, 1, 65535> analyse(srcClip_, dbl);
            break;
          }
        case OFX::eBitDepthFloat : 
          {
            Analyser<float, 1, 1> analyse(srcClip_, dbl);
            break;
          }
        default :
          OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
        }
      }
    }
  }
};


void GenericTestPlugin::setupAndProcess(GenericTestBase &processor, const OFX::RenderArguments &args)
{
  std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
  OFX::BitDepthEnum dstBitDepth       = dst->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dst->getPixelComponents();
  std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

  if(src.get()) 
  {
    OFX::BitDepthEnum    srcBitDepth      = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    // see if they have the same depths and bytes and all
    if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
      throw int(1); // HACK!! need to throw an sensible exception here!
  }

  processor.setDstImg(dst.get());
  processor.setSrcImg(src.get());
  processor.setRenderWindow(args.renderWindow);
  processor.process();
}


void GenericTestPlugin::render(const OFX::RenderArguments &args)
{
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  if(dstComponents == OFX::ePixelComponentRGBA) 
  {
    switch(dstBitDepth) 
    {
    case OFX::eBitDepthUByte : 
      {      
        ImageGenericTester<unsigned char, 4, 255> fred(*this);
        setupAndProcess(fred, args);
      }
      break;

    case OFX::eBitDepthUShort : 
      {
        ImageGenericTester<unsigned short, 4, 65535> fred(*this);
        setupAndProcess(fred, args);
      }                          
      break;

    case OFX::eBitDepthFloat : 
      {
        ImageGenericTester<float, 4, 1> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
    default :
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
  else {
    switch(dstBitDepth) 
    {
    case OFX::eBitDepthUByte : 
      {
        ImageGenericTester<unsigned char, 1, 255> fred(*this);
        setupAndProcess(fred, args);
      }
      break;

    case OFX::eBitDepthUShort : 
      {
        ImageGenericTester<unsigned short, 1, 65535> fred(*this);
        setupAndProcess(fred, args);
      }                          
      break;

    case OFX::eBitDepthFloat : 
      {
        ImageGenericTester<float, 1, 1> fred(*this);
        setupAndProcess(fred, args);
      }                          
      break;
    default :
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } 
}

class PositionOverlayDescriptor : public OFX::DefaultEffectOverlayDescriptor<PositionOverlayDescriptor, PositionInteract> {};

mDeclarePluginFactory(GenericTestExamplePluginFactory, {}, {});

using namespace OFX;
void GenericTestExamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  desc.setLabels("GenericTest", "GenericTest", "GenericTest");
  desc.setPluginGrouping("OFX Example (Support)");
  desc.addSupportedContext(eContextFilter);
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

  desc.setOverlayInteractDescriptor( new PositionOverlayDescriptor);
}

void GenericTestExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);

  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

  DoubleParamDescriptor *param1 = desc.defineDoubleParam("MinMaxTest");
  param1->setLabels("Min/Max Test", "Min/Max Test", "Min/Max Test");
  param1->setScriptName("minMaxTest");
  param1->setHint("A double parameter to illustrate visual min/max.");
  param1->setDefault(50.0);
  param1->setRange(-1000, 1000);
  param1->setDisplayRange(-1000, 1000);
  param1->setDoubleType(eDoubleTypePlain);

  ChoiceParamDescriptor* param2 = desc.defineChoiceParam("enableTest");
  param2->setLabels("Enabler", "Enabler", "Enabler");
  param2->appendOption("Enable parameter", "Enable parameter");
  param2->appendOption("Disable parameter", "Disable parameter");

  DoubleParamDescriptor *param3 = desc.defineDoubleParam("enableDbl");
  param3->setLabels("Enabled by Enabler", "Enabled by Enabler", "Enabled by Enabler");


  BooleanParamDescriptor* bparam = desc.defineBooleanParam("Insignificant");
  bparam->setLabels("Insignificant", "Insignificant", "Insignificant");
  bparam->setHint("Shouldn't cause a re-render.");
  bparam->setEvaluateOnChange(false);

  BooleanParamDescriptor* bparam2 = desc.defineBooleanParam("secretTest");
  bparam2->setLabels("SECRET!", "SECRET!", "SECRET!");
  bparam2->setIsSecret(true);
  bparam2->setHint("Shouldn't be shown in the user interface.");

  BooleanParamDescriptor* bparam3 = desc.defineBooleanParam("nonPersistant");
  bparam3->setLabels("Non-persistant", "Non-persistant", "Non-persistant");
  bparam3->setHint("Shouldn't be saved in the plugin description.");
  bparam3->setIsPersistant(false);
    
  DoubleParamDescriptor *param5 = desc.defineDoubleParam("animateDbl");
  param5->setLabels("No Animation", "No Animation", "No Animation");
  param5->setAnimates(false);

  DoubleParamDescriptor *param6 = desc.defineDoubleParam("angleTest");
  param6->setLabels("Angle?", "Angle?", "Angle?");
  param6->setRange(-180.0, 180.0);
  param6->setHint("An angle parameter.");
  param6->setDoubleType(eDoubleTypeAngle);

  PushButtonParamDescriptor* pb = desc.definePushButtonParam("pbButton");
  pb->setLabels("Push Me", "Push Me", "Push Me");

  PushButtonParamDescriptor* pb2 = desc.definePushButtonParam("analyseButton");
  pb2->setLabels("Analyse", "Analyse", "Analyse");

  DoubleParamDescriptor *param7 = desc.defineDoubleParam("analysisParam");
  param7->setLabels("Analysis Slave", "Analysis Slave", "Analysis Slave");

  Double2DParamDescriptor* widgetPos = desc.defineDouble2DParam("widgetPos");
  widgetPos->setLabels("Widget Position", "Widget Position", "Widget Position");
  widgetPos->setDoubleType(OFX::eDoubleTypeXYAbsolute);
  widgetPos->setDefaultCoordinateSystem(eCoordinatesNormalised);
  widgetPos->setDimensionLabels("X Position", "Y Position");
  widgetPos->setDefault(0.5, 0.5);
}

OFX::ImageEffect* GenericTestExamplePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/)
{
  return new GenericTestPlugin(handle);
}

namespace OFX 
{
  namespace Plugin 
  {  
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static GenericTestExamplePluginFactory p("net.sf.openfx.GenericTestPlugin", 1, 0);
      ids.push_back(&p);
    }
  }
}
