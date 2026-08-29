

#include <stdio.h>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

#include "../include/ofxsProcessing.H"
#include "../include/ofxsImageBlender.H"

////////////////////////////////////////////////////////////////////////////////
/** @brief The plugin that does our work */
class CrossFadePlugin : public OFX::ImageEffect {
protected :
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;
  OFX::Clip *fromClip_;
  OFX::Clip *toClip_;

  OFX::DoubleParam  *transition_;

public :
  /** @brief ctor */
  CrossFadePlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , fromClip_(0)
    , toClip_(0)
    , transition_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    fromClip_ = fetchClip(kOfxImageEffectTransitionSourceFromClipName);
    toClip_   = fetchClip(kOfxImageEffectTransitionSourceToClipName);
    transition_   = fetchDoubleParam("Transition");
  }

  /* Override the render */
  virtual void render(const OFX::RenderArguments &args);

  /* override is identity */
  virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime);

  /* set up and run a processor */
  void
    setupAndProcess(OFX::ImageBlenderBase &, const OFX::RenderArguments &args);
};


////////////////////////////////////////////////////////////////////////////////
/** @brief render for the filter */

////////////////////////////////////////////////////////////////////////////////
// basic plugin render function, just a skelington to instantiate templates from

// make sure components are sane
static void
checkComponents(const OFX::Image &src,
                OFX::BitDepthEnum dstBitDepth,
                OFX::PixelComponentEnum dstComponents)
{
  OFX::BitDepthEnum      srcBitDepth     = src.getPixelDepth();
  OFX::PixelComponentEnum srcComponents  = src.getPixelComponents();

  // see if they have the same depths and bytes and all
  if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
    throw int(1); // HACK!! need to throw an sensible exception here!        
}

/* set up and run a processor */
void
CrossFadePlugin::setupAndProcess(OFX::ImageBlenderBase &processor, const OFX::RenderArguments &args)
{
  // get a dst image
  std::unique_ptr<OFX::Image>  dst(dstClip_->fetchImage(args.time));
  OFX::BitDepthEnum          dstBitDepth    = dst->getPixelDepth();
  OFX::PixelComponentEnum    dstComponents  = dst->getPixelComponents();

  // fetch the two source images
  std::unique_ptr<OFX::Image> fromImg(fromClip_->fetchImage(args.time));
  std::unique_ptr<OFX::Image> toImg(toClip_->fetchImage(args.time));

  // make sure bit depths are sane
  if(fromImg.get()) checkComponents(*fromImg, dstBitDepth, dstComponents);
  if(toImg.get()) checkComponents(*toImg, dstBitDepth, dstComponents);

  // get the transition value
  float blend = (float)transition_->getValueAtTime(args.time);

  // set the images
  processor.setDstImg(dst.get());
  processor.setFromImg(fromImg.get());
  processor.setToImg(toImg.get());

  // set the render window
  processor.setRenderWindow(args.renderWindow);

  // set the scales
  processor.setBlend(blend);

  // Call the base class process member, this will call the derived templated process code
  processor.process();
}

// the overridden render function
void
CrossFadePlugin::render(const OFX::RenderArguments &args)
{
  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) {
    switch(dstBitDepth) {
case OFX::eBitDepthUByte : {      
  OFX::ImageBlender<unsigned char, 4> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  OFX::ImageBlender<unsigned short, 4> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  OFX::ImageBlender<float, 4> fred(*this);
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
  OFX::ImageBlender<unsigned char, 1> fred(*this);
  setupAndProcess(fred, args);
                           }
                           break;

case OFX::eBitDepthUShort : {
  OFX::ImageBlender<unsigned short, 1> fred(*this);
  setupAndProcess(fred, args);
                            }                          
                            break;

case OFX::eBitDepthFloat : {
  OFX::ImageBlender<float, 1> fred(*this);
  setupAndProcess(fred, args);
                           }                          
                           break;
default :
  OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  } // switch
}

// overridden is identity
bool
CrossFadePlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  // get the transition value
  float blend = (float)transition_->getValueAtTime(args.time);

  identityTime = args.time;

  // at the start?
  if(blend <= 0.0) {
    identityClip = fromClip_;
    identityTime = args.time;
    return true;
  }

  // at the end?
  if(blend >= 1.0) {
    identityClip = toClip_;
    identityTime = args.time;
    return true;
  }

  // nope, identity we isnt
  return false;
}

mDeclarePluginFactory(CrossFadeExamplePluginFactory, {}, {});
using namespace OFX;

void CrossFadeExamplePluginFactory::describe(OFX::ImageEffectDescriptor &desc) 
{
  // basic labels
  desc.setLabels("Cross Fade", "Cross Fade", "Cross Fade");
  desc.setPluginGrouping("OFX Example (Support)");

  // Say we are a transition context
  desc.addSupportedContext(eContextTransition);
  desc.addSupportedContext(eContextGeneral);

  // Add supported pixel depths
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

void CrossFadeExamplePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
  // we are a transition, so define the sourceFrom input clip
  ClipDescriptor *fromClip = desc.defineClip(kOfxImageEffectTransitionSourceFromClipName);
  fromClip->addSupportedComponent(ePixelComponentRGBA);
  fromClip->addSupportedComponent(ePixelComponentAlpha);
  fromClip->setTemporalClipAccess(false);
  fromClip->setSupportsTiles(true);

  // we are a transition, so define the sourceTo input clip
  ClipDescriptor *toClip = desc.defineClip(kOfxImageEffectTransitionSourceToClipName);
  toClip->addSupportedComponent(ePixelComponentRGBA);
  toClip->addSupportedComponent(ePixelComponentAlpha);
  toClip->setTemporalClipAccess(false);
  toClip->setSupportsTiles(true);

  // create the mandated output clip
  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

  // Define the mandated "Transition" param, note that we don't do anything with this other than.
  // describe it. It is not a true param but how the host indicates to the plug-in how far through
  // the transition it is. It appears on no plug-in side UI, it is purely the hosts to manage.
  DoubleParamDescriptor *param = desc.defineDoubleParam("Transition");
  (void)param;
}

ImageEffect* CrossFadeExamplePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
  return new CrossFadePlugin(handle);
}

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
      static CrossFadeExamplePluginFactory p("net.sf.openfx.crossFade", 1, 0);
      ids.push_back(&p);
    }
  }
}
