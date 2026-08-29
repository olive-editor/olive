

#ifdef _WINDOWS
#include <windows.h>
#endif

#include <stdio.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

#include "../include/ofxsProcessing.H"


// Base class for the RGBA and the Alpha processor
class InvertBase : public OFX::ImageProcessor {
protected :
  OFX::Image *_srcImg;
public :
  /** @brief no arg ctor */
  InvertBase(OFX::ImageEffect &instance)
    : OFX::ImageProcessor(instance)
    , _srcImg(0)
  {        
  }

  /** @brief set the src image */
  void setSrcImg(OFX::Image *v) {_srcImg = v;}
};

// template to do the RGBA processing
template <class PIX, int nComponents, int max>
class ImageInverter : public InvertBase {
public :
  // ctor
  ImageInverter(OFX::ImageEffect &instance) 
    : InvertBase(instance)
  {}

  // and do some processing
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(_effect.abort()) break;

      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {

        PIX *srcPix = (PIX *)  (_srcImg ? _srcImg->getPixelAddress(x, y) : 0);

        // do we have a source image to scale up
        if(srcPix) {
          for(int c = 0; c < nComponents; c++) {
            dstPix[c] = max - srcPix[c];
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
class InvertPlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;
  OFX::Clip *srcClip_;

public :
  /** @brief ctor */
  InvertPlugin(OfxImageEffectHandle handle)
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
  void setupAndProcess(InvertBase &, const OFX::RenderArguments &args);
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from


/* set up and run a processor */
void
InvertPlugin::setupAndProcess(InvertBase &processor, const OFX::RenderArguments &args)
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
InvertPlugin::render(const OFX::RenderArguments &args)
{
  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) {
    switch(dstBitDepth) {
case OFX::eBitDepthUByte : {      
  ImageInverter<unsigned char, 4, 255> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  ImageInverter<unsigned short, 4, 65535> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  ImageInverter<float, 4, 1> fred(*this);
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
  ImageInverter<unsigned char, 1, 255> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  ImageInverter<unsigned short, 1, 65535> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  ImageInverter<float, 1, 1> fred(*this);
  setupAndProcess(fred, args);
                           }                          
                           break;
default :
  OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } 
}

mDeclarePluginFactory(InvertExamplePluginFactory, {}, {});

using namespace OFX;
void InvertExamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  // basic labels
  desc.setLabels("Invert", "Invert", "Invert");
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
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);

}

void InvertExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
  // Source clip only in the filter context
  // create the mandated source clip
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);

  // create the mandated output clip
  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

}

OFX::ImageEffect* InvertExamplePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/)
{
  return new InvertPlugin(handle);
}

namespace OFX 
{
  namespace Plugin 
  {  
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static InvertExamplePluginFactory p("net.sf.openfx.invertPlugin", 1, 0);
      ids.push_back(&p);
    }
  }
}
