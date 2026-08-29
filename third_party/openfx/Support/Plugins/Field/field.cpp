

#ifdef _WINDOWS
#include <windows.h>
#endif

#include <iostream>
#include <stdio.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

#include "../include/ofxsProcessing.H"


// Base class for the RGBA and the Alpha processor
class FieldBase : public OFX::ImageProcessor {
protected :
  OFX::Image *_srcImg;
  OFX::FieldEnum _field;
public :
  /** @brief no arg ctor */
  FieldBase(OFX::ImageEffect &instance, OFX::FieldEnum field)
    : OFX::ImageProcessor(instance)
      , _srcImg(0), _field(field)
  {        
  }

  /** @brief set the src image */
  void setSrcImg(OFX::Image *v) {_srcImg = v;}
};

// template to do the RGBA processing
template <class PIX, int nComponents, int max>
class ImageFielder : public FieldBase {
public :
  // ctor
  ImageFielder(OFX::ImageEffect &instance, OFX::FieldEnum field) 
    : FieldBase(instance, field)
  {}

  // and do some processing
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    //eFieldLower only the spatially lower field is present
    //eFieldUpper only the spatially upper field is present
 
    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(_effect.abort()) break;

      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {

        PIX *srcPix = (PIX *)  (_srcImg ? _srcImg->getPixelAddress(x, y) : 0);

        // do we have a source image to scale up
        if(srcPix) {
          for(int c = 0; c < nComponents; c++) {
            if((_field == OFX::eFieldLower) && (c==0))
              dstPix[c] = max;
            else if((_field == OFX::eFieldUpper) && (c==2))
              dstPix[c] = max;
            else
              dstPix[c] = max - srcPix[c];
          }
        }
        else {
          // no src pixel here, be black and transparent
          for(int c = 0; c < nComponents; c++) {
            if((_field == OFX::eFieldLower) && (c==0))
              dstPix[c] = max;
            else if((_field == OFX::eFieldUpper) && (c==2))
              dstPix[c] = max;
            else
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
class FieldPlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;
  OFX::Clip *srcClip_;

public :
  /** @brief ctor */
  FieldPlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
  }

  /* Override the render */
  virtual void render(const OFX::RenderArguments &args);

  /* set up and run a processor */
  void setupAndProcess(FieldBase &, const OFX::RenderArguments &args);
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from


/* set up and run a processor */
void
FieldPlugin::setupAndProcess(FieldBase &processor, const OFX::RenderArguments &args)
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

  // set the images
  processor.setDstImg(dst.get());
  processor.setSrcImg(src.get());

  // set the render window
  processor.setRenderWindow(args.renderWindow);

  // Call the base class process member, this will call the derived templated process code
  processor.process();
}

// the overridden render function
void
FieldPlugin::render(const OFX::RenderArguments &args)
{
  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  double time = args.time;
  std::cout << "Rendering at time " << time << std::endl;
  OFX::FieldEnum field = args.fieldToRender;

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) 
  {
    switch(dstBitDepth) 
    {
    case OFX::eBitDepthUByte : 
    {      
      ImageFielder<unsigned char, 4, 255> fred(*this, field);
      setupAndProcess(fred, args);
    }
    break;
    
    case OFX::eBitDepthUShort : 
    {
      ImageFielder<unsigned short, 4, 65535> fred(*this, field);
      setupAndProcess(fred, args);
    }                          
    break;
    
    case OFX::eBitDepthFloat : 
    {
      ImageFielder<float, 4, 1> fred(*this, field);
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
      ImageFielder<unsigned char, 1, 255> fred(*this, field);
      setupAndProcess(fred, args);
    }
    break;
      
    case OFX::eBitDepthUShort : 
    {
      ImageFielder<unsigned short, 1, 65535> fred(*this, field);
      setupAndProcess(fred, args);
    }                          
    break;
      
    case OFX::eBitDepthFloat : 
    {
      ImageFielder<float, 1, 1> fred(*this, field);
      setupAndProcess(fred, args);
    }                          
    break;
    default :
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } 
}

mDeclarePluginFactory(FieldExamplePluginFactory, {}, {});

using namespace OFX;
void FieldExamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  // basic labels
  desc.setLabels("Field", "Field", "Field");
  desc.setPluginGrouping("OFX Example (Support)");

  // add the supported contexts, only filter at the moment
  desc.addSupportedContext(eContextFilter);

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
  desc.setRenderTwiceAlways(true);
  desc.setSupportsMultipleClipPARs(false);
}

void FieldExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
  // Source clip only in the filter context
  // create the mandated source clip
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);
  srcClip->setFieldExtraction(eFieldExtractSingle);

  // create the mandated output clip
  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

}

OFX::ImageEffect* FieldExamplePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/)
{
  return new FieldPlugin(handle);
}

namespace OFX 
{
  namespace Plugin 
  {  
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static FieldExamplePluginFactory p("net.sf.openfx.fieldPlugin", 1, 0);
      ids.push_back(&p);
    }
  }
}
