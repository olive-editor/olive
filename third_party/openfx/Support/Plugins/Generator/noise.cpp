

#include <limits>
#include <stdio.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

#include "../include/ofxsProcessing.H"

#include <random>

////////////////////////////////////////////////////////////////////////////////
// base class for the noise

/** @brief  Base class used to blend two images together */
class NoiseGeneratorBase : public OFX::ImageProcessor {
protected :
  float       _noiseLevel;   // how much to blend
  uint32_t    _seed;    // base seed
public :
  /** @brief no arg ctor */
  NoiseGeneratorBase(OFX::ImageEffect &instance)
    : OFX::ImageProcessor(instance)
    , _noiseLevel(0.5f)
    , _seed(0)
  {
  }

  /** @brief set the scale */
  void setNoiseLevel(float v) {_noiseLevel = v;}

  /** @brief the seed to use */
  void setSeed(uint32_t v) {_seed = v;}
};

/** @brief templated class to blend between two images */
template <class PIX, int nComponents, int max>
class NoiseGenerator : public NoiseGeneratorBase {
public :
  // ctor
  NoiseGenerator(OFX::ImageEffect &instance)
    : NoiseGeneratorBase(instance)
  {}

  // and do some processing
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    float noiseLevel = _noiseLevel;

    // set up a random number generator
    // Distribution is from 0 to pixel max level times noise level
    std::random_device rd;
    std::mt19937_64 mt(rd());
    mt.seed(_seed + procWindow.y1);
    std::uniform_real_distribution<double> dist(0.0, max * noiseLevel);

    // push pixels
    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(_effect.abort()) break;

      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {
        for(int c = 0; c < nComponents; c++) {
          double randValue = dist(mt);

          if(max == 1) // implies floating point, so don't clamp
            dstPix[c] = PIX(randValue);
          else {  // integer base one, clamp it
            dstPix[c] = randValue < 0 ? 0 : (randValue > max ? max : PIX(randValue));
          }
        }
        dstPix += nComponents;
      }
    }
  }

};

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class NoisePlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;

  OFX::DoubleParam  *noise_;

public :
  /** @brief ctor */
  NoisePlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , noise_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    noise_   = fetchDoubleParam("Noise");
  }

  /* Override the render */
  virtual void render(const OFX::RenderArguments &args);

  /* Override the clip preferences, we need to say we are setting the frame varying flag */
  virtual void getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences);

  /* set up and run a processor */
  void setupAndProcess(NoiseGeneratorBase &, const OFX::RenderArguments &args);

  /** @brief The get RoD action.  We flag an infinite rod */
  bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from


/* set up and run a processor */
void
NoisePlugin::setupAndProcess(NoiseGeneratorBase &processor, const OFX::RenderArguments &args)
{
  // get a dst image
  std::unique_ptr<OFX::Image>  dst(dstClip_->fetchImage(args.time));
  //OFX::BitDepthEnum         dstBitDepth    = dst->getPixelDepth();
  //OFX::PixelComponentEnum   dstComponents  = dst->getPixelComponents();

  // set the images
  processor.setDstImg(dst.get());

  // set the render window
  processor.setRenderWindow(args.renderWindow);

  // set the scales
  processor.setNoiseLevel((float)noise_->getValueAtTime(args.time));

  // set the seed based on the current time, and double it we get difference seeds on different fields
  processor.setSeed(uint32_t(args.time * 2.0f + 2000.0f));

  // Call the base class process member, this will call the derived templated process code
  processor.process();
}

/* Override the clip preferences, we need to say we are setting the frame varying flag */
void
NoisePlugin::getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences)
{
  clipPreferences.setOutputFrameVarying(true);
}

/** @brief The get RoD action.  We flag an infinite rod */
bool
NoisePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &/*args*/, OfxRectD &rod)
{
  // we can generate noise anywhere on the image plan, so set our RoD to be infinite
  rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
  rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
  return true;
}

// the overridden render function
void
NoisePlugin::render(const OFX::RenderArguments &args)
{
  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) {
    switch(dstBitDepth)
    {
    case OFX::eBitDepthUByte : {
      NoiseGenerator<unsigned char, 4, 255> fred(*this);
      setupAndProcess(fred, args);
                               }
                               break;

    case OFX::eBitDepthUShort :
      {
        NoiseGenerator<unsigned short, 4, 65535> fred(*this);
        setupAndProcess(fred, args);
      }
      break;

    case OFX::eBitDepthFloat :
      {
        NoiseGenerator<float, 4, 1> fred(*this);
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
        NoiseGenerator<unsigned char, 1, 255> fred(*this);
        setupAndProcess(fred, args);
      }
      break;

    case OFX::eBitDepthUShort :
      {
        NoiseGenerator<unsigned short, 1, 65535> fred(*this);
        setupAndProcess(fred, args);
      }
      break;

    case OFX::eBitDepthFloat :
      {
        NoiseGenerator<float, 1, 1> fred(*this);
        setupAndProcess(fred, args);
      }
      break;
    default :
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
}

mDeclarePluginFactory(NoiseExamplePluginFactory, {}, {});

using namespace OFX;

void NoiseExamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
  desc.setLabels("Noise", "Noise", "Noise");
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
}

void NoiseExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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
  DoubleParamDescriptor *param = desc.defineDoubleParam("Noise");
  param->setLabels("noise", "noise", "noise");
  param->setScriptName("noise");
  param->setHint("How much noise to make.");
  param->setDefault(0.2);
  param->setRange(0, 10);
  param->setIncrement(0.1);
  param->setDisplayRange(0, 1);
  param->setAnimates(true); // can animate
  param->setDoubleType(eDoubleTypeScale);
  PageParamDescriptor *page = desc.definePageParam("Controls");
  page->addChild(*param);
}

ImageEffect* NoiseExamplePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
  return new NoisePlugin(handle);
}

namespace OFX
{
  namespace Plugin
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static NoiseExamplePluginFactory p("net.sf.openfx.noisePlugin", 1, 0);
      ids.push_back(&p);
    }
  };
};
