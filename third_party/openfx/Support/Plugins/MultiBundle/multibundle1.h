

#include "ofxsCore.h"

class GammaExamplePluginFactory : public OFX::PluginFactoryHelper<GammaExamplePluginFactory>
{
public:
  GammaExamplePluginFactory():OFX::PluginFactoryHelper<GammaExamplePluginFactory>("net.sf.openfx.gammaexample", 1, 0){}
  virtual void describe(OFX::ImageEffectDescriptor &desc);
  virtual void describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context);
  virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context);
};

