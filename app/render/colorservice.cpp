#include "colorservice.h"

const int kRGBAChannels = 4;

ColorService::ColorService()
{
  // FIXME: Hardcoded values for testing purposes
  OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile("/run/media/matt/Home/OpenColorIO/ocio.configs.0.7v4/nuke-default/config.ocio");

  processor = config->getProcessor("srgb",
                                   OCIO::ROLE_SCENE_LINEAR);
}

void ColorService::ConvertFrame(FramePtr f)
{
  OCIO::PackedImageDesc img(reinterpret_cast<float*>(f->data()), f->width(), f->height(), kRGBAChannels);

  processor->apply(img);
}
