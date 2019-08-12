#include "colorservice.h"

const int kRGBAChannels = 4;

ColorService::ColorService()
{

}

void ColorService::ConvertFrame(FramePtr f)
{
  OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile("/run/media/matt/Home/OpenColorIO/ocio.configs.0.7v4/nuke-default/config.ocio");

//  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

  OCIO::ConstProcessorRcPtr processor = config->getProcessor("srgb",
                                                             OCIO::ROLE_SCENE_LINEAR);

  OCIO::PackedImageDesc img(reinterpret_cast<float*>(f->data()), f->width(), f->height(), kRGBAChannels);

  processor->apply(img);
}
