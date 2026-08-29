

#include "ofxsImageEffect.h"

class DotExamplePluginFactory : public OFX::PluginFactoryHelper<DotExamplePluginFactory>
{
public:
  DotExamplePluginFactory():OFX::PluginFactoryHelper<DotExamplePluginFactory>("net.sf.openfx.dotexample", 1, 0){}
  virtual void describe(OFX::ImageEffectDescriptor &desc);
  virtual void describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context);
  virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context);
};

